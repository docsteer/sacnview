# All 3rd party libraries

# Bundled OpenSSL for Qt5
if (WIN32)
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64")
    set(OPENSSL_LIBS
        ${CMAKE_CURRENT_LIST_DIR}/openssl-1.1.1q-win64/libcrypto-1_1-x64.dll
        ${CMAKE_CURRENT_LIST_DIR}/openssl-1.1.1q-win64/libssl-1_1-x64.dll
    )
  else()
    set(OPENSSL_LIBS
        ${CMAKE_CURRENT_LIST_DIR}/openssl-1.1.1q-win32/libcrypto-1_1.dll
        ${CMAKE_CURRENT_LIST_DIR}/openssl-1.1.1q-win632/libssl-1_1.dll
    )
  endif()
endif()

# WinPCap Library
if (WIN32)
  set(PCAP_LIBS wpcap Packet)

  add_library(wpcap SHARED IMPORTED GLOBAL)
  add_library(Packet SHARED IMPORTED GLOBAL)

  set(PCAP_ROOT_PATH ${CMAKE_CURRENT_LIST_DIR}/WpdPack_4_1_2)

  target_include_directories(wpcap INTERFACE ${PCAP_ROOT_PATH}/Include)

  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64")
    set_property(TARGET wpcap PROPERTY IMPORTED_IMPLIB ${PCAP_ROOT_PATH}/Lib/x64/wpcap.lib)
    set_property(TARGET Packet PROPERTY IMPORTED_IMPLIB ${PCAP_ROOT_PATH}/Lib/x64/Packet.lib)
  else()
    set_property(TARGET wpcap PROPERTY IMPORTED_IMPLIB ${PCAP_ROOT_PATH}/Lib/wpcap.lib)
    set_property(TARGET Packet PROPERTY IMPORTED_IMPLIB ${PCAP_ROOT_PATH}/Lib/Packet.lib)
  endif()
else()
  set(PCAP_LIBS pcap)
endif()

# Blake2
if((${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64") AND NOT APPLE)
  # 64bit SSE
  set(BLAKE2_PATH ${CMAKE_CURRENT_LIST_DIR}/blake2/sse)
  set(BLAKE2_DEFINES HAVE_SSE2)
  set(BLAKE2_SOURCES
    ${BLAKE2_PATH}/blake2b.c
    ${BLAKE2_PATH}/blake2bp.c
    ${BLAKE2_PATH}/blake2s.c
    ${BLAKE2_PATH}/blake2sp.c
    ${BLAKE2_PATH}/blake2xb.c
    ${BLAKE2_PATH}/blake2xs.c
  )
else()
  # Reference implementation
  set(BLAKE2_PATH ${CMAKE_CURRENT_LIST_DIR}/blake2/ref)
  set(BLAKE2_SOURCES
    ${BLAKE2_PATH}/blake2bp-ref.c
    ${BLAKE2_PATH}/blake2b-ref.c
    ${BLAKE2_PATH}/blake2sp-ref.c
    ${BLAKE2_PATH}/blake2s-ref.c
    ${BLAKE2_PATH}/blake2xb-ref.c
    ${BLAKE2_PATH}/blake2xs-ref.c
  )
endif()

# Breakpad
if(WIN32)
    message(STATUS "Configuring Breakpad")
    set(USING_BREAKPAD true)

    set(DEPOT_TOOLS_PATH ${CMAKE_CURRENT_LIST_DIR}/../tools/depot_tools)
    set(BREAKPAD_FETCH_DIR ${CMAKE_CURRENT_LIST_DIR}/breakpad)
    set(BREAKPAD_PATH ${BREAKPAD_FETCH_DIR}/src)

    if(NOT EXISTS ${BREAKPAD_PATH})
        message(STATUS "Fetching Breakpad using depot_tools (${DEPOT_TOOLS_PATH})")
        file(MAKE_DIRECTORY ${BREAKPAD_FETCH_DIR})
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env "PATH=${DEPOT_TOOLS_PATH};$ENV{PATH}" fetch.bat breakpad
            WORKING_DIRECTORY ${BREAKPAD_FETCH_DIR}
            RESULT_VARIABLE BREAKPAD_FETCH_RESULT
        )
        if(NOT BREAKPAD_FETCH_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to fetch Breakpad using depot_tools (exit code ${BREAKPAD_FETCH_RESULT})")
        endif()
    endif()

    set(BREAKPAD_SOURCES
        ${BREAKPAD_PATH}/src/client/windows/handler/exception_handler.cc
        ${BREAKPAD_PATH}/src/common/windows/string_utils.cc
        ${BREAKPAD_PATH}/src/common/windows/guid_string.cc
        ${BREAKPAD_PATH}/src/client/windows/crash_generation/crash_generation_client.cc
    )
    set(BREAKPAD_HEADER_PATHS
        ${BREAKPAD_PATH}/src
    )
endif()
