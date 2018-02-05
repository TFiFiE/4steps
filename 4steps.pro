QT       += core gui svg multimedia network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = 4steps
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

# Explicitly configure C++11 support, so that the project can be built with both GCC and Clang/LLVM compilers.
# See https://wiki.qt.io/How_to_use_C%2B%2B11_in_your_Qt_Projects
CONFIG += c++11

SOURCES += \
    asip.cpp \
    asip1.cpp \
    asip2.cpp \
    board.cpp \
    creategame.cpp \
    duration.cpp \
    game.cpp \
    gamelist.cpp \
    gamestate.cpp \
    login.cpp \
    main.cpp \
    mainwindow.cpp \
    movetree.cpp \
    opengame.cpp \
    playerbar.cpp \
    server.cpp \
    timecontrol.cpp \
    timeestimator.cpp

HEADERS += \
    asip.hpp \
    asip1.hpp \
    asip2.hpp \
    board.hpp \
    creategame.hpp \
    def.hpp \
    duration.hpp \
    game.hpp \
    gamelist.hpp \
    gamestate.hpp \
    globals.hpp \
    io.hpp \
    login.hpp \
    mainwindow.hpp \
    messagebox.hpp \
    movetree.hpp \
    opengame.hpp \
    playerbar.hpp \
    server.hpp \
    timecontrol.hpp \
    timeestimator.hpp \
    tree.hpp

RESOURCES += \
    resources.qrc
