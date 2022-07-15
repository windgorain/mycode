INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

aux_source_directory(./hookapi SRC)
aux_source_directory(./my_ip_helper SRC)
aux_source_directory(./netfw SRC)
aux_source_directory(./tray SRC)
aux_source_directory(./update SRC)
aux_source_directory(./vnic SRC)
aux_source_directory(./sock_rdt SRC)

add_library(win_util ${SRC})

set_property(TARGET win_util PROPERTY POSITION_INDEPENDENT_CODE ON)

