file(GLOB HEADERS "include/graphene/db/*.hpp")
add_library( graphene_db undo_database.cpp index.cpp object_database.cpp ${HEADERS} )
target_link_libraries( graphene_db fc asan )
target_include_directories( graphene_db PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_compile_options(-fsanitize=address -fsanitize-recover=address)

install( TARGETS
   graphene_db

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/db" )
