/**
 *  @file
 *  @copyright defined in fibos/LICENSE.txt
 */

#include <eosio.token/eosio.token.hpp>

namespace eosio {
void token::bill(account_name owner, extended_asset asset, double price, string memo)
{
    require_auth(owner);
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    extended_symbol s_sym = asset.get_extended_symbol();
    eosio_assert(s_sym == pst_sym, "only proof of service token can be billed");
    eosio_assert(price >= 0.0001 && price < std::pow(2, 32), "invaild price");
    eosio_assert(asset.amount > 0, "must bill a positive amount");

    uint64_t price_t = price * std::pow(2, 32);
    sub_balance(owner, asset);
    bill_stats sst(_self, owner);

    auto hash = sha256<stake_id_args>({ owner, asset, price_t, now(), memo });
    uint64_t bill_id = uint64_t(*reinterpret_cast<const uint64_t*>(&hash));

    sst.emplace(_self, [&](auto& r) {
        r.primary = sst.available_primary_key();
        r.bill_id = bill_id;
        r.owner = owner;
        r.unmatched = asset;
        r.matched = extended_asset(0, pst_sym);
        r.price = price_t;
        r.created_at = time_point_sec(now());
        r.updated_at = time_point_sec(now());
    });
    SEND_INLINE_ACTION(*this, billrec, { _self, N(active) }, { owner, asset, bill_id, BILL });
}

void token::unbill(account_name owner, uint64_t bill_id, string memo)
{
    require_auth(owner);
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    bill_stats sst(_self, owner);
    auto ust_idx = sst.get_index<N(byid)>();
    auto ust = ust_idx.find(bill_id);
    eosio_assert(ust != ust_idx.end(), "no such record");
    extended_asset unmatched_asseet = ust->unmatched;
    calbonus(owner, bill_id, true, owner);
    add_balance(owner, unmatched_asseet, owner);

    SEND_INLINE_ACTION(*this, billrec, { _self, N(active) }, { owner, unmatched_asseet, bill_id, UNBILL });
}

void token::order(account_name owner, account_name miner, uint64_t bill_id, extended_asset asset, string memo)
{
    require_auth(owner);
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    eosio_assert(asset.get_extended_symbol() == pst_sym, "only proof of service token can be ordered");
    eosio_assert(asset.amount > 0, "must order a positive amount");
    require_recipient(owner);
    require_recipient(miner);

    bill_stats sst(_self, miner);
    auto ust_idx = sst.get_index<N(byid)>();
    auto ust = ust_idx.find(bill_id);
    eosio_assert(ust != ust_idx.end(), "no such record");
    eosio_assert(ust->unmatched >= asset, "overdrawn balance");

    double price = (double)ust->price / std::pow(2, 32);
    double dmc_amount = price * asset.amount;
    extended_asset user_to_pay = get_asset_by_amount<double, std::ceil>(dmc_amount, dmc_sym);
    sub_balance(owner, user_to_pay);

    calbonus(miner, bill_id, false, owner);

    // re-open, because calbonus has modified it
    bill_stats sst2(_self, miner);
    auto ust_idx2 = sst2.get_index<N(byid)>();
    auto ust2 = ust_idx2.find(bill_id);
    ust_idx2.modify(ust2, 0, [&](auto& s) {
        s.unmatched -= asset;
        s.matched += asset;
    });

    uint64_t claims_interval = get_dmc_config(name { N(claiminter) }, default_dmc_claims_interval);

    dmc_orders order_tbl(_self, _self);
    auto hash = sha256<order_id_args>({ owner, miner, bill_id, asset, memo, time_point_sec(now()) });
    uint64_t order_id = uint64_t(*reinterpret_cast<const uint64_t*>(&hash));
    while (order_tbl.find(order_id) != order_tbl.end()) {
        order_id += 1;
    }
    order_tbl.emplace(owner, [&](auto& o) {
        o.order_id = order_id;
        o.user = owner;
        o.miner = miner;
        o.bill_id = bill_id;
        o.user_pledge = user_to_pay;
        o.miner_pledge = asset;
        o.settlement_pledge = extended_asset(0, user_to_pay.get_extended_symbol());
        o.lock_pledge = extended_asset(0, user_to_pay.get_extended_symbol());
        o.price = user_to_pay;
        o.state = OrderStateDeliver;
        o.create_date = time_point_sec(now());
        o.claim_date = time_point_sec(now());
    });

    trace_price_history(price, bill_id);
    SEND_INLINE_ACTION(*this, orderrec, { _self, N(active) }, { owner, miner, user_to_pay, asset, bill_id, order_id });
}

void token::getincentive(account_name owner, uint64_t bill_id)
{
    require_auth(owner);
    calbonus(owner, bill_id, false, owner);
}

void token::calbonus(account_name owner, uint64_t bill_id, bool unbill, account_name ram_payer)
{
    bill_stats sst(_self, owner);
    auto ust_idx = sst.get_index<N(byid)>();
    auto ust = ust_idx.find(bill_id);
    eosio_assert(ust != ust_idx.end(), "no such record");

    auto now_time = time_point_sec(now());
    uint64_t now_time_t = now_time.sec_since_epoch();
    uint64_t updatet_at_t = ust->updated_at.sec_since_epoch();
    uint64_t max_dmc_claims_interval = ust->created_at.sec_since_epoch() + default_bill_dmc_claims_interval;

    now_time_t = now_time_t >= max_dmc_claims_interval ? max_dmc_claims_interval : now_time_t;
    uint64_t duration = now_time_t - updatet_at_t;
    eosio_assert(duration <= now_time_t, "subtractive overflow"); // never happened

    extended_asset quantity = get_asset_by_amount<double, std::floor>(get_dmc_config(name { N(bmrate) }, default_benchmark_stake_rate) / 100.0 / default_bill_dmc_claims_interval, rsi_sym);
    quantity.amount *= duration;
    quantity.amount *= ust->unmatched.amount;
    if (quantity.amount != 0) {
        add_stats(quantity);
        add_balance(owner, quantity, ram_payer);
        SEND_INLINE_ACTION(*this, incentiverec, { _self, N(active) }, { owner, quantity, bill_id, 0, 0 });
    }

    if (unbill) {
        ust_idx.erase(ust);
    } else {
        ust_idx.modify(ust, 0, [&](auto& r) {
            r.updated_at = time_point_sec(now_time_t);
        });
    }
}

void token::setabostats(uint64_t stage, double user_rate, double foundation_rate, extended_asset total_release, time_point_sec start_at, time_point_sec end_at)
{
    require_auth(eos_account);
    eosio_assert(stage >= 1 && stage <= 11, "invaild stage");
    eosio_assert(user_rate <= 1 && user_rate >= 0, "invaild user_rate");
    eosio_assert(foundation_rate + user_rate == 1, "invaild foundation_rate");
    eosio_assert(start_at < end_at, "invaild time");
    eosio_assert(total_release.get_extended_symbol() == dmc_sym, "invaild symbol");
    bool set_now = false;
    auto now_time = time_point_sec(now());
    if (now_time > start_at)
        set_now = true;

    abostats ast(_self, _self);
    const auto& st = ast.find(stage);
    if (st != ast.end()) {
        ast.modify(st, 0, [&](auto& a) {
            a.user_rate = user_rate;
            a.foundation_rate = foundation_rate;
            a.total_release = total_release;
            a.start_at = start_at;
            a.end_at = end_at;
        });
    } else {
        ast.emplace(_self, [&](auto& a) {
            a.stage = stage;
            a.user_rate = user_rate;
            a.foundation_rate = foundation_rate;
            a.total_release = total_release;
            a.remaining_release = total_release;
            a.start_at = start_at;
            a.end_at = end_at;
            if (set_now)
                a.last_released_at = time_point_sec(now());
            else
                a.last_released_at = start_at;
        });
    }
}

void token::allocation(string memo)
{
    require_auth(eos_account);
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    abostats ast(_self, _self);
    extended_asset to_foundation(0, dmc_sym);
    extended_asset to_user(0, dmc_sym);

    auto now_time = time_point_sec(now());
    for (auto it = ast.begin(); it != ast.end();) {
        if (now_time > it->end_at) {
            // 超过时间，但前一阶段仍然有份额没有增发完，一次性全部增发
            if (it->remaining_release.amount != 0) {
                to_foundation.amount = it->remaining_release.amount * it->foundation_rate;
                to_user.amount = it->remaining_release.amount * it->user_rate;
                ast.modify(it, 0, [&](auto& a) {
                    a.last_released_at = it->end_at;
                    a.remaining_release.amount = 0;
                });
            }
            it++;
        } else if (now_time < it->start_at) {
            break;
        } else {
            auto duration_time = now_time.sec_since_epoch() - it->last_released_at.sec_since_epoch();
            // 每天只能领取一次
            if (duration_time / 86400 == 0) // 24 * 60 * 60
                break;
            auto remaining_time = it->end_at.sec_since_epoch() - it->last_released_at.sec_since_epoch();
            double per = (double)duration_time / (double)remaining_time;
            if (per > 1)
                per = 1;
            uint64_t total_asset_amount = per * it->remaining_release.amount;
            if (total_asset_amount != 0) {
                to_foundation.amount = total_asset_amount * it->foundation_rate;
                to_user.amount = total_asset_amount * it->user_rate;
                ast.modify(it, 0, [&](auto& a) {
                    a.last_released_at = now_time;
                    a.remaining_release.amount -= total_asset_amount;
                });
            }
            break;
        }
    }

    if (to_foundation.amount != 0 && to_user.amount != 0) {
        SEND_INLINE_ACTION(*this, issue, { eos_account, N(active) }, { system_account, to_foundation, "allocation to foundation" });
        auto dueto = now_time.sec_since_epoch() + 24 * 60 * 60;
        string memo = uint32_to_string(dueto) + ";RSI@datamall";
        SEND_INLINE_ACTION(*this, issue, { eos_account, N(active) }, { abo_account, to_user, memo });
    }
}

void token::setdmcconfig(account_name key, uint64_t value)
{
    require_auth(system_account);
    dmc_global dmc_global_tbl(_self, _self);
    switch (key) {
    case N(stakerate):
        eosio_assert(value > 0 && value < 1, "invalid stake rate");
        break;
    case N(claiminter):
        eosio_assert(value > 0, "invalid claims interval");
        break;
    default:
        break;
    }
    auto config_itr = dmc_global_tbl.find(key);
    if (config_itr == dmc_global_tbl.end()) {
        dmc_global_tbl.emplace(_self, [&](auto& conf) {
            conf.key = key;
            conf.value = value;
        });
    } else {
        dmc_global_tbl.modify(config_itr, 0, [&](auto& conf) {
            conf.value = value;
        });
    }
}

uint64_t token::get_dmc_config(name key, uint64_t default_value)
{
    dmc_global dmc_global_tbl(_self, _self);
    auto dmc_global_iter = dmc_global_tbl.find(key);
    if (dmc_global_iter != dmc_global_tbl.end())
        return dmc_global_iter->value;
    return default_value;
}

void token::trace_price_history(double price, uint64_t bill_id)
{
    price_table ptb(_self, _self);
    auto iter = ptb.get_index<N(bytime)>();
    auto now_time = time_point_sec(now());
    auto rtime = now_time - price_fluncuation_interval;

    avg_table atb(_self, _self);
    auto aitr = atb.begin();
    if (aitr == atb.end()) {
        aitr = atb.emplace(_self, [&](auto& a) {
            a.primary = 0;
            a.total = 0;
            a.count = 0;
            a.avg = 0;
        });
    }

    for (auto it = iter.begin(); it != iter.end();) {
        if (it->created_at < rtime) {
            it = iter.erase(it);
            atb.modify(aitr, _self, [&](auto& a) {
                a.total -= it->price;
                a.count -= 1;
            });
        } else {
            break;
        }
    }

    ptb.emplace(_self, [&](auto& p) {
        p.primary = ptb.available_primary_key();
        p.bill_id = bill_id;
        p.price = price;
        p.created_at = now_time;
    });

    atb.modify(aitr, _self, [&](auto& a) {
        a.total += price;
        a.count += 1;
        a.avg = (double)a.total / a.count;
    });
}
} // namespace eosio
