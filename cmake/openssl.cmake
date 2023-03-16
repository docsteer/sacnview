cmake_minimum_required(VERSION 3.14)
include(ExternalProject)

set(OPENSSL_VERSION 1.1.1)
set(OPENSSL_TAG "OpenSSL_1_1_1m")

find_package(OpenSSL ${OPENSSL_VERSION} EXACT)
if ((NOT ${OPENSSL_FOUND}) AND (NOT WIN32))
    # Not found, pull and compile
    ExternalProject_Add(
        OpenSSL
        GIT_REPOSITORY          https://github.com/openssl/openssl.git
        GIT_TAG                 ${OPENSSL_TAG}
        GIT_SHALLOW             TRUE
        GIT_PROGRESS            TRUE
        USES_TERMINAL_DOWNLOAD  TRUE
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD     TRUE
        USES_TERMINAL_INSTALL   TRUE
        CONFIGURE_COMMAND
            <SOURCE_DIR>/config
            --prefix=<INSTALL_DIR>/install
            --openssldir=<INSTALL_DIR>/install
            no-tests
        BUILD_COMMAND
            make
        INSTALL_COMMAND
            make install
        UPDATE_COMMAND          ""
        BUILD_BYPRODUCTS
            <INSTALL_DIR>/install/lib/libssl${CMAKE_SHARED_LIBRARY_SUFFIX}
            <INSTALL_DIR>/install/lib/libcrypto${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    ExternalProject_Get_Property(OpenSSL SOURCE_DIR BINARY_DIR INSTALL_DIR)
    set(OpenSSL_INSTALL_DIR ${INSTALL_DIR}/install)
    set(OpenSSL_BINARY_DIR ${OpenSSL_INSTALL_DIR}/lib)
    set(OpenSSL_INCLUDE_DIR ${OpenSSL_INSTALL_DIR}/include)

    # Create the dir now, else INTERFACE_INCLUDE_DIRECTORIES fails
    file(MAKE_DIRECTORY ${OpenSSL_INCLUDE_DIR})

    add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
    set_target_properties(OpenSSL::SSL PROPERTIES
        IMPORTED_LOCATION ${OpenSSL_BINARY_DIR}/libssl${CMAKE_SHARED_LIBRARY_SUFFIX}
        INTERFACE_INCLUDE_DIRECTORIES ${OpenSSL_INCLUDE_DIR}
    )
    add_dependencies(OpenSSL::SSL OpenSSL)

    add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
    set_target_properties(OpenSSL::Crypto PROPERTIES
        IMPORTED_LOCATION ${OpenSSL_BINARY_DIR}/libcrypto${CMAKE_SHARED_LIBRARY_SUFFIX}
        INTERFACE_INCLUDE_DIRECTORIES ${OpenSSL_INCLUDE_DIR}
    )
    add_dependencies(OpenSSL::Crypto OpenSSL)

endif((NOT ${OPENSSL_FOUND}) AND (NOT WIN32))
