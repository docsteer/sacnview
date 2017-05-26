# sacnview
sACNView is a tool for viewing, monitoring and testing the ANSI/ESTA E1.17 protcol, informally known as "Streaming ACN".

This protcol is used in lighting systems (typically theatre, television, live entertainment and other systems where a high performance stream of data is required).

This is sACNView mark 2. The original program was written in Borland Delphi, but this new version supplants it.

#Installation
If you are interested in simply using the program, visit the download page to download installers for your platform (Mac or Windows OS).

#Building
If you are interested in building the application you will need the following:
* Qt Creator - https://www.qt.io/ide/
* To build the windows installer - NSIS - http://nsis.sourceforge.net/

To build, open the .PRO file in the root directory and build with Qt Creator. To build the windows installer, compile the NSIS file in the install/win directory.
