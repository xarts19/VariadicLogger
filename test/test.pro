TEMPLATE = app
CONFIG -= qt
CONFIG += console
TARGET = ../test

!win32 {
    QMAKE_CXXFLAGS += -std=c++0x
}

INCLUDEPATH += \
    ../include/

unix|win32: LIBS += -l../VTLogger
win32: PRE_TARGETDEPS += ../VTLogger.lib
else:unix: PRE_TARGETDEPS += ../libVTLogger.a

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
    ../include/VTLogger/SafeSprintf.h \
    ../include/VTLogger/Logger.h

SOURCES += \
    test_main.cpp
