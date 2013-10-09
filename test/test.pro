TEMPLATE = app
CONFIG -= qt
CONFIG += console
TARGET = ../TestLogger

!win32 {
    QMAKE_CXXFLAGS += -std=c++0x
    LIBS += -lpthread
}

INCLUDEPATH += \
    ../include/

win32: LIBS += ../VariadicLogger.lib
else:unix: LIBS += ../libVariadicLogger.a
win32: PRE_TARGETDEPS += ../VariadicLogger.lib
else:unix: PRE_TARGETDEPS += ../libVariadicLogger.a

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
    ../include/VariadicLogger/Event.hpp \
    catch.hpp

SOURCES += \
    test_main.cpp

