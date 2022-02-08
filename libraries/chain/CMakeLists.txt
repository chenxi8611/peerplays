
add_custom_target( build_hardfork_hpp
                   COMMAND cat-parts "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" "${CMAKE_CURRENT_BINARY_DIR}/include/graphene/chain/hardfork.hpp" )
set_source_files_properties( "${CMAKE_CURRENT_BINARY_DIR}/include/graphene/chain/hardfork.hpp" PROPERTIES GENERATED TRUE )

add_dependencies( build_hardfork_hpp cat-parts )

file(GLOB HEADERS "include/graphene/chain/*.hpp")
list(APPEND HEADERS "${CMAKE_CURRENT_BINARY_DIR}/include/graphene/chain/hardfork.hpp")

file(GLOB CPP_FILES "*.cpp")

#if( GRAPHENE_DISABLE_UNITY_BUILD )
   list(FILTER CPP_FILES EXCLUDE REGEX "[/]database[.]cpp$")
   #message ("--- ${CPP_FILES}")
   message( STATUS "Graphene database unity build disabled" )
#else( GRAPHENE_DISABLE_UNITY_BUILD )
#   set( GRAPHENE_DB_FILES
#        database.cpp )
#   message( STATUS "Graphene database unity build enabled" )
#endif( GRAPHENE_DISABLE_UNITY_BUILD )

set( GRAPHENE_CHAIN_FILES
     ${CPP_FILES}
     ${HEADERS}
     "${CMAKE_CURRENT_BINARY_DIR}/include/graphene/chain/hardfork.hpp"
   )
if (NOT ${GRAPHENE_BUILD_DYNAMIC_LIBRARIES})
    add_library( graphene_chain ${GRAPHENE_CHAIN_FILES} )
else()
    add_library( graphene_chain SHARED ${GRAPHENE_CHAIN_FILES} )
endif()

add_dependencies( graphene_chain build_hardfork_hpp )
target_link_libraries( graphene_chain fc graphene_db graphene_protocol )
target_include_directories( graphene_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if(MSVC)
  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   graphene_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/chain" )
