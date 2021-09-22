## global defintions : target lib name, version
TARGET = SolARPipeline_MapUpdate_Remote
VERSION = 0.10.0

QMAKE_PROJECT_DEPTH = 0

## remove Qt dependencies
QT     -= core gui
CONFIG -= app_bundle qt
CONFIG += c++1z
CONFIG += console
CONFIG += verbose
CONFIG -= qt

DEFINES += MYVERSION=\"\\\"$${VERSION}\\\"\"
DEFINES += WITHREMOTING

include(findremakenrules.pri)

CONFIG(debug,debug|release) {
    TARGETDEPLOYDIR = $${PWD}/../bin/Debug
    DEFINES += _DEBUG=1
    DEFINES += DEBUG=1
}

CONFIG(release,debug|release) {
    TARGETDEPLOYDIR = $${PWD}/../bin/Release
    DEFINES += NDEBUG=1
}

win32:CONFIG -= static
win32:CONFIG += shared
QMAKE_TARGET.arch = x86_64 #must be defined prior to include
DEPENDENCIESCONFIG = shared install_recurse
PROJECTCONFIG = QTVS

#NOTE : CONFIG as staticlib or sharedlib,  DEPENDENCIESCONFIG as staticlib or sharedlib, QMAKE_TARGET.arch and PROJECTDEPLOYDIR MUST BE DEFINED BEFORE templatelibconfig.pri inclusion
include ($$shell_quote($$shell_path($${QMAKE_REMAKEN_RULES_ROOT}/templateappconfig.pri)))  # Shell_quote & shell_path required for visual on windows

HEADERS += \
    GrpcServerManager.h


SOURCES += \
    GrpcServerManager.cpp\
    SolARPipeline_MapUpdate_Remote.cpp

unix {
    LIBS += -ldl
    QMAKE_CXXFLAGS += -DBOOST_LOG_DYN_LINK
}

linux {
    LIBS += -ldl
    LIBS += -L/home/linuxbrew/.linuxbrew/lib # temporary fix caused by grpc with -lre2 ... without -L in grpc.pc
}


macx {
    DEFINES += _MACOS_TARGET_
    QMAKE_MAC_SDK= macosx
    QMAKE_CFLAGS += -mmacosx-version-min=10.7 #-x objective-c++
    QMAKE_CXXFLAGS += -mmacosx-version-min=10.7  -std=c++17 -fPIC#-x objective-c++
    QMAKE_LFLAGS += -mmacosx-version-min=10.7 -v -lstdc++
    LIBS += -lstdc++ -lc -lpthread
    LIBS += -L/usr/local/lib
}

win32 {
    QMAKE_LFLAGS += /MACHINE:X64
    DEFINES += WIN64 UNICODE _UNICODE
    QMAKE_COMPILER_DEFINES += _WIN64

    # Windows Kit (msvc2013 64)
    LIBS += -L$$(WINDOWSSDKDIR)lib/winv6.3/um/x64 -lshell32 -lgdi32 -lComdlg32
    INCLUDEPATH += $$(WINDOWSSDKDIR)lib/winv6.3/um/x64
}

linux {
  start_service_install.path = $${TARGETDEPLOYDIR}
  start_service_install.files = $${PWD}/../start_service.sh
  CONFIG(release,debug|release) {
    start_service_install.extra = cp $$files($${PWD}/../start_serviceRelease.sh) $${PWD}/../start_service.sh
  }
  CONFIG(debug,debug|release) {
    start_service_install.extra = cp $$files($${PWD}/../start_serviceDebug.sh) $${PWD}/../start_service.sh
  }
  INSTALLS += start_service_install
}

DISTFILES += \
    SolARPipeline_MapUpdate_Remote_modules.xml \
    SolARPipeline_MapUpdate_Remote_modules.xml \
    SolARPipeline_MapUpdate_Remote_properties.xml \
    SolARPipeline_MapUpdate_Remote_properties.xml \
    packagedependencies.txt

xml_files.path = $${TARGETDEPLOYDIR}
xml_files.files =  SolARPipeline_MapUpdate_Remote_modules.xml \
                   SolARPipeline_MapUpdate_Remote_properties.xml

INSTALLS += xml_files

