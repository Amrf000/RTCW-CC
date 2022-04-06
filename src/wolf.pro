#include(wolf.pri)

#SOURCES += \
#    main.cpp

#INSTALLS += target
TARGET = wolf
TEMPLATE = subdirs
SUBDIRS = renderer extractfuncs botlib splines ui cgame game wolf
CONFIG += ordered
DEFINES += QT_DEPRECATED_WARNINGS

