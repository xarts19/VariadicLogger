TEMPLATE = lib
CONFIG -= qt
CONFIG += plugin
TARGET = VTLogger

!win32 {
    QMAKE_CXXFLAGS += -std=c++0x
}

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
HEADERS += Logger.h \
           SafeSprintf.h \
           StringUtil.h \
           Util.h \

SOURCES += Logger.cpp \
           SafeSprintf.cpp \
           StringUtil.cpp \
           Util.cpp \

win32 {
    SOURCES += UtilWin.cpp
} else {
    SOURCES += UtilLin.cpp
}