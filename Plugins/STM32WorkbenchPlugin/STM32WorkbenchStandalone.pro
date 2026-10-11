QT += core gui widgets network

CONFIG += c++17 app_bundle

TARGET = STM32Workbench
TEMPLATE = app

INCLUDEPATH += $$PWD/../../PhudonTools/include
INCLUDEPATH += $$PWD/Core
INCLUDEPATH += $$PWD/UI

HEADERS += \
    Core/ToolchainDetector.h \
    Core/STM32Profile.h \
    Core/MakefileGenerator.h \
    Core/BuildManager.h \
    Core/FlashProgrammer.h \
    Core/ProbeDetector.h \
    Core/GdbServerManager.h \
    Core/GdbMiEngine.h \
    UI/GdbDebugPanel.h \
    UI/ToolchainConfigDialog.h \
    UI/FlashConfigDialog.h \
    UI/STM32WorkbenchWidget.h

SOURCES += \
    standalone_main.cpp \
    Core/ToolchainDetector.cpp \
    Core/STM32Profile.cpp \
    Core/MakefileGenerator.cpp \
    Core/BuildManager.cpp \
    Core/FlashProgrammer.cpp \
    Core/ProbeDetector.cpp \
    Core/GdbServerManager.cpp \
    Core/GdbMiEngine.cpp \
    UI/GdbDebugPanel.cpp \
    UI/ToolchainConfigDialog.cpp \
    UI/FlashConfigDialog.cpp \
    UI/STM32WorkbenchWidget.cpp

win32:LIBS += -lsetupapi -lcfgmgr32

RESOURCES += cubemx_linkers.qrc
