TEMPLATE = lib

DEFINES +=_WINDOWS
DEFINES +=_USRDLL
DEFINES +=UI_EXPORTS

win32 {
    DEFINES -= UNICODE _UNICODE
    DEFINES += _MBCS _WIN32
}

QMAKE_CXXFLAGS += /Zc:strictStrings- /DEF:".\ui.def"

DISTFILES += \
    ui.def

HEADERS += \
    keycodes.h \
    ui_local.h \
    ui_public.h \
    ui_shared.h

SOURCES += \
    ui_atoms.cc \
    ui_gameinfo.cc \
    ui_main.cc \
    ui_players.cc \
    ui_shared.cc \
    ui_syscalls.cc \
    ui_util.cc \
    ..\game\bg_misc.cc \
    ..\game\q_math.cc \
    ..\game\q_shared.cc

TARGET = ui_mp_x86
DESTDIR = $$top_builddir/../bin/main
