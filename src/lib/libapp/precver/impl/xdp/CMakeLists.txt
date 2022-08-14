INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

set(NAME "precver-xdp")

INCLUDE(../plugin_common.txt)
target_link_libraries(${NAME} bpf)

