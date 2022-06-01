/**
 *  @file
 *  @copyright defined in fibos/LICENSE.txt
 */

#include <eosio.token/eosio.token.hpp>

namespace eosio {

void token::exdestroy(extended_symbol sym)
{
    eosio_assert(sym.is_valid(), "invalid symbol name");

    auto sym_name = sym.name();

    require_auth(sym.contract);

    stats statstable(_self, sym.contract);
    const auto& st = statstable.get(sym_name, "token with symbol does not exist");

    if (st.supply.amount > 0) {
        sub_balance(st.issuer, extended_asset(st.supply, st.issuer));
    }

    if (st.reserve_supply.amount > 0) {
        lock_accounts from_acnts(_self, sym.contract);

        uint64_t balances = 0;
        for (auto it = from_acnts.begin(); it != from_acnts.end();) {
            if (it->balance.get_extended_symbol() == sym) {
                balances += it->balance.amount;
                it = from_acnts.erase(it);
            } else {
                it++;
            }
        }

        eosio_assert(st.reserve_supply.amount == balances, "reserve_supply must all in issuer");
    }

    statstable.erase(st);
}

void token::exchange(account_name owner, extended_asset quantity, extended_asset to, double price, account_name id, string memo)
{
    require_auth(owner);
    eosio_assert(quantity.is_valid() && to.is_valid(), "invalid exchange currency");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");
    price = 0;
    auto from_sym = quantity.get_extended_symbol();
    auto to_sym = to.get_extended_symbol();

    uniswaporder(owner, quantity, to, price, id, owner);
}

} /// namespace eosio
