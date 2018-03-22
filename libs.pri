# Firewall Checker
win32 {
    LIBS += -lole32 -loleaut32
}

#PCap/WinPcap
win32 {
    # Not for Windows XP Build
    equals(TARGET_WINXP, 0) {
        PCAP_PATH = $${_PRO_FILE_PWD_}/libs/WinPcap-413-173-b4
        contains(QT_ARCH, i386) {
            LIBS += -L$${PCAP_PATH}/lib
        } else {
            LIBS += -L$${PCAP_PATH}/lib/x64
        }
        LIBS += -lwpcap -lPacket
        INCLUDEPATH += $${PCAP_PATH}/Include

        SOURCES += src/pcapplayback.cpp \
            src/pcapplaybacksender.cpp
        HEADERS += src/pcapplayback.h \
            src/pcapplaybacksender.h
        FORMS += ui/pcapplayback.ui
    }
}
!win32 {
    LIBS += -lpcap

    SOURCES += src/pcapplayback.cpp \
        src/pcapplaybacksender.cpp
    HEADERS += src/pcapplayback.h \
        src/pcapplaybacksender.h
    FORMS += ui/pcapplayback.ui
}

# OpenSSL
win32 {
    # https://wiki.openssl.org/index.php/Binaries
    equals(QT_MAJOR_VERSION, 5):equals(QT_MINOR_VERSION, 6) { #https://wiki.qt.io/Qt_5.6_Tools_and_Versions
        OPENSSL_VERS = 1.0.2g
    }
    equals(QT_MAJOR_VERSION, 5):equals(QT_MINOR_VERSION, 9) { #https://wiki.qt.io/Qt_5.9_Tools_and_Versions
        OPENSSL_VERS = 1.0.2j
    }
    equals(QT_MAJOR_VERSION, 5):equals(QT_MINOR_VERSION, 10) { #https://wiki.qt.io/Qt_5.10_Tools_and_Versions
        OPENSSL_VERS = 1.0.2j
    }
    contains(QT_ARCH, i386) {
        OPENSSL_PATH = $${_PRO_FILE_PWD_}/libs/openssl-$${OPENSSL_VERS}-i386-win32
    } else {
        OPENSSL_PATH = $${_PRO_FILE_PWD_}/libs/openssl-$${OPENSSL_VERS}-x64_86-win64
    }
}