# Download helper function
function(download_exe url target)
    # Download, if needed
    if(NOT EXISTS ${target})
        message("Downloading ${target}")
        file(DOWNLOAD 
            ${url}
            ${target}
            SHOW_PROGRESS
        )
    else()
        message("Skipping download of ${target}")
    endif()

    # Make executable
    file(
        CHMOD ${target}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    )
endfunction()

# Download linuxdeploy
message(STATUS "Downloading linuxdeploy")
download_exe(
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    "linuxdeploy-x86_64.AppImage"
)
download_exe(
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    "linuxdeploy-plugin-qt-x86_64.AppImage"
)
download_exe(
    "https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage"
    "linuxdeploy-plugin-appimage-x86_64.AppImage"
)
get_filename_component(
   LINUXDEPLOY "linuxdeploy-x86_64.AppImage"
   REALPATH
)