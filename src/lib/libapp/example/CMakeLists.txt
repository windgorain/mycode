INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(SRC "")
aux_source_directory(./ SRC)
aux_source_directory(./init SRC)
set(NAME example)
INCLUDE(../my_libapp.txt)

#target_link_libraries(${LIB_NAME} comp-pcap)
