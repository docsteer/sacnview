cmake_minimum_required(VERSION 3.18)
include(ExternalProject)

set(OPENSSL_TARGET_VERSION 1.1.1)
set(OPENSSL_TARGET_TAG "OpenSSL_1_1_1m")

find_package(OpenSSL ${OPENSSL_TARGET_VERSION} EXACT)
if(NOT ${OPENSSL_FOUND})
    # Not found, pull and compile

    if(WIN32)
        set(OpenSSL_MAKE_COMMAND nmake)

        # Perl is needed to configure
        find_package(Perl)
        if (NOT Perl_FOUND)
            message(FATAL_ERROR "Perl is required for OpenSSL, install from https://strawberryperl.com/")
        endif()

        if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID MATCHES "x64")
            set(OpenSSL_CONFIG_COMMAND
                ${PERL_EXECUTABLE} <SOURCE_DIR>/Configure VC-WIN64A-masm)
        elseif()
            set(OpenSSL_CONFIG_COMMAND
                ${PERL_EXECUTABLE} <SOURCE_DIR>/Configure VC-WIN32)
        endif()

    else()
        set(OpenSSL_MAKE_COMMAND make)
        set(OpenSSL_CONFIG_COMMAND <SOURCE_DIR>/config)   
    endif()

    ExternalProject_Add(
        OpenSSL
        GIT_REPOSITORY          https://github.com/openssl/openssl.git
        GIT_TAG                 ${OPENSSL_TARGET_TAG}
        GIT_SHALLOW             TRUE
        GIT_PROGRESS            TRUE
        USES_TERMINAL_DOWNLOAD  TRUE
        USES_TERMINAL_CONFIGURE TRUE
        USES_TERMINAL_BUILD     TRUE
        USES_TERMINAL_INSTALL   TRUE
        CONFIGURE_COMMAND
            ${OpenSSL_CONFIG_COMMAND}
            --prefix=<INSTALL_DIR>/install
            --openssldir=<INSTALL_DIR>/install
            no-tests
        BUILD_COMMAND
            ${OpenSSL_MAKE_COMMAND}
        INSTALL_COMMAND
            ${OpenSSL_MAKE_COMMAND} install
        UPDATE_COMMAND          ""
        BUILD_BYPRODUCTS
            <INSTALL_DIR>/install/lib/libssl${CMAKE_STATIC_LIBRARY_SUFFIX}
            <INSTALL_DIR>/install/lib/libcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
    ExternalProject_Get_Property(OpenSSL SOURCE_DIR BINARY_DIR INSTALL_DIR)
    set(OpenSSL_INSTALL_DIR ${INSTALL_DIR}/install)
    set(OpenSSL_BINARY_DIR ${OpenSSL_INSTALL_DIR}/lib)
    set(OpenSSL_INCLUDE_DIR ${OpenSSL_INSTALL_DIR}/include)

    # Create the dir now, else INTERFACE_INCLUDE_DIRECTORIES fails
    file(MAKE_DIRECTORY ${OpenSSL_INCLUDE_DIR})

    add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
    set_target_properties(OpenSSL::SSL PROPERTIES
        IMPORTED_LOCATION ${OpenSSL_BINARY_DIR}/libssl${CMAKE_STATIC_LIBRARY_SUFFIX}
        INTERFACE_INCLUDE_DIRECTORIES ${OpenSSL_INCLUDE_DIR}
    )
    add_dependencies(OpenSSL::SSL OpenSSL)

    add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
    set_target_properties(OpenSSL::Crypto PROPERTIES
        IMPORTED_LOCATION ${OpenSSL_BINARY_DIR}/libcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}
        INTERFACE_INCLUDE_DIRECTORIES ${OpenSSL_INCLUDE_DIR}
    )
    add_dependencies(OpenSSL::Crypto OpenSSL)

endif(NOT ${OPENSSL_FOUND})
