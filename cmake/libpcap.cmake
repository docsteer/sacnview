cmake_minimum_required(VERSION 3.14)
include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# Use Npcap on Windows
if (WIN32)
    FetchContent_Declare(
        npcap
        URL         https://npcap.com/dist/npcap-sdk-1.13.zip
        URL_HASH    MD5=2067B3975763DDF61D4114D28D9D6C9B
    )
    FetchContent_MakeAvailable(npcap)

    if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID MATCHES "x64")
        set(npcap_LIB_DIR ${npcap_SOURCE_DIR}/Lib/x64)
    else()
        set(npcap_LIB_DIR ${npcap_SOURCE_DIR}/Lib/)
    endif()
    set(npcap_LIB_FILES
        ${npcap_LIB_DIR}/Packet.lib
        ${npcap_LIB_DIR}/wpcap.lib)
    add_library(pcap ${npcap_LIB_FILES})
    target_link_libraries(pcap ${npcap_LIB_FILES})

    set(PCAP_INCLUDE_DIR ${npcap_SOURCE_DIR}/include)
endif()
