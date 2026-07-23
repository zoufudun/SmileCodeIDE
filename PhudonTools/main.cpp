#include "TOOLS/CIconFont.h"
#include "mainwindow.h"
#include <QApplication>
#include <QIcon>
#include <QStyleFactory>


int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

    // 设置应用全局图标
    app.setWindowIcon(QIcon(":/resources/logo.png"));

    // ----------------------------
    // 载入 QDarkStyle 样式表
    // ----------------------------
    QString qss;
    QFile f(":/qdarkstyle/dark/darkstyle.qss");  // 需 QRC 资源
    if (!f.exists()) {
        // 如果没有加入资源，可考虑用 Python 生成 qss 文件路径加载
        QFile f2(QCoreApplication::applicationDirPath() + "/style.qss");
        if (f2.open(QFile::ReadOnly | QFile::Text)) {
            qss = QString::fromUtf8(f2.readAll());
        }
    } else if (f.open(QFile::ReadOnly | QFile::Text)) {
        qss = QString::fromUtf8(f.readAll());
    }

    if (!qss.isEmpty())
        app.setStyleSheet(qss);

  app.setStyle(QStyleFactory::create("Fusion"));

  // 设置应用程序信息
  app.setApplicationName("PhudonTools");
  app.setOrganizationName("PhudonTools");

  // 初始化全局图标字体
  CIconFont::instance()->loadFont(":/Font/iconfontUltra.ttf");

  MainWindow window;
  window.show();

  return app.exec();
}