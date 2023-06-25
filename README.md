# sACNView ![Build Status](https://github.com/docsteer/sacnview/actions/workflows/build.yml/badge.svg)

sACNView is a tool for viewing, monitoring and testing the ANSI/ESTA E1.31 protcol, informally known as "Streaming ACN".

For general information on the project, visit the [web site](http://docsteer.github.io/sacnview/)

This protocol is used in lighting systems (typically theatre, television, live entertainment and other systems where a high performance stream of data is required).

This is sACNView mark 2. The original program was written in Borland Delphi, but this new version supplants it. It is written in C++, using the Qt library and sACN reception code from [Electronic Theatre Controls](http://www.etcconnect.com).

# Installation
If you are interested in simply using the program, visit the [product web site](http://docsteer.github.io/sacnview/) to download installers for your platform (Linux, Mac or Windows OS).

# Building
If you are interested in building the application you will need the following:

* Qt Library and Creator - https://www.qt.io/ide/
	* Note : Due to [this bug](https://bugreports.qt.io/browse/QTBUG-27641) in Qt you need to use Qt version 5.9.0 or higher to build sACNView. 
* CMake - https://cmake.org
* To build the windows installer - NSIS - http://nsis.sourceforge.net/

The application uses a few external libraries:

* Blake2 for Pathway Secure Protocol
* Google Breakpad for crash reporting
* LibPCAP (or Npacp/WinPcap on Windows) for captured packet playback
* OpenSSL for secure web access during updated version checking
* ZLib for checksum generation