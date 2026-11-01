QT += core gui testlib
CONFIG += testcase c++17 console
TARGET = FlashLogModelTest
MOC_DIR = $$OUT_PWD/moc
INCLUDEPATH += $$MOC_DIR
SOURCES += FlashLogModelTest.cpp ../Core/FlashLogModel.cpp
HEADERS += ../Core/FlashLogModel.h
