QT += core gui network testlib
CONFIG += testcase c++17 console
CONFIG -= app_bundle
TARGET = STM32DebugControllerTest
MOC_DIR = $$OUT_PWD/moc
INCLUDEPATH += .. ../../Plugins/STM32WorkbenchPlugin/Core $$MOC_DIR
CORE = ../../Plugins/STM32WorkbenchPlugin/Core
SOURCES += STM32DebugControllerTest.cpp ../stm32debugcontroller.cpp \
    $$CORE/BuildManager.cpp $$CORE/BuildLogModel.cpp $$CORE/MakefileGenerator.cpp \
    $$CORE/ToolchainDetector.cpp $$CORE/STM32Profile.cpp $$CORE/ProbeDetector.cpp \
    $$CORE/FlashProgrammer.cpp $$CORE/FlashLogModel.cpp \
    $$CORE/GdbServerManager.cpp $$CORE/GdbMiEngine.cpp
HEADERS += ../stm32debugcontroller.h $$CORE/BuildManager.h $$CORE/BuildLogModel.h \
    $$CORE/ToolchainDetector.h $$CORE/ProbeDetector.h $$CORE/FlashProgrammer.h \
    $$CORE/FlashLogModel.h $$CORE/GdbServerManager.h $$CORE/GdbMiEngine.h
win32:LIBS += -lsetupapi -lcfgmgr32
