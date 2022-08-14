INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

aux_source_directory(./utun SRC)

add_library(macos_util ${SRC} )

set_property(TARGET macos_util PROPERTY POSITION_INDEPENDENT_CODE ON)
