TEMPLATE = lib
CONFIG -= qt
CONFIG += staticlib
TARGET = ../VTLogger

!win32 {
    QMAKE_CXXFLAGS += -std=c++0x
}

INCLUDEPATH += \
    ../include/

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
    ../src/SafeSprintf.cpp \
    ../src/Logger.cpp
