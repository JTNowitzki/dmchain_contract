/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/eosio.hpp>

#define private public
#include "../../../eosio.contracts/eosio.system/include/eosio.system/eosio.system.hpp"
#undef private

#define apply apply_old
#include "../../../eosio.contracts/eosio.system/src/eosio.system.cpp"
#undef apply

namespace eosiosystem {

class boot_contract : public system_contract {

public:
    boot_contract(account_name s)
        : system_contract(s){};

public:
    void setstake(uint64_t stake);
};

} /// eosiosystem
