TEMPLATE = lib
CONFIG += staticlib

DEFINES += BOTLIB
QMAKE_CXXFLAGS += /Zc:strictStrings-

HEADERS += \
    aasfile.h \
    be_aas_bsp.h \
    be_aas_cluster.h \
    be_aas_debug.h \
    be_aas_def.h \
    be_aas_entity.h \
    be_aas_file.h \
    be_aas_funcs.h \
    be_aas_main.h \
    be_aas_move.h \
    be_aas_optimize.h \
    be_aas_reach.h \
    be_aas_route.h \
    be_aas_routealt.h \
    be_aas_routetable.h \
    be_aas_sample.h \
    be_ai_weight.h \
    be_interface.h \
    l_crc.h \
    l_libvar.h \
    l_log.h \
    l_memory.h \
    l_precomp.h \
    l_script.h \
    l_struct.h \
    l_utils.h

SOURCES += \
    be_aas_bspq3.cc \
    be_aas_cluster.cc \
    be_aas_debug.cc \
    be_aas_entity.cc \
    be_aas_file.cc \
    be_aas_main.cc \
    be_aas_move.cc \
    be_aas_optimize.cc \
    be_aas_reach.cc \
    be_aas_route.cc \
    be_aas_routealt.cc \
    be_aas_routetable.cc \
    be_aas_sample.cc \
    be_ai_char.cc \
    be_ai_chat.cc \
    be_ai_gen.cc \
    be_ai_goal.cc \
    be_ai_move.cc \
    be_ai_weap.cc \
    be_ai_weight.cc \
    be_ea.cc \
    be_interface.cc \
    l_crc.cc \
    l_libvar.cc \
    l_log.cc \
    l_memory.cc \
    l_precomp.cc \
    l_script.cc \
    l_struct.cc


