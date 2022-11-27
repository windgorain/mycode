INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

set(SRC "")

aux_source_directory(./pipe SRC)

add_library(applib ${SRC})
set_property(TARGET applib PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(applib PRIVATE USE_BS)
