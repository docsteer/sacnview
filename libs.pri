LIBS_PATH = $$system_path($${_PRO_FILE_PWD_}/libs)

# Breakpad
BREAKPAD_PATH = $$system_path($${LIBS_PATH}/breakpad)
DEPOT_TOOLS_PATH = $$system_path($${_PRO_FILE_PWD_}/tools/depot_tools)
INCLUDEPATH += $$system_path($${BREAKPAD_PATH}/src/src)
linux {
    DEFINES += USE_BREAKPAD

    # Fetch breakpad if required
    system([ ! -d $$shell_quote($${BREAKPAD_PATH}) ] && mkdir $$shell_quote($${BREAKPAD_PATH}) && cd $$shell_quote($${BREAKPAD_PATH}) && PATH=$${DEPOT_TOOLS_PATH}:$PATH fetch breakpad)
    # Update Breakpad
    system(cd $$shell_quote($${BREAKPAD_PATH}) && PATH=$${DEPOT_TOOLS_PATH}:$PATH gclient sync)
    # Build
    system(cd $$shell_quote($${BREAKPAD_PATH}/src) && ./configure && make)

    LIBS += -L$${BREAKPAD_PATH}/src/src/client/linux -lbreakpad_client

    HEADERS += src/crash_handler.h \
        src/ui/crash_test.h
    SOURCES += src/crash_handler.cpp \
        src/ui/crash_test.cpp
    FORMS += ui/crash_test.ui
}
win32 {
    DEFINES += USE_BREAKPAD

    # Config breakpad if required
    system(if not exist $$shell_quote($${BREAKPAD_PATH}) ( mkdir $$shell_quote($${BREAKPAD_PATH}) && cd $$shell_quote($${BREAKPAD_PATH}) && $$shell_quote($${DEPOT_TOOLS_PATH}\fetch.bat) breakpad))
    # Update Breakpad
    system(cd $$shell_quote($${BREAKPAD_PATH}) && $$shell_quote($${DEPOT_TOOLS_PATH}\gclient.bat) sync)

    LIBS += -luser32
    INCLUDEPATH  += {BREAKPAD_PATH}/src/src/
    HEADERS += $${BREAKPAD_PATH}/src/src/common/windows/string_utils-inl.h \
        $${BREAKPAD_PATH}/src/src/common/windows/guid_string.h \
        $${BREAKPAD_PATH}/src/src/client/windows/handler/exception_handler.h \
        $${BREAKPAD_PATH}/src/src/client/windows/common/ipc_protocol.h \
        $${BREAKPAD_PATH}/src/src/google_breakpad/common/minidump_format.h \
        $${BREAKPAD_PATH}/src/src/google_breakpad/common/breakpad_types.h \
        $${BREAKPAD_PATH}/src/src/client/windows/crash_generation/crash_generation_client.h \
        $${BREAKPAD_PATH}/src/src/common/scoped_ptr.h \
        src/crash_handler.h \
        src/ui/crash_test.h
    SOURCES += $${BREAKPAD_PATH}/src/src/client/windows/handler/exception_handler.cc \
        $${BREAKPAD_PATH}/src/src/common/windows/string_utils.cc \
        $${BREAKPAD_PATH}/src/src/common/windows/guid_string.cc \
        $${BREAKPAD_PATH}/src/src/client/windows/crash_generation/crash_generation_client.cc \
        src/crash_handler.cpp \
        src/ui/crash_test.cpp
    FORMS += ui/crash_test.ui
}
macx {
    # Breakpad is disabled for MacOS as it has been superceded by Crashpad
    # see https://groups.google.com/a/chromium.org/forum/#!topic/chromium-dev/6eouc7q2j_g
    DEFINES -= USE_BREAKPAD
}

# Firewall Checker
win32 {
    LIBS += -lole32 -loleaut32
}

#PCap/WinPcap
win32 {
        PCAP_PATH = $${_PRO_FILE_PWD_}/libs/WinPcap-413-173-b4
        contains(QT_ARCH, i386) {
            LIBS += -L$${PCAP_PATH}/lib
        } else {
            LIBS += -L$${PCAP_PATH}/lib/x64
        }
        LIBS += -lwpcap -lPacket
        INCLUDEPATH += $${PCAP_PATH}/Include

        INCLUDEPATH += src/pcap/
        SOURCES += src/pcap/pcapplayback.cpp \
            src/pcap/pcapplaybacksender.cpp
        HEADERS += src/pcap/pcapplayback.h \
            src/pcap/pcapplaybacksender.h \
            src/pcap/ethernetstrut.h
        FORMS += ui/pcapplayback.ui
}
!win32 {
    LIBS += -lpcap

    INCLUDEPATH += src/pcap/
    SOURCES += src/pcap/pcapplayback.cpp \
        src/pcap/pcapplaybacksender.cpp
    HEADERS += src/pcap/pcapplayback.h \
        src/pcap/pcapplaybacksender.h \
        src/pcap/ethernetstrut.h
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
    equals(QT_MAJOR_VERSION, 5):equals(QT_MINOR_VERSION, 11) { #https://wiki.qt.io/Qt_5.11_Tools_and_Versions
        OPENSSL_VERS = 1.0.2j
    }
    equals(QT_MAJOR_VERSION, 5):equals(QT_MINOR_VERSION, 12) { #https://wiki.qt.io/Qt_5.12_Tools_and_Versions
        OPENSSL_VERS = 1.1.1b
    }
    equals(QT_MAJOR_VERSION, 5):equals(QT_MINOR_VERSION, 15) { #https://wiki.qt.io/Qt_5.15_Tools_and_Versions
        OPENSSL_VERS = 1.1.1g
    }
    equals(QT_MAJOR_VERSION, 6):equals(QT_MINOR_VERSION, 2) { #https://wiki.qt.io/Qt_6.2_Tools_and_Versions
        OPENSSL_VERS = 1.1.1q
    }
    contains(QT_ARCH, i386) {
        OPENSSL_PATH = $${_PRO_FILE_PWD_}/libs/openssl-$${OPENSSL_VERS}-win32
    } else {
        OPENSSL_PATH = $${_PRO_FILE_PWD_}/libs/openssl-$${OPENSSL_VERS}-win64
    }
}

# Blake2
contains(QT_ARCH, x86_64) {
    # SSE Implementation
    DEFINES += HAVE_SSE2
    BLAKE2_PATH = $$system_path($${LIBS_PATH}/blake2/sse)
    HEADERS += \
        $${BLAKE2_PATH}/blake2b-load-sse2.h \
        $${BLAKE2_PATH}/blake2b-load-sse41.h \
        $${BLAKE2_PATH}/blake2b-round.h \
        $${BLAKE2_PATH}/blake2-config.h \
        $${BLAKE2_PATH}/blake2s-load-sse2.h \
        $${BLAKE2_PATH}/blake2s-load-sse41.h \
        $${BLAKE2_PATH}/blake2s-load-xop.h \
        $${BLAKE2_PATH}/blake2s-round.h
    SOURCES += \
        $${BLAKE2_PATH}/blake2b.c \
        $${BLAKE2_PATH}/blake2bp.c \
        $${BLAKE2_PATH}/blake2s.c \
        $${BLAKE2_PATH}/blake2sp.c \
        $${BLAKE2_PATH}/blake2xb.c \
        $${BLAKE2_PATH}/blake2xs.c
} else {
    # Standard Implimentation
    BLAKE2_PATH = $$system_path($${LIBS_PATH}/blake2/ref)
    SOURCES += \
        $${BLAKE2_PATH}/blake2bp-ref.c \
        $${BLAKE2_PATH}/blake2b-ref.c \
        $${BLAKE2_PATH}/blake2sp-ref.c \
        $${BLAKE2_PATH}/blake2s-ref.c \
        $${BLAKE2_PATH}/blake2xb-ref.c \
        $${BLAKE2_PATH}/blake2xs-ref.c
}
INCLUDEPATH  += $${BLAKE2_PATH}
HEADERS += \
    $${BLAKE2_PATH}/blake2.h
    $${BLAKE2_PATH}/blake2-impl.h
