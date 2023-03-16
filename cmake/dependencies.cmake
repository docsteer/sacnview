# Dependencies

# Qt
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network Charts Multimedia)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::Multimedia)

# Breakpad
include(breakpad)
target_include_directories(${PROJECT_NAME} PRIVATE ${BREAKPAD_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Breakpad)

# Blake2
include(blake2)
target_include_directories(${PROJECT_NAME} PRIVATE ${BLAKE2_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE Blake2)

# libpcap
include(libpcap)
target_include_directories(${PROJECT_NAME} PRIVATE ${PCAP_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE pcap)

# Zlib
if (NOT WIN32)
    find_package(ZLIB REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
endif()

# OpenSSL
include(openssl)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
