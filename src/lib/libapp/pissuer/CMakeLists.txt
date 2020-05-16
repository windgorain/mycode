INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

set(SRC "")
aux_source_directory(./ SRC)
aux_source_directory(./core SRC)
aux_source_directory(./comp SRC)
aux_source_directory(./init SRC)

set(NAME pissuer)
INCLUDE(../my_libapp.txt)
