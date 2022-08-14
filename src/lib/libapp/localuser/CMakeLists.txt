INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(NAME localuser)
set(SRC "")
aux_source_directory(./ SRC)
aux_source_directory(./init SRC)
aux_source_directory(./core SRC)

INCLUDE(../my_libapp.txt)
target_link_libraries(${LIB_NAME} dynamic-kfapp)
