INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

set(SRC "")
aux_source_directory(. SRC)
add_executable(rundaemon ${SRC})
target_link_libraries(rundaemon bs util dl pthread m ncurses)

