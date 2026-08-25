#include "TOOLS/CIconFont.h"
#include "mainwindow.h"
#include "oscilloscopewindow.h"
#include "serialportplot.h"
#include "cantool.h"
#include "networktool.h"
#include "iaptool.h"
#include "appmanager.h"
#include "apphubwindow.h"
#include <QApplication>
#include <QIcon>
#include <QStyleFactory>
#include <QFile>

static void registerCoreApps() {
    AppManager *mgr = AppManager::instance();

    // 1. SmileCode IDE (代码编辑器)
    AppInfo ideInfo;
    ideInfo.id = "smilecode_ide";
    ideInfo.name = QStringLiteral("SmileCode IDE");
    ideInfo.subtitle = QStringLiteral("嵌入式 C/C++ 代码编辑器与 GDB 调试器");
    ideInfo.category = AppCategory::EmbeddedDev;
    ideInfo.version = "v2.1.0";
    ideInfo.author = "SmileCode Team";
    ideInfo.iconPath = ":/icons/file_code_icon.png";
    ideInfo.colorHex = "#6c5ce7";
    ideInfo.description = QStringLiteral("基于 QScintilla 的全功能嵌入式 C/C++ IDE，支持 GCC/Make 一键编译构建、GDB 图形化断点调试与多主题语法高亮。");
    ideInfo.tags = QStringList() << "C/C++" << "GCC/Make" << "GDB 调试" << "STM32";
    mgr->registerBuiltInApp(ideInfo, [](QWidget *parent) -> QWidget* {
        return new MainWindow(parent);
    });

    // 2. 多通信接口数字示波器
    AppInfo scopeInfo;
    scopeInfo.id = "oscilloscope";
    scopeInfo.name = QStringLiteral("多接口数字示波器");
    scopeInfo.subtitle = QStringLiteral("串口/TCP/UDP/WebSocket 实时波形采集与频谱分析");
    scopeInfo.category = AppCategory::Measurement;
    scopeInfo.version = "v2.0.0";
    scopeInfo.author = "SmileCode Team";
    scopeInfo.iconPath = ":/icons/CLOCK.png";
    scopeInfo.colorHex = "#00cec9";
    scopeInfo.description = QStringLiteral("支持串口、TCP/UDP、WebSocket 与 CAN 多通道数据流高速采集，提供实时波形绘制、FFT 频谱分析与测量光标。");
    scopeInfo.tags = QStringList() << "多通道波形" << "FFT 频谱" << "TCP/UDP" << "测量标尺";
    mgr->registerBuiltInApp(scopeInfo, [](QWidget *parent) -> QWidget* {
        Q_UNUSED(parent);
        return new OscilloscopeWindow();
    });

    // 3. 串口调试助手
    AppInfo serialInfo;
    serialInfo.id = "serial_plot";
    serialInfo.name = QStringLiteral("串口调试大师 Pro");
    serialInfo.subtitle = QStringLiteral("多标签页高速串口通信与分屏工作台");
    serialInfo.category = AppCategory::BusProtocol;
    serialInfo.version = "v3.0.0";
    serialInfo.author = "SmileCode Team";
    serialInfo.iconPath = ":/icons/serialport.png";
    serialInfo.colorHex = "#e17055";
    serialInfo.description = QStringLiteral("支持多标签页独立会话、水平/垂直多窗口分屏、HEX/ASCII 高速收发、实时曲线绘制、Modbus 解析与自定义协议设计。");
    serialInfo.tags = QStringList() << "多标签页" << "分屏监视" << "曲线绘制" << "Modbus";
    mgr->registerBuiltInApp(serialInfo, [](QWidget *parent) -> QWidget* {
        Q_UNUSED(parent);
        SerialPortContainer *container = new SerialPortContainer();
        container->setWindowTitle(QStringLiteral("串口调试助手 Pro"));
        container->resize(1000, 620);
        return container;
    });

    // 4. CAN 调试助手
    AppInfo canInfo;
    canInfo.id = "can_tool";
    canInfo.name = QStringLiteral("CAN 总线工作台 Pro");
    canInfo.subtitle = QStringLiteral("ZLG / CXCAN 工业级 CAN/CANFD 与 CANopen 调试");
    canInfo.category = AppCategory::BusProtocol;
    canInfo.version = "v2.2.0";
    canInfo.author = "SmileCode Team";
    canInfo.iconPath = ":/icons/CAN.png";
    canInfo.colorHex = "#d63031";
    canInfo.description = QStringLiteral("支持周立功 USBCANFD、创芯 ControlCAN 硬件接口，集成 CANopen Master 主站管理、总线负载率统计与多包周期发送。");
    canInfo.tags = QStringList() << "USBCANFD" << "CXCAN" << "CANopen" << "负载率统计";
    mgr->registerBuiltInApp(canInfo, [](QWidget *parent) -> QWidget* {
        Q_UNUSED(parent);
        return new CANTool();
    });

    // 5. 网络调试助手
    AppInfo netInfo;
    netInfo.id = "network_tool";
    netInfo.name = QStringLiteral("网络通信调试助手");
    netInfo.subtitle = QStringLiteral("TCP Client/Server 与 UDP 单播/广播/多播调试");
    netInfo.category = AppCategory::BusProtocol;
    netInfo.version = "v2.0.0";
    netInfo.author = "SmileCode Team";
    netInfo.iconPath = ":/icons/network_tool.png";
    netInfo.colorHex = "#0984e3";
    netInfo.description = QStringLiteral("支持 TCP 客户端自动重连、TCP 服务端多客户端在线列表管理与广播/单发、UDP 广播调试，支持定时循环发送与日志导出。");
    netInfo.tags = QStringList() << "TCP 客户端" << "TCP 服务端" << "UDP 通信" << "HEX 收发";
    mgr->registerBuiltInApp(netInfo, [](QWidget *parent) -> QWidget* {
        Q_UNUSED(parent);
        return new NetworkTool();
    });

    // 6. STM32 IAP 固件升级工具
    AppInfo iapInfo;
    iapInfo.id = "iap_tool";
    iapInfo.name = QStringLiteral("STM32 IAP 固件升级");
    iapInfo.subtitle = QStringLiteral("串口与网络双通道 Bootloader 固件升级工具");
    iapInfo.category = AppCategory::Flashing;
    iapInfo.version = "v1.8.0";
    iapInfo.author = "SmileCode Team";
    iapInfo.iconPath = ":/icons/IAP.png";
    iapInfo.colorHex = "#00b894";
    iapInfo.description = QStringLiteral("专为 STM32 系列微控制器设计的 Bootloader 升级工具，支持串口 Ymodem/自定义协议及网络 TCP 双通道固件下载与校验。");
    iapInfo.tags = QStringList() << "STM32" << "Bootloader" << "固件烧录" << "TCP 升级";
    mgr->registerBuiltInApp(iapInfo, [](QWidget *parent) -> QWidget* {
        Q_UNUSED(parent);
        IAPTool *iap = new IAPTool();
        iap->setWindowTitle(QStringLiteral("STM32 IAP 固件升级工具"));
        iap->resize(640, 520);
        return iap;
    });

    // 初始化管理器并加载用户配置与外部插件
    mgr->initialize();
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 设置应用全局图标
    app.setWindowIcon(QIcon(":/resources/logo.png"));

    // ----------------------------
    // 载入全局深色样式表
    // ----------------------------
    QString qss;
    QFile f(":/resources/styles/dark.qss");
    if (!f.exists()) {
        f.setFileName(":/qdarkstyle/dark/darkstyle.qss");
    }
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        qss = QString::fromUtf8(f.readAll());
        app.setStyleSheet(qss);
    }

    app.setStyle(QStyleFactory::create("Fusion"));

    // 设置应用程序信息
    app.setApplicationName("SmileCodeStudio");
    app.setOrganizationName("SmileCode");

    // 初始化全局图标字体
    CIconFont::instance()->loadFont(":/Font/iconfontUltra.ttf");

    // 注册所有内置核心应用与插件系统
    registerCoreApps();

    // 启动展现 APP 市场工作台主界面
    AppHubWindow hubWindow;
    hubWindow.show();

    return app.exec();
}