INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

set(SRC "")
aux_source_directory(./ SRC)
aux_source_directory(./init SRC)
aux_source_directory(./fd SRC)
aux_source_directory(./map SRC)
aux_source_directory(./prog SRC)
aux_source_directory(./loader SRC)
aux_source_directory(./cmd SRC)
aux_source_directory(./ebpf SRC)
aux_source_directory(./hookpoint SRC)
aux_source_directory(./verifier SRC)
aux_source_directory(./base_helper SRC)

set(NAME ulc)
INCLUDE(../my_libapp.txt)

target_link_libraries(${LIB_NAME} dynamic-event_hub elf)

