#include <eosio.token/eosio.token.hpp>

namespace eosio {
void token::stake(account_name owner, extended_asset asset, double price, string memo)
{
    require_auth(owner);
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    extended_symbol s_sym = asset.get_extended_symbol();
    eosio_assert(s_sym == pst_sym, "only proof of service token can be staked");
    eosio_assert(price >= 0 && price < std::pow(2, 32), "invaild price");

    stats statstable(_self, system_account);
    const auto& st = statstable.get(pst_sym.name(), "token with symbol does not exist"); // never happened

    uint64_t price_t = price * std::pow(2, 32);
    sub_balance(owner, asset);
    stake_stats sst(_self, owner);
    sst.emplace(_self, [&](auto& r) {
        r.primary = sst.available_primary_key();
        r.owner = owner;
        r.unmatched = asset;
        r.matched = extended_asset(0, pst_sym);
        r.price = price_t;
        r.created_at = time_point_sec(now());
        r.updated_at = time_point_sec(now());
    });
}

void token::unstake(account_name owner, uint64_t primary, string memo)
{
    require_auth(owner);
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    calbonus(owner, primary, true);
}

void token::getincentive(account_name owner, uint64_t primary)
{
    require_auth(owner);
    calbonus(owner, primary, false);
}

void token::calbonus(account_name owner, uint64_t primary, bool unstake)
{
    stake_stats sst(_self, owner);
    const auto& ust = sst.get(primary, "no such record");

    stats statstable(_self, system_account);
    const auto& st = statstable.get(rsi_sym.name(), "real storage incentive does not exist"); // never happened

    auto now_time = time_point_sec(now());
    auto duration = now_time.sec_since_epoch() - ust.updated_at.sec_since_epoch();
    eosio_assert(duration <= now_time.sec_since_epoch(), "subtractive overflow"); // never happened
    extended_asset quantity = per_sec_bonus;
    quantity.amount *= duration;
    quantity.amount *= ust.unmatched.amount;
    eosio_assert(quantity.amount > 0, "dust attack detected");

    eosio_assert(quantity.amount <= st.max_supply.amount - st.supply.amount - st.reserve_supply.amount, "amount exceeds available supply when issue");
    statstable.modify(st, 0, [&](auto& s) {
        s.supply += quantity;
    });

    if (unstake) {
        add_balance(owner, ust.unmatched, owner);
        sst.erase(ust);
    } else {
        sst.modify(ust, 0, [&](auto& r) {
            r.updated_at = now_time;
        });
    }
    add_balance(owner, quantity, owner);
}

void token::setabostats(uint64_t stage, double user_rate, double foundation_rate, extended_asset total_release, time_point_sec start_at, time_point_sec end_at)
{
    require_auth(eos_account);
    eosio_assert(stage >= 1 && stage <= 11, "invaild stage");
    eosio_assert(user_rate <= 1 && user_rate >= 0, "invaild user_rate");
    eosio_assert(foundation_rate + user_rate == 1, "invaild foundation_rate");
    eosio_assert(start_at < end_at, "invaild time");
    eosio_assert(total_release.get_extended_symbol() == dmc_sym, "invaild symbol");

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
        SEND_INLINE_ACTION(*this, issue, { eos_account, N(active) }, { abo_account, to_user, "allocation to user" });
    }
}
} // namespace eosio
