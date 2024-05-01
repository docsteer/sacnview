# Prebuild for CPackDeb
if(${CPACK_GENERATOR} MATCHES "DEB")
    cmake_minimum_required(VERSION 3.21)

    # Desktop file...
    set(CPACK_DEBIAN_DESKTOPFILE_DIR ${CPACK_TEMPORARY_INSTALL_DIRECTORY}/usr/share/applications/)
    file(MAKE_DIRECTORY ${CPACK_DEBIAN_DESKTOPFILE_DIR})
    file(
        COPY 
            ${PROJECT_SOURCE_DIR}/install/linux/${PROJECT_NAME}.desktop
        DESTINATION
            ${CPACK_DEBIAN_DESKTOPFILE_DIR}
    )
    file(
        CHMOD_RECURSE
            ${CPACK_DEBIAN_DESKTOPFILE_DIR}
        FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE
            GROUP_READ 
            WORLD_READ
    )

    #... and icon
    set(CPACK_DEBIAN_ICON_DIR ${CPACK_TEMPORARY_INSTALL_DIRECTORY}/usr/share/icons/hicolor/scalable/apps/)
    file(MAKE_DIRECTORY ${CPACK_DEBIAN_ICON_DIR})
    file(
        COPY_FILE 
            ${PROJECT_ICON}
            ${CPACK_DEBIAN_ICON_DIR}/${PROJECT_NAME}.svg
    )
    file(
        CHMOD_RECURSE
            ${CPACK_DEBIAN_ICON_DIR}
        FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE
            GROUP_READ 
            WORLD_READ
    )

    # Directory permissions
    file(
        CHMOD_RECURSE 
            ${CPACK_TEMPORARY_INSTALL_DIRECTORY}
        DIRECTORY_PERMISSIONS 
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE 
            WORLD_READ WORLD_EXECUTE
    )
endif(${CPACK_GENERATOR} MATCHES "DEB")