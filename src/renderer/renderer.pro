TEMPLATE = lib
CONFIG += staticlib

QMAKE_CXXFLAGS += /Zc:strictStrings-
DEFINES += __USEA3D
DEFINES += __A3D_GEOM
win32 {
    DEFINES -= UNICODE _UNICODE
    DEFINES += _MBCS _WIN32
}

include(../jpeg-6/jpeg-6.pri)

HEADERS += \
    anorms256.h \
    qgl.h \
    qgl_linked.h \
    tr_local.h \
    tr_public.h

DISTFILES += \
    ref_trin.def

SOURCES += \
    tr_animation.cc \
    tr_backend.cc \
    tr_bsp.cc \
    tr_cmds.cc \
    tr_cmesh.cc \
    tr_curve.cc \
    tr_flares.cc \
    tr_font.cc \
    tr_image.cc \
    tr_init.cc \
    tr_light.cc \
    tr_main.cc \
    tr_marks.cc \
    tr_mesh.cc \
    tr_model.cc \
    tr_noise.cc \
    tr_scene.cc \
    tr_shade.cc \
    tr_shade_calc.cc \
    tr_shader.cc \
    tr_shadows.cc \
    tr_sky.cc \
    tr_surface.cc \
    tr_world.cc \
    ..\win32\win_gamma.cc \
    ..\win32\win_glimp.cc \
    ..\win32\win_qgl.cc


