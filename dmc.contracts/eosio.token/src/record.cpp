#include <eosio.token/eosio.token.hpp>

namespace eosio {

void token::receipt(extended_asset in, extended_asset out, extended_asset fee)
{
    require_auth(_self);
}

void token::outreceipt(account_name owner, extended_asset x, extended_asset y)
{
    require_auth(_self);
}

void token::traderecord(account_name owner, account_name oppo, extended_asset from, extended_asset to, extended_asset fee, uint64_t bid_id)
{
    require_auth(_self);
}

void token::orderchange(uint64_t bid_id, uint8_t state)
{
    require_auth(_self);
}

void token::bidrec(uint64_t price, extended_asset quantity, extended_asset filled, uint64_t bid_id)
{
    require_auth(_self);
}

void token::pricerec(uint64_t old_price, uint64_t new_price)
{
    require_auth(_self);
}

void token::uniswapsnap(account_name owner, extended_asset quantity)
{
    require_auth(_self);
    require_recipient(owner);
}
} /// namespace eosio
