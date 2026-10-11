QT += core testlib
CONFIG += testcase c++17 console
TARGET = ToolchainDetectorTest
INCLUDEPATH += $$OUT_PWD/release
SOURCES += ToolchainDetectorTest.cpp \
           ../Core/ToolchainDetector.cpp \
           ../Core/MakefileGenerator.cpp \
           ../Core/STM32Profile.cpp
HEADERS += ../Core/ToolchainDetector.h \
           ../Core/MakefileGenerator.h \
           ../Core/STM32Profile.h
RESOURCES += ../cubemx_linkers.qrc
