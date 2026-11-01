QT += core gui widgets network testlib
CONFIG += testcase c++17 console
TARGET = FlashExperienceWidgetTest
MOC_DIR = $$OUT_PWD/moc
INCLUDEPATH += $$MOC_DIR ../Core ../UI
SOURCES += FlashExperienceWidgetTest.cpp \
    ../Core/BuildLogModel.cpp ../UI/SourceNavigator.cpp \
    ../Core/FlashLogModel.cpp \
    ../Core/ToolchainDetector.cpp ../Core/STM32Profile.cpp ../Core/MakefileGenerator.cpp \
    ../Core/BuildManager.cpp ../Core/FlashProgrammer.cpp ../Core/ProbeDetector.cpp \
    ../Core/GdbServerManager.cpp ../Core/GdbMiEngine.cpp \
    ../UI/GdbDebugPanel.cpp ../UI/ToolchainConfigDialog.cpp ../UI/FlashConfigDialog.cpp \
    ../UI/TargetChipDialog.cpp ../UI/STM32WorkbenchWidget.cpp
HEADERS += ../Core/ToolchainDetector.h ../Core/STM32Profile.h ../Core/MakefileGenerator.h \
    ../Core/BuildLogModel.h ../Core/BuildDiagnostic.h ../UI/SourceNavigator.h \
    ../Core/FlashLogModel.h \
    ../Core/BuildManager.h ../Core/FlashProgrammer.h ../Core/ProbeDetector.h \
    ../Core/GdbServerManager.h ../Core/GdbMiEngine.h \
    ../UI/GdbDebugPanel.h ../UI/ToolchainConfigDialog.h ../UI/FlashConfigDialog.h \
    ../UI/TargetChipDialog.h ../UI/STM32WorkbenchWidget.h
RESOURCES += ../cubemx_linkers.qrc ../cubemx_f1f4f7h7.qrc
win32:LIBS += -lsetupapi -lcfgmgr32
