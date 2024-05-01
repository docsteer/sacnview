# Creates AppImage, called by CPack
cmake_minimum_required(VERSION 3.15)
include(linuxdeploy)
include(GNUInstallDirs)

# Linux only!!
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "AppImages can only be created for Linux")
endif()

message(STATUS "Creating AppImage(s) for ${PROJECT_NAME}")

# Create AppDir
set(CPACK_APPIMAGE_APPDIR ${CPACK_TEMPORARY_INSTALL_DIRECTORY}/_appdir)
file(MAKE_DIRECTORY ${CPACK_APPIMAGE_APPDIR})

# Copy component libs into AppDir
foreach(COMPONENT ${CPACK_COMPONENTS_ALL})
    set(COMPONENT_LIB_DIR 
        ${CPACK_TEMPORARY_INSTALL_DIRECTORY}/${COMPONENT}/${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/)

    if(EXISTS ${COMPONENT_LIB_DIR})
        file(
            COPY ${COMPONENT_LIB_DIR}
            DESTINATION ${CPACK_APPIMAGE_APPDIR}/usr/${CMAKE_INSTALL_LIBDIR}/
            FOLLOW_SYMLINK_CHAIN)
    endif()
endforeach()

# Find binaries for project
file(GLOB CPACK_APPIMAGE_BINARIES 
    "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/${PROJECT_NAME}/${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/*")

# Run LinuxDeploy to create AppImage(s)
set(ENV{LD_LIBRARY_PATH} ${QT_LIB_DIR}:${CPACK_APPIMAGE_APPDIR}/usr/${CMAKE_INSTALL_LIBDIR})
set(ENV{QMAKE} ${QT_QMAKE_EXE})
set(ENV{VERSION} ${PROJECT_VERSION})
foreach(APPIMAGE_BINARY ${CPACK_APPIMAGE_BINARIES})
    message(STATUS "Creating AppImage for ${APPIMAGE_BINARY}")

    execute_process(
        COMMAND "${LINUXDEPLOY}" 
            "--plugin=qt"
            "--create-desktop-file"
            "--icon-file=${PROJECT_ICON}" "--icon-filename=${PROJECT_NAME}"
            "--appdir=${CPACK_APPIMAGE_APPDIR}"
            "--executable=${APPIMAGE_BINARY}"
            "--output=appimage"
    )
endforeach()