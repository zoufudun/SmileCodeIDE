QT += core gui widgets

CONFIG += c++11 plugin

TARGET = TemplatePlugin
TEMPLATE = lib

# 引入核心宿主插件接口头文件目录
INCLUDEPATH += $$PWD/../../PhudonTools/include

HEADERS += \
    templateplugin.h \
    templatewidget.h

SOURCES += \
    templateplugin.cpp \
    templatewidget.cpp

DISTFILES += \
    plugin.json \
    README.md

# 默认直接生成在当前工程根目录下
DESTDIR = $$PWD

# 编译完成后自动复制到宿主主程序 plugins 目录
DEST_PLUGIN_DIR = $$PWD/../../PhudonTools/plugins
QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$DESTDIR/$${TARGET}.dll) $$shell_path($$DEST_PLUGIN_DIR)

