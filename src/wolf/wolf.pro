
win32 {
    DEFINES -= UNICODE _UNICODE
    DEFINES += _MBCS _WIN32
}

QMAKE_CXXFLAGS += /Zc:strictStrings-
DEFINES +=_WINDOWS
DEFINES +=__USEA3D
DEFINES +=__A3D_GEOM
LIBS += $$top_builddir/renderer/$(OBJECTS_DIR)/renderer.lib
PRE_TARGETDEPS += $$top_builddir/renderer/$(OBJECTS_DIR)/renderer.lib
LIBS += $$top_builddir/botlib/$(OBJECTS_DIR)/botlib.lib
PRE_TARGETDEPS += $$top_builddir/botlib/$(OBJECTS_DIR)/botlib.lib
LIBS += $$top_builddir/splines/$(OBJECTS_DIR)/splines.lib
PRE_TARGETDEPS += $$top_builddir/splines/$(OBJECTS_DIR)/splines.lib

win32:LIBS += -lAdvapi32 -lgdi32 -luser32 -lshell32 -lwinmm -lwsock32

include(wolf.pri)
include(../qcommon/qcommon.pri)
include(../server/server.pri)
#include(../null/null.pri)
include(../client/client.pri)
include(../win32/win32.pri)

SOURCES += \
    ../game/q_math.cc \
    ../game/q_shared.cc

INSTALLS += target

TARGET = WolfMP
DESTDIR = $$top_builddir/../bin

