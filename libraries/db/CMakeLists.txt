file(GLOB HEADERS "include/graphene/db/*.hpp")

set( GRAPHENE_DB_FILES
     undo_database.cpp
     index.cpp
     object_database.cpp
     ${HEADERS}
    )

if (NOT ${GRAPHENE_BUILD_DYNAMIC_LIBRARIES})
    add_library( graphene_db ${GRAPHENE_DB_FILES} )
else()
    add_library( graphene_db SHARED ${GRAPHENE_DB_FILES} )
endif()

target_link_libraries( graphene_db graphene_protocol fc )
target_include_directories( graphene_db PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   graphene_db

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/db" )
