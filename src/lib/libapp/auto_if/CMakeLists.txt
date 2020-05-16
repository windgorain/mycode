INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(NAME aif)
set(SRC "")
aux_source_directory(./ SRC)
INCLUDE(../my_libapp.txt)

target_link_libraries(${LIB_NAME} comp-pcap comp-if comp-vnic)
