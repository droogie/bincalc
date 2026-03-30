QT += core

CONFIG += console c++11
CONFIG -= app_bundle

TARGET = bincalc_tests
TEMPLATE = app

SOURCES += \
    calculator_core.cpp \
    numeric_formats.cpp \
    tests.cpp

HEADERS += \
    calculator_core.h \
    numeric_formats.h
