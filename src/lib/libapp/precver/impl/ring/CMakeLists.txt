INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

set(NAME "precver-ring")

INCLUDE(../plugin_common.txt)

IF (USE_DPDK)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -march=native")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")

    include_directories($ENV{RTE_SDK}/$ENV{RTE_TARGET}/include/)
    include_directories($ENV{NF_TRUNK}/include/dpdk)
    include_directories($ENV{NF_TRUNK}/include/linux)
    include_directories($ENV{NF_TRUNK}/include/share)
    include_directories($ENV{NF_TRUNK}/include/)

    include_directories(${CMAKE_SOURCE_DIR}/dpdk-sdk/include)
	target_link_libraries(${NAME} -Wl,--whole-archive $ENV{NF_TRUNK}/lib/linux/libjson.a -Wl,--no-whole-archive)
	target_link_libraries(${NAME} -Wl,--whole-archive $ENV{NF_TRUNK}/lib/dpdk/libnetframe.a -Wl,--no-whole-archive numa json)
ENDIF()
