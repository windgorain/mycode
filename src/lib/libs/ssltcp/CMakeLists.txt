INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

set(NAME "ssltcp")
set(SRC "")
aux_source_directory(./ SRC)

INCLUDE(../my_libs_cmake.txt)

