QT       += core gui
QT += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# 替换为你的实际路径
LIBS += -L"D:/Soft/Qt/5.15.2/mingw81_64/lib" -lqscintilla2_qt5
INCLUDEPATH += "D:/Soft/Qt/5.15.2/mingw81_64/include/Qsci"

# 添加QCodeEditor
include(QCodeEditor/QCodeEditor.pri)

# 源文件
SOURCES += \
    buildsystem.cpp \
    codeeditor.cpp \
    foldableeditor.cpp \
    main.cpp \
    mainwindow.cpp \
    toolchaindialog.cpp

# 头文件
HEADERS += \
    buildsystem.h \
    codeeditor.h \
    foldableeditor.h \
    mainwindow.h \
    toolchaindialog.h

# 默认规则
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic

# 资源文件
# 添加资源文件
RESOURCES += resources.qrc
