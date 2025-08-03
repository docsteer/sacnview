
isEmpty(TARGET_EXT) {
    win32 {
        TARGET_CUSTOM_EXT = .exe
        DESTDIR = $${OUT_PWD}/bin
    }
    macx {
        TARGET_CUSTOM_EXT = .app
    }
} else {
    TARGET_CUSTOM_EXT = $${TARGET_EXT}
}

win32 {
    CONFIG += file_copies

    PRODUCT_VERSION = "$$GIT_VERSION"

    DEPLOY_DIR = $$shell_quote($$system_path($${_PRO_FILE_PWD_}/install/deploy))
    DEPLOY_TARGET = $$shell_quote($$system_path($${DESTDIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))

    mkpath($${DEPLOY_DIR})
    PRE_DEPLOY_COMMAND += $${QMAKE_DEL_FILE} $${DEPLOY_DIR}\*.* /S /Q $$escape_expand(\\n\\t)
    PRE_DEPLOY_COMMAND += $$QMAKE_COPY $${DEPLOY_TARGET} $${DEPLOY_DIR} $$escape_expand(\\n\\t)

    # OpenSSL
    PRE_DEPLOY_COMMAND += $$QMAKE_COPY $$shell_quote($$system_path($$OPENSSL_PATH/*.dll)) $${DEPLOY_DIR} $$escape_expand(\\n\\t)
    LocalDeployOpenSSL.files += $$files($${OPENSSL_PATH}/*.dll)
    LocalDeployOpenSSL.path = $$DESTDIR
    COPIES += LocalDeployOpenSSL

    # PCap
    contains(QT_ARCH, i386) {
        PCAP_BINARY_DIR = $${PCAP_PATH}/Bin
    } else {
        PCAP_BINARY_DIR = $${PCAP_PATH}/Bin/x64
    }
    PRE_DEPLOY_COMMAND += $$QMAKE_COPY $$shell_quote($$system_path($${PCAP_BINARY_DIR}/*)) $${DEPLOY_DIR} $$escape_expand(\\n\\t)
    LocalDeployPCap.files += $$files($${PCAP_BINARY_DIR}/*.dll)
    LocalDeployPCap.path = $$DESTDIR
    COPIES += LocalDeployPCap

    DEPLOY_COMMAND = $$shell_quote($$system_path($$(QTDIR)/bin/windeployqt))
    DEPLOY_OPT = --release --no-compiler-runtime --dir $${DEPLOY_DIR}

    # NSIS
    contains(QT_ARCH, i386) {
        INSTALL_NSI_FILE = win/install.nsi
    } else {
        INSTALL_NSI_FILE = win64/install.nsi
    }

    DEPLOY_INSTALLER = makensis /DPRODUCT_VERSION="$${PRODUCT_VERSION}" $$shell_quote($$system_path($${_PRO_FILE_PWD_}/install/$${INSTALL_NSI_FILE}))
}
macx {
    VERSION = $$system(echo $$GIT_VERSION | sed 's/[a-zA-Z]//')

    DEPLOY_DIR = $${_PRO_FILE_PWD_}/install/mac
    DEPLOY_TARGET = $${OUT_PWD}/$${TARGET}$${TARGET_CUSTOM_EXT}

    DEPLOY_COMMAND = macdeployqt

    # Skip while working on github runner
    # DEPLOY_CLEANUP = codesign --force --deep --verify --verbose --sign \"Thomas Steer\" $${DEPLOY_TARGET} $$escape_expand(\\n\\t)
    DEPLOY_CLEANUP += $${QMAKE_DEL_FILE} $${DEPLOY_DIR}/sACNView*.dmg

    DEPLOY_INSTALLER = create-dmg --volname "sACNView_Installer" --volicon "$${_PRO_FILE_PWD_}/res/icon.icns"
    DEPLOY_INSTALLER += --background "$${_PRO_FILE_PWD_}/res/mac_install_bg.png" --window-pos 200 120 --window-size 800 400 --icon-size 100
    DEPLOY_INSTALLER += --icon $${TARGET}$${TARGET_CUSTOM_EXT} 200 190 --hide-extension $${TARGET}$${TARGET_CUSTOM_EXT} --app-drop-link 600 185 --skip-jenkins
    DEPLOY_INSTALLER += $${DEPLOY_DIR}/sACNView_$${VERSION}.dmg $${OUT_PWD}/$${TARGET}$${TARGET_CUSTOM_EXT}
}
linux {
    VERSION = $$system(echo $$GIT_VERSION | sed 's/[a-zA-Z]//')

    DEPLOY_DIR = $${_PRO_FILE_PWD_}/install/linux

    # linuxdeploy
    DEPLOY_COMMAND = export OUTPUT=$${DEPLOY_DIR}/$${TARGET}_$${VERSION}-$${QMAKE_HOST.arch}.AppImage &&
    DEPLOY_COMMAND += $${OUT_PWD}/linuxdeploy-x86_64.AppImage
    DEPLOY_OPT += --plugin=qt
    DEPLOY_OPT += --create-desktop-file
    DEPLOY_OPT += --appdir=$${OUT_PWD}/appdir
    DEPLOY_OPT += --icon-file=$${_PRO_FILE_PWD_}/res/Logo.png
    DEPLOY_OPT += --icon-file=$${_PRO_FILE_PWD_}/res/icon_16.png
    DEPLOY_OPT += --icon-file=$${_PRO_FILE_PWD_}/res/icon_24.png
    DEPLOY_OPT += --icon-file=$${_PRO_FILE_PWD_}/res/icon_32.png
    DEPLOY_OPT += --icon-file=$${_PRO_FILE_PWD_}/res/icon_48.png
    DEPLOY_OPT += --icon-file=$${_PRO_FILE_PWD_}/res/icon_256.png
    DEPLOY_OPT += --icon-filename=$${TARGET}
    DEPLOY_OPT += --executable=$${OUT_PWD}/$${TARGET}$${TARGET_CUSTOM_EXT}
    DEPLOY_OPT += --output appimage

    ## Clean Deploy folder
    PRE_DEPLOY_COMMAND += $${QMAKE_DEL_FILE} $${DEPLOY_DIR}/*.AppImage $$escape_expand(\\n\\t)
    PRE_DEPLOY_COMMAND += $${QMAKE_DEL_FILE} $${DEPLOY_DIR}/*.deb $$escape_expand(\\n\\t)

    ## Download linuxdeploy and plugins
    PRE_DEPLOY_COMMAND += wget -c "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" $$escape_expand(\\n\\t)
    PRE_DEPLOY_COMMAND += wget -c "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" $$escape_expand(\\n\\t)
    PRE_DEPLOY_COMMAND += wget -c "https://github.com/linuxdeploy/linuxdeploy-plugin-appimage/releases/download/continuous/linuxdeploy-plugin-appimage-x86_64.AppImage" $$escape_expand(\\n\\t)
    PRE_DEPLOY_COMMAND += chmod a+x linuxdeploy-*x86_64.AppImage $$escape_expand(\\n\\t)

    ## Create Debian installer
    DEPLOY_INSTALLER = $${QMAKE_DEL_FILE} $${DEPLOY_DIR}/opt/sacnview/*.AppImage
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/LICENSE $${DEPLOY_DIR}/COPYRIGHT
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${DEPLOY_DIR}/$${TARGET}_$${VERSION}-$${QMAKE_HOST.arch}.AppImage $${DEPLOY_DIR}/opt/sacnview/
    DEPLOY_INSTALLER += && ln -s $${TARGET}_$${VERSION}-$${QMAKE_HOST.arch}.AppImage $${DEPLOY_DIR}/opt/sacnview/sACNView2.AppImage
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/icon_16.png $${DEPLOY_DIR}/usr/share/icons/hicolor/16x16/apps/sacnview.png
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/icon_24.png $${DEPLOY_DIR}/usr/share/icons/hicolor/24x24/apps/sacnview.png
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/icon_32.png $${DEPLOY_DIR}/usr/share/icons/hicolor/32x32/apps/sacnview.png
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/icon_48.png $${DEPLOY_DIR}/usr/share/icons/hicolor/48x48/apps/sacnview.png
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/icon_256.png $${DEPLOY_DIR}/usr/share/icons/hicolor/256x256/apps/sacnview.png
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/Logo.png $${DEPLOY_DIR}/usr/share/icons/hicolor/scalable/apps/sacnview.png
    DEPLOY_INSTALLER += && cd $${DEPLOY_DIR}
    DEPLOY_INSTALLER += && fpm -s dir -t deb --deb-meta-file $${DEPLOY_DIR}/COPYRIGHT --license $${LICENSE} --name $${TARGET} --version $${VERSION} --description $${DESCRIPTION} --url $${URL} opt usr
}

CONFIG( release , debug | release) {
    QMAKE_POST_LINK += $${PRE_DEPLOY_COMMAND} $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET} $${DEPLOY_OPT} $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $${DEPLOY_CLEANUP} $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $${DEPLOY_INSTALLER} $$escape_expand(\\n\\t)
}
