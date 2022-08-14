INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(SRC "")
aux_source_directory(./ SRC)
aux_source_directory(./init SRC)
aux_source_directory(./cmd SRC)
aux_source_directory(./core SRC)
set(NAME muc)
INCLUDE(../my_libapp.txt)

