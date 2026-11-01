QT += core gui widgets concurrent testlib
CONFIG += testcase c++17 console debug
TARGET = ToolchainConfigDialogAsyncTest
INCLUDEPATH += $$PWD/../../../PhudonTools
MOC_DIR = $$OUT_PWD/moc
SOURCES += ToolchainConfigDialogAsyncTest.cpp \
    ../UI/ToolchainConfigDialog.cpp ../UI/TargetChipDialog.cpp \
    ../Core/ToolchainDetector.cpp ../Core/STM32Profile.cpp
HEADERS += ../UI/ToolchainConfigDialog.h ../UI/TargetChipDialog.h \
    ../Core/ToolchainDetector.h ../Core/STM32Profile.h
