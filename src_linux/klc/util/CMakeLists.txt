INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)

set(SRC "")

IF (UNIX AND NOT APPLE)
aux_source_directory(./klctool SRC)
aux_source_directory(./mgwtool SRC)
add_library(klc_lib ${SRC})
set_property(TARGET klc_lib PROPERTY POSITION_INDEPENDENT_CODE ON)
ENDIF()

#INSTALL(TARGETS linux_util ARCHIVE DESTINATION /usr/local/lib/utl/)
