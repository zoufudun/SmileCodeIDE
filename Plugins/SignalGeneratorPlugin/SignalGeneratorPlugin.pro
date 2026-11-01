QT += core gui widgets

CONFIG += c++17 plugin

TARGET = SignalGeneratorPlugin
TEMPLATE = lib

INCLUDEPATH += $$PWD/../../PhudonTools/include

HEADERS += \
    signalgeneratorplugin.h \
    signalgeneratorwidget.h

SOURCES += \
    signalgeneratorplugin.cpp \
    signalgeneratorwidget.cpp

DISTFILES += \
    plugin.json

win32 {
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

