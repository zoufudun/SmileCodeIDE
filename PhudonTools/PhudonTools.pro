QT       += core gui serialport printsupport network websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# 替换为你的实际路径
LIBS += -L"D:/Soft/Qt/5.15.2/mingw81_64/lib" -lqscintilla2_qt5
INCLUDEPATH += "D:/Soft/Qt/5.15.2/mingw81_64/include/Qsci"

# ZLG zlgcan SDK & 创芯科技 ControlCAN SDK 头文件
INCLUDEPATH += $$PWD/USBCANFD $$PWD/CXCAN

# 将 zlgcan.dll, ControlCAN.dll 与 kerneldlls 拷贝到生成目录（与可执行文件同级），便于运行时加载。
win32 {
    ZLG_SRC = $$PWD/USBCANFD
    CX_SRC = $$PWD/CXCAN
    CONFIG(debug, debug|release) {
        ZLG_OUT = $$OUT_PWD/debug
    } else {
        ZLG_OUT = $$OUT_PWD/release
    }
    ZLG_SRC ~= s,/,\\,g
    CX_SRC ~= s,/,\\,g
    ZLG_OUT ~= s,/,\\,g
    QMAKE_POST_LINK += $$quote(cmd /c copy /y "$${ZLG_SRC}\zlgcan.dll" "$${ZLG_OUT}"$$escape_expand(\n\t))
    QMAKE_POST_LINK += $$quote(cmd /c copy /y "$${CX_SRC}\ControlCAN.dll" "$${ZLG_OUT}"$$escape_expand(\n\t))
    QMAKE_POST_LINK += $$quote(cmd /c xcopy /e /i /y "$${ZLG_SRC}\kerneldlls" "$${ZLG_OUT}\kerneldlls"$$escape_expand(\n\t))
}

# 添加QCodeEditor
# include(QCodeEditor/QCodeEditor.pri)

# 生成可执行文件名称
TARGET = PhudonTools
TEMPLATE = app

# Windows 可执行文件图标
win32: RC_ICONS = resources/app.ico

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
    normalsenddialog.cpp \
    caninterface.cpp \
    canopenmaster.cpp \
    candevicedialog.cpp \
    canprotocolmonitor.cpp \
    canprotocolconfigdialog.cpp \
    protocolconfigdialog.cpp \
    devicestatuswidget.cpp \
    devicemonitorpanel.cpp \
    roomwidget.cpp \
    roommanagerdialog.cpp \
    toastwidget.cpp \
    scrollinglabel.cpp \
    terminalwidget.cpp \
    curvesettings.cpp \
    customwidget.cpp \
    widgetdesigner.cpp \
    editorwidget.cpp \
    mcuprofilemanager.cpp \
    verticaltabwidget.cpp \
    canviewpanel.cpp \
    canopenviewpanel.cpp \
    devicemonitordialog.cpp \
    canbusutilizationdialog.cpp

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
    normalsenddialog.h \
    caninterface.h \
    canopenmaster.h \
    candevicedialog.h \
    canprotocolmonitor.h \
    canprotocolconfigdialog.h \
    protocolconfigdialog.h \
    devicestatuswidget.h \
    devicemonitorpanel.h \
    roomwidget.h \
    roommanagerdialog.h \
    cantheme.h \
    toastwidget.h \
    scrollinglabel.h \
    terminalwidget.h \
    curvesettings.h \
    customwidget.h \
    widgetdesigner.h \
    editorwidget.h \
    mcuprofilemanager.h \
    verticaltabwidget.h \
    canviewpanel.h \
    canopenviewpanel.h \
    devicemonitordialog.h \
    canbusutilizationdialog.h

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
RESOURCES += resources.qrc \
    ../dark/darkstyle.qrc
