INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(SRC "")
aux_source_directory(./ SRC)
set(NAME lua_mgr)
INCLUDE(../my_libapp.txt)
target_link_libraries(${LIB_NAME} lua)

