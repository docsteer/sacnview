cmake_minimum_required(VERSION 3.18)
include(ExternalProject)

find_package(OpenSSL)

if(OPENSSL_FOUND)
    set(OPENSSL_RUNTIME_DLLS
        "${OPENSSL_INCLUDE_DIR}/../bin/libssl*.dll"
        "${OPENSSL_INCLUDE_DIR}/../bin/libcrypto*.dll"
        )

    message("Found Open SSL, version ${OPENSSL_VERSION} at ${OPENSSL_ROOT_DIR}")
else()
    message("OpenSSL Not Found")
endif()
