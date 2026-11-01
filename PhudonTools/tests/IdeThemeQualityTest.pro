QT += core gui widgets testlib core5compat
CONFIG += testcase c++17 console release
CONFIG -= app_bundle debug
DEFINES += QSCINTILLA_DLL
TARGET = IdeThemeQualityTest
INCLUDEPATH += .. ../../QScintilla/src
SOURCES += IdeThemeQualityTest.cpp ../codeeditor.cpp
HEADERS += ../codeeditor.h ../idetheme.h
LIBS += -L$$PWD/../../QScintilla/src/release -lqscintilla2_qt6
exists($$PWD/../codeeditorcommands.cpp):SOURCES += ../codeeditorcommands.cpp

