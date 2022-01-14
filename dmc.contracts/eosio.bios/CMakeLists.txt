add_executable(eosio.bios.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/eosio.bios.cpp)
target_include_directories(eosio.bios.wasm
   PUBLIC 
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${EOS_DIR}/eosio.system/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.token/include)

set_target_properties(eosio.bios.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/eosio.bios")


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/abi/eosio.bios.abi" "${CMAKE_CURRENT_SOURCE_DIR}/bin/eosio.bios" COPYONLY)
#install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${WASM_ROOT}/eosio.wasmsdk/include)
