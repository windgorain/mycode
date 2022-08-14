INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(NAME pipecmds)
set(SRC "")
aux_source_directory(./ SRC)
INCLUDE(../my_libapp.txt)

target_link_libraries(${LIB_NAME} applib dynamic-poller)
