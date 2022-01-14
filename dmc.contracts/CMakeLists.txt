cmake_minimum_required(VERSION 3.5)
project(eosio_contracts VERSION 1.1.1)

set(EOS_DIR ${PROJECT_SOURCE_DIR}/../eosio.contracts)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
   set(TEST_BUILD_TYPE "Debug")
   set(CMAKE_BUILD_TYPE "Release")
else()
   set(TEST_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()
if(CXX_COMPILER STREQUAL "" OR NOT CXX_COMPILER)
   set(CXX_COMPILER ${CMAKE_CXX_COMPILER})
endif()

if(EOSIO_INSTALL_PREFIX STREQUAL "" OR NOT EOSIO_INSTALL_PREFIX)
   set(EOSIO_INSTALL_PREFIX "/usr/local")
endif()

# if no wasm root is given use default path
if(WASM_ROOT STREQUAL "" OR NOT WASM_ROOT)
   set(WASM_ROOT ${CMAKE_INSTALL_PREFIX}/eosio.wasmsdk)
endif()

list(APPEND CMAKE_MODULE_PATH ${WASM_ROOT}/lib/cmake)
include(EosioWasmToolchain)

add_subdirectory(eosio.bios)
add_subdirectory(eosio.system)
add_subdirectory(hello)
