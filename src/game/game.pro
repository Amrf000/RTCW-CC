TEMPLATE = lib

DEFINES +=_WINDOWS
DEFINES +=BUILDING_REF_GL
DEFINES +=GAMEDLL
win32 {
    DEFINES -= UNICODE _UNICODE
    DEFINES += _MBCS _WIN32
}

QMAKE_CXXFLAGS += /Zc:strictStrings- /DEF:".\game.def"

DISTFILES += \
    game.def

include(../botai/botai.pri)

HEADERS += \
    ai_cast.h \
    ai_cast_fight.h \
    ai_cast_global.h \
    be_aas.h \
    be_ai_char.h \
    be_ai_chat.h \
    be_ai_gen.h \
    be_ai_goal.h \
    be_ai_move.h \
    be_ai_weap.h \
    be_ea.h \
    bg_local.h \
    bg_public.h \
    botlib.h \
    g_local.h \
    g_public.h \
    g_team.h \
    q_shared.h \
    surfaceflags.h

SOURCES += \
    g_vehicles.cc \
    q_math.cc \
    q_shared.cc \
    ai_cast.cc \
    ai_cast_characters.cc \
    ai_cast_debug.cc \
    ai_cast_events.cc \
    ai_cast_fight.cc \
    ai_cast_func_attack.cc \
    ai_cast_func_boss1.cc \
    ai_cast_funcs.cc \
    ai_cast_script.cc \
    ai_cast_script_actions.cc \
    ai_cast_script_ents.cc \
    ai_cast_sight.cc \
    ai_cast_think.cc \
    bg_animation.cc \
    bg_misc.cc \
    bg_pmove.cc \
    bg_slidemove.cc \
    g_active.cc \
    g_alarm.cc \
    g_antilag.cc \
    g_bot.cc \
    g_client.cc \
    g_cmds.cc \
    g_combat.cc \
    g_items.cc \
    g_main.cc \
    g_mem.cc \
    g_misc.cc \
    g_missile.cc \
    g_mover.cc \
    g_props.cc \
    g_save.cc \
    g_script.cc \
    g_script_actions.cc \
    g_session.cc \
    g_spawn.cc \
    g_svcmds.cc \
    g_syscalls.cc \
    g_target.cc \
    g_team.cc \
    g_tramcar.cc \
    g_trigger.cc \
    g_utils.cc \
    g_weapon.cc

TARGET = qagame_mp_x86
DESTDIR = $$top_builddir/../bin/main

