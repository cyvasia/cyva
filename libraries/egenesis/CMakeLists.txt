
add_library( graphene_egenesis_none
   egenesis_none.cpp
   include/graphene/egenesis/egenesis.hpp
   )

target_link_libraries( graphene_egenesis_none graphene_chain fc )
target_include_directories( graphene_egenesis_none
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_executable( embed_genesis
   embed_genesis.cpp
   )

target_link_libraries( embed_genesis graphene_chain graphene_app graphene_egenesis_none fc )

if(MAINNET)
    set(GRAPHENE_EGENESIS_JSON  ${CMAKE_SOURCE_DIR}/genesis.json)
else()
    set(GRAPHENE_EGENESIS_JSON  ${CMAKE_SOURCE_DIR}/genesis-testnet.json)
endif()

set( embed_genesis_args
   "${CMAKE_CURRENT_BINARY_DIR}/embed_genesis -t ${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl---${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp -t ${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl---${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp -t ${CMAKE_CURRENT_SOURCE_DIR}/egenesis_cyva.cpp.tmpl---${CMAKE_CURRENT_SOURCE_DIR}/egenesis_cyva.cpp -t ${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp -t ${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp -t ${CMAKE_CURRENT_SOURCE_DIR}/egenesis_cyva.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_cyva.cpp --genesis-json ${GRAPHENE_EGENESIS_JSON}"
)


MESSAGE( STATUS "embed_genesis_command: " "/bin/sh -c" "${embed_genesis_args}" )


add_custom_command(
   OUTPUT
      "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
      "${CMAKE_CURRENT_BINARY_DIR}/egenesis_cyva.cpp"
      "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
   COMMAND sh -c "${embed_genesis_args}"
   DEPENDS
      "${GRAPHENE_EGENESIS_JSON}"
      "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl"
      "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_cyva.cpp.tmpl"
      "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl"
      embed_genesis )

add_library( graphene_egenesis_brief "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp" include/graphene/egenesis/egenesis.hpp )
add_library( graphene_egenesis_full  "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp" include/graphene/egenesis/egenesis.hpp )
add_library( graphene_egenesis_cyva "${CMAKE_CURRENT_BINARY_DIR}/egenesis_cyva.cpp" include/graphene/egenesis/egenesis.hpp )

target_link_libraries( graphene_egenesis_brief graphene_chain fc )
target_link_libraries( graphene_egenesis_full graphene_chain fc )
target_link_libraries( graphene_egenesis_cyva graphene_chain fc )

target_include_directories( graphene_egenesis_brief
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_include_directories( graphene_egenesis_full
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_include_directories( graphene_egenesis_cyva
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
