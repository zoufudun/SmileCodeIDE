QT += core gui widgets concurrent testlib
CONFIG += testcase c++17 console
CONFIG -= app_bundle
TARGET = ProjectSearchTest
INCLUDEPATH += ..
SOURCES += ProjectSearchTest.cpp ../projectsearchengine.cpp ../projectsearchpanel.cpp
HEADERS += ../projectsearchengine.h ../projectsearchpanel.h
