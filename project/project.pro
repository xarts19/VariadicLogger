TEMPLATE = lib
CONFIG -= qt
CONFIG += staticlib
TARGET = ../VariadicLogger

!win32 {
    QMAKE_CXXFLAGS += -std=c++0x
}

INCLUDEPATH += \
    ../include/

CONFIG( debug, debug|release )  {
    #DEFINES += _GLIBCXX_DEBUG
}

CONFIG( release, debug|release )  {
    DEFINES *= NDEBUG
}

# Input
HEADERS += \
    ../include/VariadicLogger/SafeSprintf.h \
    ../include/VariadicLogger/Logger.h \
    ../include/VariadicLogger/Event.hpp

SOURCES += \
    ../src/SafeSprintf.cpp \
    ../src/Logger.cpp
