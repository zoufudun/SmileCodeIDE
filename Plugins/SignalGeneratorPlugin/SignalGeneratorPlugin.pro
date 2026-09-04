QT += core gui widgets

CONFIG += c++11 plugin

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

# 默认生成在当前插件工程根目录下
DESTDIR = $$PWD

# 自动将编译出的动态库分发至主程序的 plugins 目录
DEST_PLUGIN_DIR = $$PWD/../../PhudonTools/plugins
QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$DESTDIR/$${TARGET}.dll) $$shell_path($$DEST_PLUGIN_DIR)
