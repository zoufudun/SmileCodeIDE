QT       += core gui serialport printsupport network websockets serialbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat

CONFIG += c++17

# QScintilla 头文件与库文件配置（同时支持 Qt5 与 Qt6，自适应工程内置源码构建产物）
INCLUDEPATH += $$PWD/../QScintilla/src $$PWD/../QScintilla/src/Qsci

greaterThan(QT_MAJOR_VERSION, 5) {
    QSCINTILLA_BASE = qscintilla2_qt6
} else {
    QSCINTILLA_BASE = qscintilla2_qt5
}

win32:CONFIG(debug, debug|release) {
    exists($$PWD/../QScintilla/src/debug/$${QSCINTILLA_BASE}d.dll)|exists($$PWD/../QScintilla/src/debug/lib$${QSCINTILLA_BASE}d.a) {
        QSCINTILLA_LIB = $${QSCINTILLA_BASE}d
    } else {
        QSCINTILLA_LIB = $${QSCINTILLA_BASE}
    }
} else {
    QSCINTILLA_LIB = $${QSCINTILLA_BASE}
}

# 优先链接工程内置构建产物，同时支持 Release 与 Debug
CONFIG(debug, debug|release) {
    LIBS += -L$$PWD/../QScintilla/src/debug -L$$PWD/../QScintilla/src/release -l$$QSCINTILLA_LIB
} else {
    LIBS += -L$$PWD/../QScintilla/src/release -l$$QSCINTILLA_LIB
}

win32: LIBS += -lsetupapi -lcfgmgr32

# ZLG zlgcan SDK & 创芯科技 ControlCAN SDK 头文件 & STM32 核心引擎
INCLUDEPATH += $$PWD/USBCANFD $$PWD/CXCAN $$PWD/include $$PWD/CAN $$PWD/../Plugins/STM32WorkbenchPlugin/Core

# 将 zlgcan.dll, ControlCAN.dll, kerneldlls, qscintilla dll 以及 plugins 拷贝到生成目录（与可执行文件同级）
win32 {
    ZLG_SRC = $$PWD/USBCANFD
    CX_SRC = $$PWD/CXCAN
    PLUGINS_SRC = $$PWD/plugins
    CONFIG(debug, debug|release) {
        ZLG_OUT = $$OUT_PWD/debug
        QSCI_DIR = $$PWD/../QScintilla/src/debug
        !exists($$QSCI_DIR/$${QSCINTILLA_LIB}.dll): QSCI_DIR = $$PWD/../QScintilla/src/release
    } else {
        ZLG_OUT = $$OUT_PWD/release
        QSCI_DIR = $$PWD/../QScintilla/src/release
    }
    ZLG_SRC ~= s,/,\\,g
    CX_SRC ~= s,/,\\,g
    ZLG_OUT ~= s,/,\\,g
    QSCI_DIR ~= s,/,\\,g
    PLUGINS_SRC ~= s,/,\\,g
    QMAKE_POST_LINK += $$quote(cmd /c copy /y "$${ZLG_SRC}\zlgcan.dll" "$${ZLG_OUT}"$$escape_expand(\n\t))
    QMAKE_POST_LINK += $$quote(cmd /c copy /y "$${CX_SRC}\ControlCAN.dll" "$${ZLG_OUT}"$$escape_expand(\n\t))
    QMAKE_POST_LINK += $$quote(cmd /c xcopy /e /i /y "$${ZLG_SRC}\kerneldlls" "$${ZLG_OUT}\kerneldlls"$$escape_expand(\n\t))
    QMAKE_POST_LINK += $$quote(cmd /c if exist "$${QSCI_DIR}\\$${QSCINTILLA_LIB}.dll" copy /y "$${QSCI_DIR}\\$${QSCINTILLA_LIB}.dll" "$${ZLG_OUT}"$$escape_expand(\n\t))
    QMAKE_POST_LINK += $$quote(cmd /c if exist "$${PLUGINS_SRC}" xcopy /e /i /y "$${PLUGINS_SRC}" "$${ZLG_OUT}\plugins"$$escape_expand(\n\t))
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
    canbusutilizationdialog.cpp \
    oscilloscopewindow.cpp \
    appmanager.cpp \
    networktool.cpp \
    appcardwidget.cpp \
    pluginmanagerdialog.cpp \
    apphubwindow.cpp \
    CAN/Adapter/canadapterfactory.cpp \
    CAN/Adapter/simulationcanadapter.cpp \
    CAN/Adapter/qtserialbusadapter.cpp \
    CAN/Engine/isotpengine.cpp \
    CAN/Engine/udsengine.cpp \
    CAN/Engine/dbcparserengine.cpp \
    CAN/Engine/recordingengine.cpp \
    CAN/Engine/knowledgeengine.cpp \
    CAN/Service/flashservice.cpp \
    CAN/Service/diagnosticservice.cpp \
    CAN/Service/busmonitorservice.cpp \
    CAN/UI/cansignalplotpanel.cpp \
    CAN/UI/candiagnosticpanel.cpp \
    CAN/UI/canflashpanel.cpp \
    CAN/UI/canaiassistant.cpp \
    ../Plugins/STM32WorkbenchPlugin/Core/ToolchainDetector.cpp \
    ../Plugins/STM32WorkbenchPlugin/Core/STM32Profile.cpp \
    ../Plugins/STM32WorkbenchPlugin/Core/MakefileGenerator.cpp \
    ../Plugins/STM32WorkbenchPlugin/Core/BuildManager.cpp \
    ../Plugins/STM32WorkbenchPlugin/Core/BuildLogModel.cpp \
    ../Plugins/STM32WorkbenchPlugin/Core/FlashProgrammer.cpp \
    ../Plugins/STM32WorkbenchPlugin/Core/FlashLogModel.cpp \
    ../Plugins/STM32WorkbenchPlugin/Core/ProbeDetector.cpp \
    ../Plugins/STM32WorkbenchPlugin/Core/GdbServerManager.cpp \
    ../Plugins/STM32WorkbenchPlugin/Core/GdbMiEngine.cpp \
    stm32debugcontroller.cpp \
    stm32debugdocks.cpp

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
    canbusutilizationdialog.h \
    scopetheme.h \
    idetheme.h \
    oscilloscopewindow.h \
    appmanager.h \
    networktool.h \
    appcardwidget.h \
    pluginmanagerdialog.h \
    apphubwindow.h \
    include/iappplugin.h \
    include/iplugincontext.h \
    CAN/Adapter/icanadapter.h \
    CAN/Adapter/canadapterfactory.h \
    CAN/Adapter/simulationcanadapter.h \
    CAN/Adapter/qtserialbusadapter.h \
    CAN/Engine/isotpengine.h \
    CAN/Engine/udsengine.h \
    CAN/Engine/dbcparserengine.h \
    CAN/Engine/recordingengine.h \
    CAN/Engine/knowledgeengine.h \
    CAN/Service/flashservice.h \
    CAN/Service/diagnosticservice.h \
    CAN/Service/busmonitorservice.h \
    CAN/UI/cansignalplotpanel.h \
    CAN/UI/candiagnosticpanel.h \
    CAN/UI/canflashpanel.h \
    CAN/UI/canaiassistant.h \
    ../Plugins/STM32WorkbenchPlugin/Core/ToolchainDetector.h \
    ../Plugins/STM32WorkbenchPlugin/Core/STM32Profile.h \
    ../Plugins/STM32WorkbenchPlugin/Core/MakefileGenerator.h \
    ../Plugins/STM32WorkbenchPlugin/Core/BuildManager.h \
    ../Plugins/STM32WorkbenchPlugin/Core/BuildDiagnostic.h \
    ../Plugins/STM32WorkbenchPlugin/Core/BuildLogModel.h \
    ../Plugins/STM32WorkbenchPlugin/Core/FlashProgrammer.h \
    ../Plugins/STM32WorkbenchPlugin/Core/FlashLogModel.h \
    ../Plugins/STM32WorkbenchPlugin/Core/ProbeDetector.h \
    ../Plugins/STM32WorkbenchPlugin/Core/GdbServerManager.h \
    ../Plugins/STM32WorkbenchPlugin/Core/GdbMiEngine.h \
    stm32debugcontroller.h \
    stm32debugdocks.h

# 默认规则
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic

# Windows下需要的配置（解决编码、大对象段数限制与工具链调用）
win32 {
    # 解决 MinGW 在 Debug 下编译 QCustomPlot 等大型文件报 too many sections / file too big 错误
    contains(QMAKE_COMPILER, gcc)|win32-g++|win32-clang-g++ {
        QMAKE_CXXFLAGS += -Wa,-mbig-obj
    }
    win32-msvc {
        QMAKE_CXXFLAGS += /bigobj -utf-8
    }

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
    ../dark/darkstyle.qrc \
    $$PWD/../Plugins/STM32WorkbenchPlugin/cubemx_linkers.qrc
