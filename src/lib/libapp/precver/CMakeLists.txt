INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(SRC "")
aux_source_directory(./ SRC)
aux_source_directory(./init SRC)
aux_source_directory(./cmd SRC)
aux_source_directory(./core SRC)
aux_source_directory(./impl SRC)
aux_source_directory(./conf SRC)
aux_source_directory(./comp SRC)

set(NAME precver)
INCLUDE(../my_libapp.txt)

target_link_libraries(${LIB_NAME} comp-pissuer)

IF (USE_DPDK)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -march=native")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")

    include_directories(${CMAKE_SOURCE_DIR}/dpdk-sdk/include)
    target_link_libraries(${LIB_NAME} -Wl,--whole-archive ${CMAKE_SOURCE_DIR}/dpdk-sdk/lib/libdpdk.a -Wl,--no-whole-archive numa)
ENDIF()

