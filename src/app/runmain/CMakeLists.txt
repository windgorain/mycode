INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

aux_source_directory(./ RUNMAIN_SRC)
add_executable(runmain ${RUNMAIN_SRC})
target_link_libraries(runmain bs util dl pthread m ncurses)

