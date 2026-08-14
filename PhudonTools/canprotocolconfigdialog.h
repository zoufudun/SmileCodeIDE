#ifndef CANPROTOCOLCONFIGDIALOG_H
#define CANPROTOCOLCONFIGDIALOG_H

#include <QDialog>
#include <QJsonArray>

class QTableWidget;
class QPushButton;
class QSpinBox;
class QComboBox;
class QLineEdit;
class QGroupBox;

// CAN 2.0B 协议位映射
struct DeviceBitMapping {
  int deviceId;      // 唯一标识
  QString label;      // 显示名称（如"1F大厅烟感01"）
  QString deviceType; // "detector" | "valve"
  quint32 canId;      // CAN 帧 ID
  int byteIndex;      // 数据字节索引 (0-7)
  int bitIndex;       // 位索引 (0-7, LSB=0)
  int defaultVal;     // 默认值 (0或1)
  QString targetView; // 所属界面/标签页名称（如"界面1", "驾驶舱"）
  QString targetRoom; // 所属房间名称/ID（如"1号机房", "控制室"）
  int canDevice = 0;  // CAN 设备 Index (如 0)
  int canChannel = -1;// CAN 通道 Index (-1: 任意通道, 0: 通道0, 1: 通道1)
};

// 协议配置对话框 —— 管理 CAN ID 到设备状态的映射表。
// 支持添加/删除/导入/导出（JSON 格式）。
class CanProtocolConfigDialog : public QDialog {
  Q_OBJECT
public:
  explicit CanProtocolConfigDialog(QWidget *parent = nullptr);

  // 设置可选界面与房间列表
  void setAvailableViews(const QStringList &viewNames);
  void setAvailableRooms(const QStringList &roomNames);
  void setRoomViewPairs(const QList<QPair<QString, QString>> &pairs);

  // 获取/设置当前的映射配置
  void setMappings(const QList<DeviceBitMapping> &mappings);
  QList<DeviceBitMapping> mappings() const;

  // 应用主题样式
  void applyThemeStyle(const QString &qss);

  // CSV 导入导出静态辅助接口 (QByteArray 正确处理 BOM 与 GBK/UTF-8 编码)
  static QByteArray exportMappingsToCsv(const QList<DeviceBitMapping> &list);
  static QList<DeviceBitMapping> parseCsvToMappings(const QByteArray &csvData);

private slots:
  void onAddRow();
  void onDeleteRow();
  void onModifyRow();
  void onSelectionChanged();
  void onImportJson();
  void onExportJson();

private:
  void addTableRow(int deviceId, const QString &label, const QString &type,
                   quint32 canId, int byteIdx, int bitIdx, int defaultVal,
                   const QString &targetView = QStringLiteral("界面1"),
                   const QString &targetRoom = QString(),
                   int canChannel = -1);

  QTableWidget *m_table;
  QPushButton *m_btnAdd;
  QPushButton *m_btnModify;
  QPushButton *m_btnImport;
  QPushButton *m_btnExport;
  QPushButton *m_btnDelete;
  QPushButton *m_btnClearAll;
  QPushButton *m_btnOk;
  QPushButton *m_btnCancel;

  // 默认值设置控件
  QLineEdit *m_labelEdit;
  QComboBox *m_defaultTypeCombo;
  QLineEdit *m_defaultCanIdEdit;
  QSpinBox *m_defaultByteSpin;
  QSpinBox *m_defaultBitSpin;
  QComboBox *m_defaultValCombo; // 默认状态选择
  QComboBox *m_defaultChannelCombo; // CAN 通道选择
  QComboBox *m_targetViewCombo; // 所属界面下拉框
  QComboBox *m_targetRoomCombo; // 所属房间下拉/可输入框

  QStringList m_availableViews;
  QStringList m_availableRooms;
  QList<QPair<QString, QString>> m_roomViewPairs;
  int m_nextDeviceId = 1;
};

#endif // CANPROTOCOLCONFIGDIALOG_H
