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


SOURCES += \
    arimaa_com.cpp \
    asip.cpp \
    asip1.cpp \
    asip2.cpp \
    board.cpp \
    creategame.cpp \
    duration.cpp \
    game.cpp \
    gamelist.cpp \
    gamestate.cpp \
    iconengine.cpp \
    login.cpp \
    main.cpp \
    mainwindow.cpp \
    movetree.cpp \
    opengame.cpp \
    pieceicons.cpp \
    playerbar.cpp \
    server.cpp \
    timecontrol.cpp \
    timeestimator.cpp

HEADERS += \
    arimaa_com.hpp \
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
    iconengine.hpp \
    io.hpp \
    login.hpp \
    mainwindow.hpp \
    messagebox.hpp \
    movetree.hpp \
    opengame.hpp \
    pieceicons.hpp \
    playerbar.hpp \
    readonly.hpp \
    server.hpp \
    timecontrol.hpp \
    timeestimator.hpp \
    tree.hpp

RESOURCES += \
    resources.qrc
