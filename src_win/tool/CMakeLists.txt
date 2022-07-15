INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

aux_source_directory(./win_tun WIN_TUN_SRC)
add_executable(win_tun ${WIN_TUN_SRC})
target_link_libraries(win_tun util win_util ws2_32 iphlpapi legacy_stdio_definitions)

