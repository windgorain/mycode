SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/out/spf_runtime) 

set(SRC "")
aux_source_directory(./bare_cmd SRC)
add_executable(bare_cmd ${SRC})
target_link_libraries(bare_cmd sutil static_bs sutil static_bs dl pthread m elf)

set(SRC "")
aux_source_directory(./bare_interactive SRC)
add_executable(bare_interactive ${SRC})
target_link_libraries(bare_interactive sutil static_bs sutil static_bs dl pthread m elf)

set(SRC "")
aux_source_directory(./bare_spf SRC)
add_executable(bare_spf ${SRC})
target_link_libraries(bare_spf sutil static_bs sutil static_bs dl pthread m elf)

