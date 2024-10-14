INCLUDE(${CMAKE_SOURCE_DIR}/my_cmake_def.txt)
set(SRC "")
aux_source_directory(./ SRC)
aux_source_directory(./init SRC)
aux_source_directory(./cmd SRC)
aux_source_directory(./core SRC)
aux_source_directory(./impl SRC)
aux_source_directory(./conf SRC)
aux_source_directory(./plug SRC)
aux_source_directory(./ev SRC)

set(NAME precver)
INCLUDE(../my_libapp.txt)

target_link_libraries(${LIB_NAME} dynamic-event_hub)

#plugin
add_subdirectory(./impl/http)
add_subdirectory(./impl/udp)
add_subdirectory(./impl/pcap)
add_subdirectory(./impl/null)
add_subdirectory(./impl/rand)
add_subdirectory(./impl/tcphsk)
add_subdirectory(./impl/ring)
add_subdirectory(./impl/shm)

IF (UNIX AND NOT APPLE)
add_subdirectory(./impl/rawsocket)
ENDIF()

IF (USE_XDP)    
add_subdirectory(./impl/xdp)
target_link_libraries(${LIB_NAME} xdp)
ENDIF()

