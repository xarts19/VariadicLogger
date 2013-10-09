TEMPLATE = lib
CONFIG -= qt
CONFIG += staticlib
TARGET = ../VariadicLogger

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

CONFIG( debug, debug|release )  {
    #DEFINES += _GLIBCXX_DEBUG
}

# Input
HEADERS += \
    ../include/VariadicLogger/SafeSprintf.h \
    ../include/VariadicLogger/Logger.h \
    ../include/VariadicLogger/Event.hpp

SOURCES += \
    ../src/SafeSprintf.cpp \
    ../src/Logger.cpp
