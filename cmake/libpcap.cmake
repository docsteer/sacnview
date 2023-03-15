cmake_minimum_required(VERSION 3.14)
include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# Use Npcap on Windows
if (WIN32)
    set(DISABLE_DPDK ON CACHE INTERNAL "Disable Npcap DPDK support")
    FetchContent_Declare(
        npcap
        GIT_REPOSITORY https://github.com/nmap/npcap.git
        GIT_TAG        v1.72
    )
    FetchContent_MakeAvailable(npcap)
    add_subdirectory(${npcap_SOURCE_DIR}/wpcap/libpcap ${npcap_BINARY_DIR})
    include_directories(${npcap_SOURCE_DIR}/wpcap/libpcap)
endif()
