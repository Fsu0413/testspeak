#-------------------------------------------------
#
# Project created by QtCreator 2018-01-08T23:15:09
#
#-------------------------------------------------

QT       += core network

TEMPLATE = app
CONFIG -= app_bundle

#CONFIG += graphicsclient

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS _CRT_SECURE_NO_WARNINGS QT_NO_CAST_FROM_ASCII

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        client.cpp \
    lua/lapi.c \
    lua/lauxlib.c \
    lua/lbaselib.c \
    lua/lbitlib.c \
    lua/lcode.c \
    lua/lcorolib.c \
    lua/lctype.c \
    lua/ldblib.c \
    lua/ldebug.c \
    lua/ldo.c \
    lua/ldump.c \
    lua/lfunc.c \
    lua/lgc.c \
    lua/linit.c \
    lua/liolib.c \
    lua/llex.c \
    lua/lmathlib.c \
    lua/lmem.c \
    lua/loadlib.c \
    lua/lobject.c \
    lua/lopcodes.c \
    lua/loslib.c \
    lua/lparser.c \
    lua/lstate.c \
    lua/lstring.c \
    lua/lstrlib.c \
    lua/ltable.c \
    lua/ltablib.c \
    lua/ltm.c \
    lua/lundump.c \
    lua/lvm.c \
    lua/lzio.c \
    Ai.cpp Ai_wrap.cxx

HEADERS += \
        dialog.h client.h \
    lua/lapi.h \
    lua/lauxlib.h \
    lua/lcode.h \
    lua/lctype.h \
    lua/ldebug.h \
    lua/ldo.h \
    lua/lfunc.h \
    lua/lgc.h \
    lua/llex.h \
    lua/llimits.h \
    lua/lmem.h \
    lua/lobject.h \
    lua/lopcodes.h \
    lua/lparser.h \
    lua/lstate.h \
    lua/lstring.h \
    lua/ltable.h \
    lua/ltm.h \
    lua/lua.h \
    lua/lua.hpp \
    lua/luaconf.h \
    lua/lualib.h \
    lua/lundump.h \
    lua/lvm.h \
    lua/lzio.h \
    Ai.h

CONFIG(graphicsclient) {
    QT += gui widgets
    DEFINES += GRAPHICSCLIENT
    SOURCES += dialog.cpp
    TARGET = graphicClient
} else {
    CONFIG += console
    QT -= gui widgets
    DEFINES += CONSOLECLIENT
    SOURCES += console.cpp
    TARGET = consoleClient
}

INCLUDEPATH += $$_PRO_FILE_PWD_/lua

linux {
    android {
        DEFINES += "\"getlocaledecpoint()='.'\"" LUA_USE_POSIX
    } else {
        DEFINES += LUA_USE_LINUX
        LIBS += -ldl -lreadline
    }
}

macos {
    DEFINES += LUA_USE_MACOSX
    LIBS += -lreadline
}

ANDROID_PACKAGE_SOURCE_DIR = $$_PRO_FILE_PWD_/android

OTHER_FILES += \
    Ai.i

