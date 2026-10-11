QT += core widgets testlib
CONFIG += testcase c++17 console
TARGET = ProbeDetectorTest
INCLUDEPATH += $$OUT_PWD/release
SOURCES += ProbeDetectorTest.cpp ../Core/ProbeDetector.cpp ../Core/FlashProgrammer.cpp ../Core/STM32Profile.cpp ../Core/ToolchainDetector.cpp ../UI/FlashConfigDialog.cpp
HEADERS += ../Core/ProbeDetector.h ../Core/FlashProgrammer.h ../Core/STM32Profile.h ../Core/ToolchainDetector.h ../UI/FlashConfigDialog.h
win32:LIBS += -lsetupapi -lcfgmgr32
