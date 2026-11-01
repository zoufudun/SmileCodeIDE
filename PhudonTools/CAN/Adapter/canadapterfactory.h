#ifndef CANADAPTERFACTORY_H
#define CANADAPTERFACTORY_H

#include <QList>
#include <QMap>
#include <QString>
#include "icanadapter.h"

struct CanAdapterMetadata {
  QString id;             // 标识，如 "simulation", "peakcan", "zlg"
  QString name;           // 显示名称，如 "纯软件仿真适配器", "PEAK PCAN 适配器"
  QString category;       // 类别: "Simulation", "QtSerialBus", "Commercial", "ThirdParty"
  QString description;    // 描述说明
  bool isAvailable = true;// 当前系统环境是否支持或依赖库是否完备
};

class CanAdapterFactory {
public:
  // 获取所有支持的适配器元数据列表
  static QList<CanAdapterMetadata> availableAdapters();

  // 根据类型 ID 创建适配器实例
  static ICanAdapter *createAdapter(const QString &adapterId, QObject *parent = nullptr);

  // 获取 Qt 原生支持的 SerialBus 插件名称列表
  static QStringList qtAvailablePlugins();
};

#endif // CANADAPTERFACTORY_H
