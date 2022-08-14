INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(SRC "")
aux_source_directory(./ SRC)
set(NAME signal_slot)
INCLUDE(../my_libapp.txt)

