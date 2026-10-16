QT += core gui testlib
CONFIG += testcase c++17 console
TARGET = ToolchainDetectorTest
MOC_DIR = $$OUT_PWD/moc
INCLUDEPATH += $$MOC_DIR
SOURCES += ToolchainDetectorTest.cpp \
           ../Core/BuildLogModel.cpp \
           ../Core/ToolchainDetector.cpp \
           ../Core/MakefileGenerator.cpp \
           ../Core/STM32Profile.cpp \
           ../Core/BuildManager.cpp
HEADERS += ../Core/ToolchainDetector.h \
           ../Core/BuildLogModel.h ../Core/BuildDiagnostic.h \
           ../Core/MakefileGenerator.h \
           ../Core/STM32Profile.h \
           ../Core/BuildManager.h
RESOURCES += ../cubemx_linkers.qrc
RESOURCES += ../cubemx_f1f4f7h7.qrc
