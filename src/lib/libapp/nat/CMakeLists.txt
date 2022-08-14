INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(SRC "")
aux_source_directory(./ SRC)
set(NAME nat)
INCLUDE(../my_libapp.txt)

target_link_libraries(${LIB_NAME} dynamic-ifnet dynamic-mypcap)
