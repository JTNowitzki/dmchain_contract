#include <eosio.token/eosio.token.hpp>

namespace eosio {

void token::create(account_name issuer,
    asset max_supply)
{
    asset a(0, max_supply.symbol);
    excreate(system_account, max_supply, a, time_point_sec());
}

void token::issue(account_name to, asset quantity, string memo)
{
    exissue(to, extended_asset(quantity, system_account), memo);
}

void token::retire(asset quantity, string memo)
{
    exretire(system_account, extended_asset(quantity, system_account), memo);
}

void token::transfer(account_name from, account_name to, asset quantity, string memo)
{
    extransfer(from, to, extended_asset(quantity, system_account), memo);
}

void token::close(account_name owner, symbol_type symbol)
{
    exclose(owner, extended_symbol(symbol, system_account));
}
}