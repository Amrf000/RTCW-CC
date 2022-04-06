TEMPLATE = app

DEFINES += SCREWUP
win32 {
    DEFINES -= UNICODE _UNICODE
    DEFINES += _MBCS _WIN32
}

QMAKE_CXXFLAGS += /Zc:strictStrings-

HEADERS += \
    l_log.h \
    l_memory.h \
    l_precomp.h \
    l_script.h

SOURCES += \
    extractfuncs.cc \
    l_log.cc \
    l_memory.cc \
    l_precomp.cc \
    l_script.cc

TARGET = extractfuncs
DESTDIR = $$top_builddir/../bin
