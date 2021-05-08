TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        ../../lib/c_cache.c \
        ../../lib/c_sh.c \
        ../../lib/c_sh_embed.c \
        ../../lib/c_sh_pars.c \
        ../../lib/c_tty.c \
        ../../lib/loop_buff.c \
        ../../lib/nprintf.c \
        main.c \
        test_shell.c

INCLUDEPATH += \
../../lib/

win32: {
  DEFINES += WIN32
  DEFINES += WIN_CONSOLE
}

!win32:{
    DEFINES += UNIX LINUX
    DEFINES += UNIX_CONSOLE
}


