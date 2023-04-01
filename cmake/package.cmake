cmake_minimum_required(VERSION 3.19)

# Locate QT Binary directory
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
get_target_property(QT_QMAKE_EXE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXE}" DIRECTORY)
file(REAL_PATH ${QT_BIN_DIR}/../lib/ QT_LIB_DIR)

# Variables common to all CPack Generators
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "sacnview.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME} ${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://sacnview.org")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CPACK_PACKAGE_ICON "${${PROJECT_NAME}_ICON_PNG}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CPACK_PACKAGE_ICON "${${PROJECT_NAME}_ICON_SVG}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CPACK_PACKAGE_ICON "${${PROJECT_NAME}_ICON_ICNS}")
endif()

# Set packing scripts
set(PROJECT_ICON ${CPACK_PACKAGE_ICON})
set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/package_prebuild.cmake")

# Strip binaries when packing
set(CPACK_STRIP_FILES TRUE)

# Select CPack Generator
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Create an NSIS installer
    # include(package_nsis)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Install into /opt/sacnview
    string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${PROJECT_NAME_LOWER}")

    # Create a debian installer
    include(package_deb)
    # ...and AppImage
    include(package_appimage)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Create a drag'n'drop dmg image
    # include(package_dmg)
else()
    message(SEND_ERROR "Unsupported platform for packer")
endif()
include(CPack)

# Config Packing scripts
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/package_prebuild.cmake.in" 
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/package_prebuild.cmake"
    @ONLY
)

#--Main Application
cpack_add_component(
    ${PROJECT_NAME}
    DISPLAY_NAME ${PROJECT_NAME}
    DESCRIPTION "Main Application"
    DEPENDS "OpenSSL"
    REQUIRED
)

#--System Libraries (i.e. MSVC)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT "SYSTEM_RUNTIME")
    include(InstallRequiredSystemLibraries)
    cpack_add_component(
        "SYSTEM_RUNTIME"
        DISPLAY_NAME "Required System Libraries"
        HIDDEN
        REQUIRED
    )
endif()