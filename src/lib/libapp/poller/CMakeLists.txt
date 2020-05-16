INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(NAME poller)
set(SRC "")
aux_source_directory(./ SRC)
aux_source_directory(./cmd SRC)
aux_source_directory(./comp SRC)
aux_source_directory(./core SRC)
aux_source_directory(./init SRC)
aux_source_directory(./plug SRC)
INCLUDE(../my_libapp.txt)


