cmake_minimum_required(VERSION 3.17)

# NSIS Installer
list(APPEND CPACK_GENERATOR "NSIS")

# Icon
set(CPACK_NSIS_MUI_ICON ${CPACK_PACKAGE_ICON})

# Ask to uninstall previous versions first
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

# If these arn't set, then the version nummber is appended
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY ${CPACK_PACKAGE_NAME})

# Start menu items
list(APPEND CPACK_PACKAGE_EXECUTABLES ${PROJECT_NAME} ${PROJECT_NAME})
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")

# Run app on finish
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${PROJECT_NAME}.exe")