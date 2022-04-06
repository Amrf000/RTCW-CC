TEMPLATE = lib

DEFINES += _WINDOWS
DEFINES += CGAMEDLL
win32 {
    DEFINES -= UNICODE _UNICODE
    DEFINES += _MBCS _WIN32
}

QMAKE_CXXFLAGS += /Zc:strictStrings- /DEF:".\cgame.def"

DISTFILES += \
    cgame.def

HEADERS += \
    cg_local.h \
    cg_public.h \
    tr_types.h

SOURCES += \
    cg_consolecmds.cc \
    cg_draw.cc \
    cg_drawtools.cc \
    cg_effects.cc \
    cg_ents.cc \
    cg_event.cc \
    cg_flamethrower.cc \
    cg_info.cc \
    cg_localents.cc \
    cg_main.cc \
    cg_marks.cc \
    cg_newDraw.cc \
    cg_particles.cc \
    cg_players.cc \
    cg_playerstate.cc \
    cg_predict.cc \
    cg_scoreboard.cc \
    cg_servercmds.cc \
    cg_snapshot.cc \
    cg_sound.cc \
    cg_spawn.cc \
    cg_syscalls.cc \
    cg_trails.cc \
    cg_view.cc \
    cg_weapons.cc \
    ..\game\bg_animation.cc \
    ..\game\bg_misc.cc \
    ..\game\bg_pmove.cc \
    ..\game\bg_slidemove.cc \
    ..\game\q_math.cc \
    ..\game\q_shared.cc \
    ..\ui\ui_shared.cc

TARGET = cgame_mp_x86
DESTDIR = $$top_builddir/../bin/main
