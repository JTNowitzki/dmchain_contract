#include <eosio.token/eosio.token.hpp>

#include "./classic_token.cpp"
#include "./smart_token.cpp"
#include "./smart_extend.cpp"
#include "./lock_account.cpp"
#include "./record.cpp"
#include "./uniswap.cpp"
#include "./dmc.cpp"

namespace eosio {

} /// namespace eosio

EOSIO_ABI(eosio::token,
    // classic tokens
    (create)(issue)(transfer)(close)(retire)
    // smart tokens
    (excreate)(exissue)(extransfer)(exclose)(exretire)(exdestroy)
    //
    (exchange)
    //
    (exunlock)(exlock)(exlocktrans)
    //
    (receipt)(outreceipt)(traderecord)(orderchange)(bidrec)(uniswapsnap)
    //
    (addreserves)(outreserves)
    //
    (stake)(unstake)(getincentive)(setabostats)(allocation))