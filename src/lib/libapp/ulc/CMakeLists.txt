INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

set(SRC "")
aux_source_directory(./ SRC)
aux_source_directory(./init SRC)
aux_source_directory(./cmd SRC)
aux_source_directory(./hookpoint SRC)
aux_source_directory(./cfg_lock SRC)
aux_source_directory(./ioctl SRC)
aux_source_directory(./runtime SRC)

set(NAME ulc)
INCLUDE(../my_libapp.txt)

target_link_libraries(${LIB_NAME} dynamic-event_hub dynamic-cioctl elf)

