
isEmpty(TARGET_EXT) {
    win32 {
        TARGET_CUSTOM_EXT = .exe
    }
    macx {
        TARGET_CUSTOM_EXT = .app
    }
} else {
    TARGET_CUSTOM_EXT = $${TARGET_EXT}
}

win32 {
    equals($${TARGET_WINXP}, "1") {
        PRODUCT_VERSION = "$$GIT_VERSION-WindowsXP"
    } else {
        PRODUCT_VERSION = "$$GIT_VERSION"
    }

    DEPLOY_DIR = $$shell_quote($$system_path($${_PRO_FILE_PWD_}/install/deploy))
    DEPLOY_TARGET = $$shell_quote($$system_path($${OUT_PWD}/release/$${TARGET}$${TARGET_CUSTOM_EXT}))

    PRE_DEPLOY_COMMAND += $${QMAKE_DEL_FILE} $${DEPLOY_DIR}\*.* /S /Q $$escape_expand(\\n\\t)
    PRE_DEPLOY_COMMAND += $$QMAKE_COPY $${DEPLOY_TARGET} $${DEPLOY_DIR} $$escape_expand(\\n\\t)
    # OpenSSL
    PRE_DEPLOY_COMMAND += $$QMAKE_COPY $$shell_quote($$system_path($$OPENSSL_PATH/*.dll)) $${DEPLOY_DIR} $$escape_expand(\\n\\t)
    # PCap
    equals(TARGET_WINXP, 0) {
        contains(QT_ARCH, i386) {
            PRE_DEPLOY_COMMAND += $$QMAKE_COPY $$shell_quote($$system_path($${PCAP_PATH}/Bin/*)) $${DEPLOY_DIR} $$escape_expand(\\n\\t)
        } else {
            PRE_DEPLOY_COMMAND += $$QMAKE_COPY $$shell_quote($$system_path($${PCAP_PATH}/Bin/x64/*)) $${DEPLOY_DIR} $$escape_expand(\\n\\t)
        }
    }

    DEPLOY_COMMAND = windeployqt
    DEPLOY_OPT = --dir $${DEPLOY_DIR}

    DEPLOY_INSTALLER = makensis /DPRODUCT_VERSION="$${PRODUCT_VERSION}" /DTARGET_WINXP="$${TARGET_WINXP}" $$shell_quote($$system_path($${_PRO_FILE_PWD_}/install/win/install.nsi))
}
macx {
    VERSION = $$system(echo $$GIT_VERSION | sed 's/[a-zA-Z]//')

    DEPLOY_DIR = $${_PRO_FILE_PWD_}/install/mac
    DEPLOY_TARGET = $${OUT_PWD}/$${TARGET}$${TARGET_CUSTOM_EXT}

    DEPLOY_COMMAND = macdeployqt
    DEPLOY_OPT = -codesign=\"Thomas Steer\"

    DEPLOY_CLEANUP = $${QMAKE_DEL_FILE} $${DEPLOY_DIR}/sACNView*.dmg

    DEPLOY_INSTALLER = $${_PRO_FILE_PWD_}/install/mac/create-dmg --volname "sACNView_Installer" --volicon "$${_PRO_FILE_PWD_}/res/icon.icns"
    DEPLOY_INSTALLER += --background "$${_PRO_FILE_PWD_}/res/mac_install_bg.png" --window-pos 200 120 --window-size 800 400 --icon-size 100 --icon $${TARGET}$${TARGET_CUSTOM_EXT} 200 190 --hide-extension $${TARGET}$${TARGET_CUSTOM_EXT} --app-drop-link 600 185
    DEPLOY_INSTALLER += $${DEPLOY_DIR}/sACNView_$${VERSION}.dmg $${OUT_PWD}/$${TARGET}$${TARGET_CUSTOM_EXT}
}
linux {
    VERSION = $$system(echo $$GIT_VERSION | sed 's/[a-zA-Z]//')

    DEPLOY_DIR = $${_PRO_FILE_PWD_}/install/linux
    DEPLOY_TARGET = $${DEPLOY_DIR}/AppDir/$${TARGET}

    DEPLOY_COMMAND = $${OUT_PWD}/linuxdeployqt
    DEPLOY_OPT = -appimage -verbose=2

    PRE_DEPLOY_COMMAND += $${QMAKE_DEL_FILE} $${DEPLOY_DIR}/*.AppImage $$escape_expand(\\n\\t)
    PRE_DEPLOY_COMMAND += $${QMAKE_DEL_FILE} $${DEPLOY_TARGET} $$escape_expand(\\n\\t)
    PRE_DEPLOY_COMMAND += wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" -O $${DEPLOY_COMMAND} $$escape_expand(\\n\\t)
    PRE_DEPLOY_COMMAND += chmod a+x $${DEPLOY_COMMAND} $$escape_expand(\\n\\t)
##    PRE_DEPLOY_COMMAND += unset LD_LIBRARY_PATH $$escape_expand(\\n\\t)
    PRE_DEPLOY_COMMAND += $$QMAKE_COPY $${OUT_PWD}/$${TARGET} $${DEPLOY_TARGET} $$escape_expand(\\n\\t)
    PRE_DEPLOY_COMMAND += $$QMAKE_COPY $${DEPLOY_DIR}/usr/share/applications/sacnview.desktop $${DEPLOY_DIR}/AppDir/sacnview.desktop $$escape_expand(\\n\\t)
    PRE_DEPLOY_COMMAND += $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/Logo.png $${DEPLOY_DIR}/AppDir/sacnview.png $$escape_expand(\\n\\t)

    DEPLOY_CLEANUP = $$QMAKE_COPY $${OUT_PWD}/$${TARGET}*.AppImage $${DEPLOY_DIR}/

    DEPLOY_INSTALLER = $${QMAKE_DEL_FILE} $${DEPLOY_DIR}/*.deb
    DEPLOY_INSTALLER += && $${QMAKE_DEL_FILE} $${DEPLOY_DIR}/opt/sacnview/*.AppImage
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/LICENSE $${DEPLOY_DIR}/COPYRIGHT
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${DEPLOY_DIR}/$${TARGET}*.AppImage $${DEPLOY_DIR}/opt/sacnview/
    DEPLOY_INSTALLER += && ln -s $${TARGET}*.AppImage $${DEPLOY_DIR}/opt/sacnview/sACNView2.AppImage
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/icon_16.png $${DEPLOY_DIR}/usr/share/icons/hicolor/16x16/apps/sacnview.png
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/icon_24.png $${DEPLOY_DIR}/usr/share/icons/hicolor/24x24/apps/sacnview.png
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/icon_32.png $${DEPLOY_DIR}/usr/share/icons/hicolor/32x32/apps/sacnview.png
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/icon_48.png $${DEPLOY_DIR}/usr/share/icons/hicolor/48x48/apps/sacnview.png
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/icon_256.png $${DEPLOY_DIR}/usr/share/icons/hicolor/256x256/apps/sacnview.png
    DEPLOY_INSTALLER += && $$QMAKE_COPY $${_PRO_FILE_PWD_}/res/Logo.png $${DEPLOY_DIR}/usr/share/icons/hicolor/scalable/apps/sacnview.png
    DEPLOY_INSTALLER += && cd $${DEPLOY_DIR}
    DEPLOY_INSTALLER += && fpm -s dir -t deb --deb-meta-file $${DEPLOY_DIR}/COPYRIGHT --license $${LICENSE} --name $${TARGET} --version $${VERSION}  --description $${DESCRIPTION} --url $${URL} opt/ usr/
}

CONFIG( release , debug | release) {
    QMAKE_POST_LINK += $${PRE_DEPLOY_COMMAND} $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET} $${DEPLOY_OPT} $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $${DEPLOY_CLEANUP} $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $${DEPLOY_INSTALLER} $$escape_expand(\\n\\t)
}
