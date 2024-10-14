INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(SRC "")
aux_source_directory(./ SRC)
aux_source_directory(./init SRC)
aux_source_directory(./cmd SRC)
aux_source_directory(./vht SRC)
aux_source_directory(./vrt SRC)
aux_source_directory(./pkt SRC)
aux_source_directory(./cfg_lock SRC)
aux_source_directory(./rcu SRC)
aux_source_directory(./ovsdb SRC)

set(NAME xgw)
INCLUDE(../my_libapp.txt)

IF (UNIX AND NOT APPLE)
target_link_libraries(${LIB_NAME} ovsdb ovslib)
ENDIF()

