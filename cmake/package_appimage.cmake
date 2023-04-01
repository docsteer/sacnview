cmake_minimum_required(VERSION 3.13)

# AppImage Using external script
if ("External" IN_LIST CPACK_GENERATOR)
    message(FATAL_ERROR "CPack is already using an external generator, can not continue with AppImage")
endif()
list(APPEND CPACK_GENERATOR "External")

# Create temporary staging area 
set(CPACK_EXTERNAL_ENABLE_STAGING TRUE)

# Appimage packing script
set(CPACK_EXTERNAL_PACKAGE_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/package_appimage_generator.cmake")