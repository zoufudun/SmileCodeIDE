QT       += core gui serialport printsupport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# 替换为你的实际路径
LIBS += -L"D:/Soft/Qt/5.15.2/mingw81_64/lib" -lqscintilla2_qt5
INCLUDEPATH += "D:/Soft/Qt/5.15.2/mingw81_64/include/Qsci"

# 添加QCodeEditor
# include(QCodeEditor/QCodeEditor.pri)

# 生成可执行文件名称
TARGET = Stm32Compiler
TEMPLATE = app

# 源文件
SOURCES += \
    ../qcustomplot/qcustomplot.cpp \
    TOOLS/CConversion.cpp \
    TOOLS/CIconFont.cpp \
    buildsystem.cpp \
    codeeditor.cpp \
    main.cpp \
    mainwindow.cpp\
    serialportplot.cpp \
    iaptool.cpp \
    toolchaindialog.cpp \
    cantool.cpp \
    toastwidget.cpp \
    scrollinglabel.cpp \
    terminalwidget.cpp \
    curvesettings.cpp \
    customwidget.cpp \
    widgetdesigner.cpp \
    editorwidget.cpp \
    mcuprofilemanager.cpp \
    verticaltabwidget.cpp

# 头文件
HEADERS += \
    ../qcustomplot/qcustomplot.h \
    TOOLS/CConversion.h \
    TOOLS/CIconFont.h \
    buildsystem.h \
    codeeditor.h \
    mainwindow.h \
    serialportplot.h \
    iaptool.h \
    toolchaindialog.h \
    cantool.h \
    toastwidget.h \
    scrollinglabel.h \
    terminalwidget.h \
    curvesettings.h \
    customwidget.h \
    widgetdesigner.h \
    editorwidget.h \
    mcuprofilemanager.h \
    verticaltabwidget.h

# 默认规则
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic

# Windows下需要的配置（解决中文乱码、进程调用）
win32 {
    QMAKE_CXXFLAGS += -utf-8
    # 指定make工具路径（如果未配置环境变量，需手动填写）
    DEFINES += MAKE_PATH="\"mingw32-make.exe\""
}
# Linux/macOS配置
unix {
    DEFINES += MAKE_PATH="\"make\""
}

# 资源文件
# 添加资源文件
RESOURCES += resources.qrc
