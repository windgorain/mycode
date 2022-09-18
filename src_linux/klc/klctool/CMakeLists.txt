set(SRC "")
aux_source_directory(./ SRC)
add_executable(klctool ${SRC})
set_target_properties(klctool PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/out/klc")
target_link_libraries(klctool klc_lib linux_util sutil dl pthread elf z)


