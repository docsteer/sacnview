cmake_minimum_required(VERSION 3.18)
include(ExternalProject)

set(OPENSSL_TARGET_VERSION 1.1.1)
set(OPENSSL_TARGET_TAG "OpenSSL_1_1_1m")

find_package(OpenSSL ${OPENSSL_TARGET_VERSION} EXACT)
if(NOT ${OPENSSL_FOUND})
    # Not found, pull and compile

    # Parse version
    string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" OPENSSL_TARGET_VERSION_MAJOR "${OPENSSL_TARGET_VERSION}")
    string(REGEX REPLACE "^[0-9]+\\.([0-9])+\\.[0-9]+" "\\1" OPENSSL_TARGET_VERSION_MINOR  "${OPENSSL_TARGET_VERSION}")
    string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" OPENSSL_TARGET_VERSION_PATCH  "${OPENSSL_TARGET_VERSION}")

    # Pull and compile
    if(WIN32)
        set(OpenSSL_MAKE_COMMAND nmake)

        # Perl is needed to configure
        # OpenSSL can be picky about the brand, lets download a portable version of Strawberry
        set(STRAWBERRY_PERL_VERSION 5.32.1.1)
        if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID MATCHES "x64")
            set(STRAWBERRY_PERL_URL "https://strawberryperl.com/download/${STRAWBERRY_PERL_VERSION}/strawberry-perl-${STRAWBERRY_PERL_VERSION}-64bit-portable.zip")
            set(STRAWBERRY_PERL_URL_HASH "MD5=2fabe7ea60b5666a4102a1cd75c9c506")
        else()
            set(STRAWBERRY_PERL_URL "https://strawberryperl.com/download/${STRAWBERRY_PERL_VERSION}/strawberry-perl-${STRAWBERRY_PERL_VERSION}-32bit-portable.zip")
            set(STRAWBERRY_PERL_URL_HASH "MD5=93fdfe261588bc82ab3a0bd4f5945b60")
        endif()
        FetchContent_Declare(
            strawberry_perl
            URL                 ${STRAWBERRY_PERL_URL}
            URL_HASH            ${STRAWBERRY_PERL_URL_HASH}
            BUILD_COMMAND       ""
            CONFIGURE_COMMAND   ""
            INSTALL_COMMAND     ""
            UPDATE_COMMAND      ""
        )
        FetchContent_MakeAvailable(strawberry_perl)
        find_file(
            STRAWBERRY_PERL_EXECUTABLE
            NAMES portableshell.bat
            PATHS ${strawberry_perl_SOURCE_DIR}
            REQUIRED
            )

        # OpenSSL config command using Strawberry
        if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID MATCHES "x64")
            set(OpenSSL_CONFIG_COMMAND
                ${STRAWBERRY_PERL_EXECUTABLE} <SOURCE_DIR>/Configure VC-WIN64A-masm)
        elseif()
            set(OpenSSL_CONFIG_COMMAND
                ${STRAWBERRY_PERL_EXECUTABLE} <SOURCE_DIR>/Configure VC-WIN32)
        endif()

    else()
        set(OpenSSL_MAKE_COMMAND make)
        set(OpenSSL_CONFIG_COMMAND <SOURCE_DIR>/config)   
    endif()

    # Platform specfic config arguments
    if((UNIX) AND (NOT APPLE))
        # https://wiki.openssl.org/index.php/Compilation_and_Installation#Using_RPATHs
        set(OpenSSL_CONFIG_EXTRA_ARGS "-Wl,-rpath='$$ORIGIN' -Wl,--enable-new-dtags")
    else()
        set(OpenSSL_CONFIG_EXTRA_ARGS "")
    endif()

    # Debug build?
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        set(OpenSSL_CONFIG_BUILD_TYPE "--debug")
    else()
        set(OpenSSL_CONFIG_BUILD_TYPE "--release")
    endif()

    # Target files
    include(GNUInstallDirs)
    set(OpenSSL_SSL_IMPORTED_IMPLIB ${CMAKE_INSTALL_LIBDIR}/libssl${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(OpenSSL_CRYPTO_IMPORTED_IMPLIB ${CMAKE_INSTALL_LIBDIR}/libcrypto${CMAKE_STATIC_LIBRARY_SUFFIX})
    if(WIN32)
        # Platform filename format: libssl-1_1.dll
        set(OpenSSL_SSL_IMPORTED_LOCATION
            ${CMAKE_INSTALL_BINDIR}/libssl-${OPENSSL_TARGET_VERSION_MAJOR}_${OPENSSL_TARGET_VERSION_MINOR}${CMAKE_SHARED_LIBRARY_SUFFIX})
        set(OpenSSL_SSL_IMPORTED_SONAME
            libssl-${OPENSSL_TARGET_VERSION_MAJOR}_${OPENSSL_TARGET_VERSION_MINOR}${CMAKE_SHARED_LIBRARY_SUFFIX})
        set(OpenSSL_CRYPTO_IMPORTED_LOCATION
            ${CMAKE_INSTALL_BINDIR}/libcrypto-${OPENSSL_TARGET_VERSION_MAJOR}_${OPENSSL_TARGET_VERSION_MINOR}${CMAKE_SHARED_LIBRARY_SUFFIX})
        set(OpenSSL_CRYPTO_IMPORTED_SONAME
            libcrypto-${OPENSSL_TARGET_VERSION_MAJOR}_${OPENSSL_TARGET_VERSION_MINOR}${CMAKE_SHARED_LIBRARY_SUFFIX})
    elseif((UNIX) AND (NOT APPLE))
        # Platform filename format: libssl.so.1.1
        set(OpenSSL_SSL_IMPORTED_LOCATION
            ${CMAKE_INSTALL_LIBDIR}/libssl${CMAKE_SHARED_LIBRARY_SUFFIX}.${OPENSSL_TARGET_VERSION_MAJOR}.${OPENSSL_TARGET_VERSION_MINOR})
        set(OpenSSL_SSL_IMPORTED_SONAME
            libssl${CMAKE_SHARED_LIBRARY_SUFFIX})
        set(OpenSSL_CRYPTO_IMPORTED_LOCATION
            ${CMAKE_INSTALL_LIBDIR}/libcrypto${CMAKE_SHARED_LIBRARY_SUFFIX}.${OPENSSL_TARGET_VERSION_MAJOR}.${OPENSSL_TARGET_VERSION_MINOR})
        set(OpenSSL_CRYPTO_IMPORTED_SONAME
            libcrypto${CMAKE_SHARED_LIBRARY_SUFFIX})
    elseif(APPLE)
        # Platform filename format: libssl.1.1.dylib
        set(OpenSSL_SSL_IMPORTED_LOCATION
            ${CMAKE_INSTALL_LIBDIR}/libssl.${OPENSSL_TARGET_VERSION_MAJOR}.${OPENSSL_TARGET_VERSION_MINOR}${CMAKE_SHARED_LIBRARY_SUFFIX})
        set(OpenSSL_SSL_IMPORTED_SONAME
            libssl${CMAKE_SHARED_LIBRARY_SUFFIX})
        set(OpenSSL_CRYPTO_IMPORTED_LOCATION
            ${CMAKE_INSTALL_LIBDIR}/libcrypto.${OPENSSL_TARGET_VERSION_MAJOR}.${OPENSSL_TARGET_VERSION_MINOR}${CMAKE_SHARED_LIBRARY_SUFFIX})
        set(OpenSSL_CRYPTO_IMPORTED_SONAME
            libcrypto${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        message(FATAL_ERROR "Unsupported target platform")
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
            ${OpenSSL_CONFIG_EXTRA_ARGS}
            ${OpenSSL_CONFIG_BUILD_TYPE}
        BUILD_COMMAND
            ${OpenSSL_MAKE_COMMAND}
        INSTALL_COMMAND
            ${OpenSSL_MAKE_COMMAND} install_sw
        UPDATE_COMMAND          ""
        BUILD_BYPRODUCTS
            <INSTALL_DIR>/install/${OpenSSL_SSL_IMPORTED_LOCATION}
            <INSTALL_DIR>/install/${OpenSSL_SSL_IMPORTED_IMPLIB}
            <INSTALL_DIR>/install/${OpenSSL_CRYPTO_IMPORTED_LOCATION}
            <INSTALL_DIR>/install/${OpenSSL_CRYPTO_IMPORTED_IMPLIB}
    )
    ExternalProject_Get_Property(OpenSSL SOURCE_DIR BINARY_DIR INSTALL_DIR)
    set(OpenSSL_INSTALL_DIR ${INSTALL_DIR}/install)
    set(OpenSSL_INCLUDE_DIR ${OpenSSL_INSTALL_DIR}/include)

    # Create the dir now, else INTERFACE_INCLUDE_DIRECTORIES fails
    file(MAKE_DIRECTORY ${OpenSSL_INCLUDE_DIR})

    add_library(OpenSSL::SSL SHARED IMPORTED GLOBAL)
    set_target_properties(OpenSSL::SSL PROPERTIES
        VERSION
            ${OPENSSL_TARGET_VERSION}
        SOVERSION
            ${OPENSSL_TARGET_VERSION_MAJOR}.${OPENSSL_TARGET_VERSION_MINOR}
        IMPORTED_LOCATION
            ${OpenSSL_INSTALL_DIR}/${OpenSSL_SSL_IMPORTED_LOCATION}
        IMPORTED_SONAME
            ${OpenSSL_SSL_IMPORTED_SONAME}
        IMPORTED_IMPLIB
            ${OpenSSL_INSTALL_DIR}/${OpenSSL_SSL_IMPORTED_IMPLIB}
        INTERFACE_INCLUDE_DIRECTORIES
            ${OpenSSL_INCLUDE_DIR}
    )
    add_dependencies(OpenSSL::SSL OpenSSL)

    add_library(OpenSSL::Crypto SHARED IMPORTED GLOBAL)
    set_target_properties(OpenSSL::Crypto PROPERTIES
        VERSION
            ${OPENSSL_TARGET_VERSION}
        SOVERSION
            ${OPENSSL_TARGET_VERSION_MAJOR}.${OPENSSL_TARGET_VERSION_MINOR}
        IMPORTED_LOCATION
            ${OpenSSL_INSTALL_DIR}/${OpenSSL_CRYPTO_IMPORTED_LOCATION}
        IMPORTED_SONAME
            ${OpenSSL_CRYPTO_IMPORTED_SONAME}
        IMPORTED_IMPLIB
            ${OpenSSL_INSTALL_DIR}/${OpenSSL_CRYPTO_IMPORTED_IMPLIB}
        INTERFACE_INCLUDE_DIRECTORIES
            ${OpenSSL_INCLUDE_DIR}
    )
    add_dependencies(OpenSSL::Crypto OpenSSL)

    # Install imported libraries
    cmake_minimum_required(VERSION 3.21)
    include(GNUInstallDirs)
    install(
        IMPORTED_RUNTIME_ARTIFACTS
            OpenSSL::SSL OpenSSL::Crypto
        RUNTIME
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT OpenSSL
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT OpenSSL
    )

endif(NOT ${OPENSSL_FOUND})
