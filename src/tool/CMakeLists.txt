SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/out/tool) 

aux_source_directory(./fake_cert FAKE_CERT_SRC)
add_executable(fakecert ${FAKE_CERT_SRC})
target_link_libraries(fakecert sutil unixlike_util static_bs dl pthread m ssl crypto)

aux_source_directory(./get_cert_chain GET_CERT_CHAIN_SRC)
add_executable(get_cert_chain ${GET_CERT_CHAIN_SRC})
target_link_libraries(get_cert_chain sutil unixlike_util static_bs dl pthread m ssl crypto)

aux_source_directory(./ssldecoder SSLDECODER_SRC)
add_executable(ssldecoder ${SSLDECODER_SRC})
target_link_libraries(ssldecoder sutil unixlike_util static_bs dl pthread m ssl crypto)

aux_source_directory(./md5tool MD5TOOL_SRC)
add_executable(md5tool ${MD5TOOL_SRC})
target_link_libraries(md5tool sutil unixlike_util static_bs dl pthread m ssl crypto)

aux_source_directory(./license LICENSE_SRC)
add_executable(license ${LICENSE_SRC})
IF (UNIX AND NOT APPLE)
target_link_libraries(license sutil unixlike_util dl pthread m ssl crypto rt)
ELSE()
target_link_libraries(license sutil unixlike_util dl pthread m ssl crypto)
ENDIF()

aux_source_directory(./license_hostid LICENSE_HOSTID_SRC)
add_executable(license_hostid ${LICENSE_HOSTID_SRC})
target_link_libraries(license_hostid sutil unixlike_util static_bs dl pthread m ssl crypto)

aux_source_directory(./pcapreader PCAP_SRC)
add_executable(pcapreader ${PCAP_SRC})
target_link_libraries(pcapreader sutil unixlike_util static_bs dl pthread m ssl crypto pcap)

aux_source_directory(./runlua RUNLUA_SRC)
add_executable(runlua ${RUNLUA_SRC})
target_link_libraries(runlua sutil unixlike_util static_bs dl pthread m lua)

aux_source_directory(./runbpf RUNBPF_SRC)
add_executable(runbpf ${RUNBPF_SRC})
target_link_libraries(runbpf sutil static_bs sutil static_bs dl pthread m elf)

aux_source_directory(./json JSON_SRC)
add_executable(json_tool ${JSON_SRC})
target_link_libraries(json_tool sutil unixlike_util static_bs dl pthread m )

aux_source_directory(./htons HTONS_SRC)
add_executable(htons ${HTONS_SRC})
target_link_libraries(htons sutil unixlike_util static_bs dl pthread m ssl crypto pcap)

aux_source_directory(./htonl HTONL_SRC)
add_executable(htonl ${HTONL_SRC})
target_link_libraries(htonl sutil unixlike_util static_bs dl pthread m ssl crypto pcap)

aux_source_directory(./encrypt ENCRYPT_SRC)
add_executable(encrypt ${ENCRYPT_SRC})
target_link_libraries(encrypt sutil unixlike_util )

aux_source_directory(./jhash JHASH_SRC)
add_executable(jhash ${JHASH_SRC})
target_link_libraries(jhash sutil unixlike_util )

aux_source_directory(./dbjhash DBJHASH_SRC)
add_executable(dbjhash ${DBJHASH_SRC})
target_link_libraries(dbjhash sutil unixlike_util )

set(SRC "")
aux_source_directory(./wasm/wasmc SRC)
add_executable(wasmc ${SRC})
target_link_libraries(wasmc sutil unixlike_util dl pthread m ssl crypto readline)

set(SRC "")
aux_source_directory(./wasm/wasme SRC)
add_executable(wasme ${SRC})
target_link_libraries(wasme sutil unixlike_util dl pthread m ssl crypto)

set(SRC "")
aux_source_directory(./pcie_sim SRC)
add_executable(pcie_sim ${SRC})
target_link_libraries(pcie_sim sutil unixlike_util dl pthread)

set(SRC "")
aux_source_directory(./equation SRC)
add_executable(equation ${SRC})
target_link_libraries(equation sutil unixlike_util dl pthread)

set(SRC "")
aux_source_directory(./hexprint SRC)
add_executable(hexprint ${SRC})
target_link_libraries(hexprint sutil unixlike_util dl pthread)

set(SRC "")
aux_source_directory(./tcprelay SRC)
add_executable(tcprelay ${SRC})
target_link_libraries(tcprelay sutil unixlike_util dl pthread)

