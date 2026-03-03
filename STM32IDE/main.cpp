#include "TOOLS/CIconFont.h"
#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>


int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setStyle(QStyleFactory::create("Fusion"));

  // 设置应用程序信息
  app.setApplicationName("STM32 编译与调试工具");
  app.setOrganizationName("STM32IDE");

  // 初始化全局图标字体
  CIconFont::instance()->loadFont(":/Font/iconfontUltra.ttf");

  MainWindow window;
  window.show();

  return app.exec();
}