cmake_minimum_required(VERSION 3.9)

# Debian Installer
list(APPEND CPACK_GENERATOR "DEB")

# Auto generate package dependency list and add required platform plugins
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt${QT_VERSION_MAJOR}-qpa-plugins")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)

# The Debian package maintainer
set(CPACK_DEBIAN_PACKAGE_MAINTAINER 
    "Marcus Birkin <marcus.birkin@gmail.com>")

# Quick description
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION 
    "A tool for viewing, monitoring, and testing; the ANSI/ESTA E1.31 protocol, informally known as 'Streaming ACN'")

# It's a network tool
set(CPACK_DEBIAN_PACKAGE_SECTION "net")

# Deb Filename
# DEB-DEFAULT = <PackageName>_<VersionNumber>-<DebianRevisionNumber>_<DebianArchitecture>.deb
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)