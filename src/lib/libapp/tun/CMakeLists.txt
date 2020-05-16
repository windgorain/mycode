INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(SRC "")
aux_source_directory(./core SRC)
aux_source_directory(./init SRC)
set(NAME tun)
INCLUDE(../my_libapp.txt)

target_link_libraries(${LIB_NAME} comp-if comp-poller)
