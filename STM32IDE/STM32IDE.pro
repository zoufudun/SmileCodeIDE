QT       += core gui serialport printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# 替换为你的实际路径
LIBS += -L"D:/Soft/Qt/5.15.2/mingw81_64/lib" -lqscintilla2_qt5
INCLUDEPATH += "D:/Soft/Qt/5.15.2/mingw81_64/include/Qsci"

# 添加QCodeEditor
# include(QCodeEditor/QCodeEditor.pri)

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
    toolchaindialog.cpp \
    cantool.cpp

# 头文件
HEADERS += \
    ../qcustomplot/qcustomplot.h \
    TOOLS/CConversion.h \
    TOOLS/CIconFont.h \
    buildsystem.h \
    codeeditor.h \
    mainwindow.h \
    serialportplot.h \
    toolchaindialog.h \
    cantool.h

# 默认规则
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic

# 资源文件
# 添加资源文件
RESOURCES += resources.qrc
