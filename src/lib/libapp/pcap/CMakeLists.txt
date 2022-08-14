INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(SRC "")
aux_source_directory(./ SRC)
aux_source_directory(./cmd SRC)
aux_source_directory(./dbg SRC)
aux_source_directory(./init SRC)
aux_source_directory(./main SRC)
set(NAME mypcap)
INCLUDE(../my_libapp.txt)


