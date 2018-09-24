
add_executable( cyvad main.cpp )

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling cyvad with TCMalloc")
    #    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

# We have to link against graphene_debug_miner because deficiency in our API infrastructure doesn't allow plugins to be fully abstracted #246
target_link_libraries( cyvad
                       PRIVATE graphene_app graphene_account_history graphene_miner graphene_chain graphene_debug_miner graphene_egenesis_cyva fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   cyvad

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
