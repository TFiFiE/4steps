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
    analysis.cpp \
    arimaa_com.cpp \
    asip.cpp \
    asip1.cpp \
    asip2.cpp \
    board.cpp \
    bots.cpp \
    creategame.cpp \
    duration.cpp \
    game.cpp \
    gamelist.cpp \
    gamestate.cpp \
    iconengine.cpp \
    login.cpp \
    main.cpp \
    mainwindow.cpp \
    node.cpp \
    offboard.cpp \
    opengame.cpp \
    palette.cpp \
    pieceicons.cpp \
    playerbar.cpp \
    puzzles.cpp \
    server.cpp \
    startanalysis.cpp \
    timecontrol.cpp \
    timeestimator.cpp \
    treemodel.cpp \
    turnstate.cpp

HEADERS += \
    analysis.hpp \
    arimaa_com.hpp \
    asip.hpp \
    asip1.hpp \
    asip2.hpp \
    board.hpp \
    bots.hpp \
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
    node.hpp \
    offboard.hpp \
    opengame.hpp \
    palette.hpp \
    pieceicons.hpp \
    playerbar.hpp \
    potentialmove.hpp \
    puzzles.hpp \
    readonly.hpp \
    server.hpp \
    startanalysis.hpp \
    timecontrol.hpp \
    timeestimator.hpp \
    treemodel.hpp \
    turnstate.hpp

RESOURCES += \
    resources.qrc
