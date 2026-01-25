# sACNView ![Build Status](https://github.com/docsteer/sacnview/actions/workflows/build.yml/badge.svg)

sACNView is a tool for viewing, monitoring and testing the ANSI/ESTA E1.31 protcol, informally known as "Streaming ACN".

For general information on the project, visit the [web site](http://docsteer.github.io/sacnview/)

This protocol is used in lighting systems (typically theatre, television, live entertainment and other systems where a high performance stream of data is required).

This is sACNView mark 2. The original program was written in Borland Delphi, but this new version supplants it. It is written in C++, using the Qt library and sACN reception code from [Electronic Theatre Controls](http://www.etcconnect.com).

# Installation
If you are interested in simply using the program, visit the [product web site](http://docsteer.github.io/sacnview/) to download installers for your platform (Linux, Mac or Windows OS).

# Building
If you are interested in building the application you will need the following:

* CMake
* Qt Library version 6.8.2 or higher https://www.qt.io/
* For Windows:
	* Visual Studio 2022 - Community edition is fine
    * NSIS - http://nsis.sourceforge.net/ and the [SimpleFW NSIS Plugin](http://nsis.sourceforge.net/NSIS_Simple_Firewall_Plugin)
		* If NSIS is included in your $PATH the installer will be built automatically

The application uses a couple of external libraries:

* Google Breakpad for crash reporting
* LibPCAP for captured packet playback

You need to acquire the Breakpad library by using the git `submodule` command

1. Clone the repository (`git clone https://github.com/docsteer/sacnview.git`)
2. Switch to the directory (`cd sacnview`)
3. Initialize the submodules (`git submodule init`)
4. Download the submodule (`git submodule update`)

Then, to build:

## Windows

Set an environment variable, QT_ROOT_DIR, pointing to the root directory of your Qt installation, including the version and compiler parts, so for example `C:\Qt\6.8.2\msvc2022_64`.

Start a developer command prompt by running the `x64 Native Tools Command Prompt for VS 2022` shortcut

**Note**: Do not start the x86 or unlabelled version, this will lead to errors about being unable to find Qt.

Configure the application for the appropriate target using a CMake preset, e.g:

    cmake --preset "windows-x64-release-installer"

Once configuration is successful, you can run the build by navigating to the directory in the out folder and running

	cd out\build\windows-x64-release-installer
    cmake --build .

## Linux

Note that we use a newer version of Qt than is provided by many distributions, so recommend using the Qt installer tool to install a recent LTS version.

Set an environment variable, QT_ROOT_DIR, pointing to the root directory of your Qt installation, including the version and compiler parts, for example:

    export QT_ROOT_DIR=~/Qt/6.8.3/gcc_64

Configure the application for the appropriate target using a CMake preset, e.g:

    cmake --preset "linux-x64-release"

Once configuration is successful, you can run the build by navigating to the directory in the out folder and running

	cd out\build\linux-x64-release
    cmake --build .

The build can be packaged as either an AppImage or a DEB package:

### AppImage

To build an AppImage, you will need the linuxdeploy utility, which is itself an appimage. Obtain it and locate it in your path.

Then from the root of the repository run the script `install/linux/build_appimage.sh`. This will run the build and then use the linuxdeploy tool to build the AppImage.

### Debian Package (DEB)

To build a DEB package, run the command `cpack --package "linux"`. This will generate the deb package under the output directory.
