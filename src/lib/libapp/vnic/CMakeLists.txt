INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(SRC "")
aux_source_directory(./ SRC)
set(NAME vnic)
INCLUDE(../my_libapp.txt)

