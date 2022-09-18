INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

set(SRC "")

IF (UNIX AND NOT APPLE AND USE_KPF)

include_directories(./include)
include_directories(./include/uapi)
include_directories(${CMAKE_SOURCE_DIR}/h/bpf)
aux_source_directory(./src SRC)
add_library(mylibbpf ${SRC})
target_link_libraries(mylibbpf unixlike_util elf)
set_property(TARGET mylibbpf PROPERTY POSITION_INDEPENDENT_CODE ON)

ENDIF()


