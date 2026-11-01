QT += core gui testlib
CONFIG += testcase console c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = BuildLogModelTest
SOURCES += BuildLogModelTest.cpp ../Core/BuildLogModel.cpp
HEADERS += ../Core/BuildLogModel.h ../Core/BuildDiagnostic.h
