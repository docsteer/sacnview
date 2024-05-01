cmake_minimum_required(VERSION 3.18)
include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

if (WIN32)
    # Use Npcap on Windows
    FetchContent_Declare(
        npcap
        URL         https://npcap.com/dist/npcap-sdk-1.13.zip
        URL_HASH    MD5=2067B3975763DDF61D4114D28D9D6C9B
    )
    FetchContent_MakeAvailable(npcap)

    # Due to Npcap license restrictions, we can't bundle the dll
    # So we will delay load
    add_definitions(-DPCAP_DLL)
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
    set_property(TARGET pcap PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS true)
    target_link_libraries(${PROJECT_NAME} PRIVATE delayimp)
    target_link_options(${PROJECT_NAME} PRIVATE "/DELAYLOAD:wpcap.dll")

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
