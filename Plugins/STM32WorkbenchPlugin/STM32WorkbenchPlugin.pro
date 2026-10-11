QT += core gui widgets network

CONFIG += c++17 plugin

TARGET = STM32WorkbenchPlugin
TEMPLATE = lib

# 引入宿主插件接口
INCLUDEPATH += $$PWD/../../PhudonTools/include
INCLUDEPATH += $$PWD/Core
INCLUDEPATH += $$PWD/UI

HEADERS += \
    stm32workbenchplugin.h \
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
    stm32workbenchplugin.cpp \
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

DISTFILES += \
    plugin.json \
    README.md

win32 {
    LIBS += -lsetupapi -lcfgmgr32
    CONFIG(debug, debug|release) {
        PLUGIN_BUILD_DIR = $$OUT_PWD/debug
    } else {
        PLUGIN_BUILD_DIR = $$OUT_PWD/release
    }
    DEST_DIR_SRC = $$PWD/../../PhudonTools/plugins
    DEST_DIR_REL = $$OUT_PWD/../../PhudonTools/release/plugins
    DEST_DIR_DBG = $$OUT_PWD/../../PhudonTools/debug/plugins

    PLUGIN_BUILD_DIR ~= s,/,\\,g
    DEST_DIR_SRC ~= s,/,\\,g
    DEST_DIR_REL ~= s,/,\\,g
    DEST_DIR_DBG ~= s,/,\\,g

    QMAKE_POST_LINK += $$quote(cmd /c if not exist "$${DEST_DIR_SRC}" mkdir "$${DEST_DIR_SRC}"$$escape_expand(\n\t))
    QMAKE_POST_LINK += $$quote(cmd /c if exist "$${PLUGIN_BUILD_DIR}\\$${TARGET}.dll" copy /y "$${PLUGIN_BUILD_DIR}\\$${TARGET}.dll" "$${DEST_DIR_SRC}"$$escape_expand(\n\t))
    QMAKE_POST_LINK += $$quote(cmd /c if exist "$${DEST_DIR_REL}" copy /y "$${PLUGIN_BUILD_DIR}\\$${TARGET}.dll" "$${DEST_DIR_REL}"$$escape_expand(\n\t))
    QMAKE_POST_LINK += $$quote(cmd /c if exist "$${DEST_DIR_DBG}" copy /y "$${PLUGIN_BUILD_DIR}\\$${TARGET}.dll" "$${DEST_DIR_DBG}"$$escape_expand(\n\t))
}
