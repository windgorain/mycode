INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

aux_source_directory(./ RUNCMD_SRC)
add_executable(runcmd ${RUNCMD_SRC})
target_link_libraries(runcmd bs util dl pthread m ncurses)

