INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(SRC "")
aux_source_directory(./ SRC)
set(NAME webcenter)
INCLUDE(../my_libapp.txt)

target_link_libraries(${LIB_NAME} comp-wsapp comp-localuser)
