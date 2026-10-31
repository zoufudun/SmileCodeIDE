include(PhudonTools.pro)
QT += testlib
CONFIG += testcase console
TARGET = MainWindowWorkflowTest
HEADERS += tests/MainWindowWorkflowTest.h
SOURCES -= main.cpp
SOURCES += tests/MainWindowWorkflowTest.cpp
DESTDIR = $$OUT_PWD/tests-bin
