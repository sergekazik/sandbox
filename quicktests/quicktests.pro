TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -std=c++1z -pthread
QMAKE_CFLAGS += -std=c++1z -pthread
LIBS += -lpthread

SOURCES += main.cpp \
    converter_xml.cpp \
    scrambler.cpp

include(deployment.pri)
qtcAddDeployment()

