cmake_minimum_required(VERSION 3.18)
include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

if (WIN32)
    # Use Npcap on Windows
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
else()
    # Use the FindPCAP scripts from Wireshark to locate libpcap
    file(DOWNLOAD
        https://raw.githubusercontent.com/boundary/wireshark/master/cmake/modules/FindWSWinLibs.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/FindWSWinLibs.cmake)
    file(DOWNLOAD
        https://raw.githubusercontent.com/boundary/wireshark/master/cmake/modules/FindPCAP.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/FindPCAP.cmake)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/cmake")

    find_package(PCAP REQUIRED)
endif()
