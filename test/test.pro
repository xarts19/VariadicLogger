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

# The following block leaves managing debug/release configuration to
# qt creator.
CONFIG -= debug_and_release
CONFIG( debug, debug|release )  {
  CONFIG -= release
}
else {
  CONFIG -= debug
  CONFIG += release
}

# Input
HEADERS += \
    ../include/VariadicLogger/SafeSprintf.h \
    ../include/VariadicLogger/Logger.h

SOURCES += \
    test_main.cpp

