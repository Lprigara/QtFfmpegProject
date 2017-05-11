#-------------------------------------------------
#
# Project created by QtCreator 2017-02-20T11:44:04
#
#-------------------------------------------------

QT       += core gui
QT += multimedia sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qtFfmpegProject
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        mainwindow.cpp \
    videodecoder.cpp \
    videoinfo.cpp \
    utilities.cpp \
    videoencoder.cpp \
    videocutter.cpp \
    audioextractor.cpp \
    videoscaler.cpp \
    audiofilter.cpp \
    videoextractor.cpp \
    watermark.cpp

HEADERS  += mainwindow.h \
    videodecoder.h \
    videoinfo.h \
    utilities.h \
    videoencoder.h \
    videocutter.h \
    audioextractor.h \
    videoscaler.h \
    audiofilter.h \
    videoextractor.h \
    watermark.h

FORMS    += mainwindow.ui

QMAKE_CXXFLAGS += -D__STDC_CONSTANT_MACROS

INCLUDEPATH += $$PWD/ffmpeg_build/include \
$$PWD/libao-1.2.0/libraries/include\
/home/leonor/Descargas/SDL-1.2.15/include

LIBS += -pthread
LIBS += -L$$PWD/ffmpeg_build/lib
LIBS += -lavdevice
LIBS += -lavfilter
LIBS += -lavformat
LIBS += -lavcodec
LIBS += -lavresample
LIBS += -ldl
LIBS += -lXfixes
LIBS += -lXext
LIBS += -lasound
LIBS += -lSDL
LIBS += -lvorbisenc
LIBS += -lvorbis
LIBS += -ltheoraenc
LIBS += -ltheoradec
LIBS += -logg
LIBS += -lmp3lame
LIBS += -lz
LIBS += -lrt
LIBS += -lswresample
LIBS += -lswscale
LIBS += -lavutil
LIBS += -lfreetype
LIBS += -lpostproc
LIBS += -lass
LIBS += -lm
LIBS += -lvdpau
LIBS += -lva
LIBS += -lX11
LIBS += -lva-drm
LIBS += -lva-x11
LIBS += -L$$PWD/libao-1.2.0/libraries/lib
LIBS += -lao

RESOURCES += \
    recursos.qrc
