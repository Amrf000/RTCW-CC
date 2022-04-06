TEMPLATE = lib
CONFIG += staticlib

QMAKE_CXXFLAGS += /Zc:strictStrings-
win32 {
    DEFINES -= UNICODE _UNICODE
    DEFINES += _MBCS _WIN32
}


HEADERS += \
    math_angles.h \
    math_matrix.h \
    math_quaternion.h \
    math_vector.h \
    q_splineshared.h \
    splines.h \
    util_list.h \
    util_str.h

SOURCES += \
    math_angles.cpp \
    math_matrix.cpp \
    math_quaternion.cpp \
    math_vector.cpp \
    q_parse.cpp \
    q_shared.cpp \
    splines.cpp \
    util_str.cpp
