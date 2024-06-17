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

  set(PCAP_ROOT_PATH ${CMAKE_CURRENT_LIST_DIR}/WinPcap-413-173-b4)

  target_include_directories(wpcap INTERFACE ${PCAP_ROOT_PATH}/Include)

  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64")
    set_property(TARGET wpcap PROPERTY IMPORTED_IMPLIB ${PCAP_ROOT_PATH}/Lib/x64/wpcap.lib)
    set_property(TARGET wpcap PROPERTY IMPORTED_LOCATION ${PCAP_ROOT_PATH}/Bin/x64/wpcap.dll)
    set_property(TARGET Packet PROPERTY IMPORTED_IMPLIB ${PCAP_ROOT_PATH}/Lib/x64/Packet.lib)
    set_property(TARGET Packet PROPERTY IMPORTED_LOCATION ${PCAP_ROOT_PATH}/Bin/x64/Packet.dll)
  else()
    set_property(TARGET wpcap PROPERTY IMPORTED_IMPLIB ${PCAP_ROOT_PATH}/Lib/wpcap.lib)
    set_property(TARGET wpcap PROPERTY IMPORTED_LOCATION ${PCAP_ROOT_PATH}/Bin/wpcap.dll)
    set_property(TARGET Packet PROPERTY IMPORTED_IMPLIB ${PCAP_ROOT_PATH}/Lib/Packet.lib)
    set_property(TARGET Packet PROPERTY IMPORTED_LOCATION ${PCAP_ROOT_PATH}/Bin/Packet.dll)
  endif()
else()
  set(PCAP_LIBS pcap)
endif()

# Blake2
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64")
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
