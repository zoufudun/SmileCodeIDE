#include <QApplication>
#include <QSettings>
#include "UI/STM32WorkbenchWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("STM32Workbench");
    app.setOrganizationName("SmileCode");

    QSettings settings("SmileCode", "STM32Workbench");
    STM32WorkbenchWidget window(nullptr, &settings);
    window.setWindowTitle(QStringLiteral("SmileCode STM32 独立编译构建、固件烧录与硬件 GDB 调试工作台"));
    window.resize(1100, 780);

    // 如果命令行传入了工程目录参数，自动打开
    if (argc > 1) {
        QString path = QString::fromLocal8Bit(argv[1]);
        window.setProjectPath(path);
    }

    window.show();
    return app.exec();
}
