#ifndef TARGETCHIPDIALOG_H
#define TARGETCHIPDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QGroupBox>
#include <QSettings>
#include <QMap>
#include <QStringList>

#include "../Core/STM32Profile.h"

/**
 * @brief 编译构建与 GDB 调试工作台的目标芯片选择对话框
 * 包含厂商/分类、系列、器件三级级联选择，型号过滤搜索及规格参数概览
 */
class TargetChipDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TargetChipDialog(QWidget *parent = nullptr, QSettings *settings = nullptr,
                              const QString &initialChip = QString());
    ~TargetChipDialog() override = default;

    QString getSelectedChip() const;
    void setSelectedChip(const QString &chip);

    static bool selectChip(QWidget *parent, QString &chipInOut, QSettings *settings = nullptr);

private slots:
    void onVendorChanged(int index);
    void onFamilyChanged(int index);
    void onDeviceChanged(const QString &device);
    void onSearchTextChanged(const QString &text);
    void onTableSelectionChanged();
    void onTableDoubleClicked(int row, int column);
    void onAcceptClicked();

private:
    void setupUI();
    void applyTheme();
    void initChipData();
    void updateFamilyCombo();
    void updateDeviceCombo();
    void updateDeviceTable();
    void updateChipSpecs(const QString &chipName);

    QSettings *m_settings;
    QString m_selectedChip;
    bool m_updatingUi = false;

    // 顶部级联选择控件 (三级联动，与用户截图保持一致)
    QComboBox *m_vendorCombo;  // 厂商/分类: 如 "ST(意法半导体)"
    QComboBox *m_familyCombo;  // 系列: 如 "STM32H7xx"
    QComboBox *m_deviceCombo;  // 器件: 如 "STM32H7x_2M"

    // 搜索过滤与器件表格
    QLineEdit *m_searchEdit;
    QTableWidget *m_deviceTable;

    // 详细规格参数展示标签
    QLabel *m_specChipLabel;
    QLabel *m_specVendorFamilyLabel;
    QLabel *m_specCoreLabel;
    QLabel *m_specFlashLabel;
    QLabel *m_specRamLabel;
    QLabel *m_specMacroLabel;
    QLabel *m_specLdLabel;
    QLabel *m_specDebuggerLabel;

    // 底部操作按钮
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;

    // 内部数据映射
    QList<QString> m_vendors;
    QMap<QString, QStringList> m_vendorFamilies;
    QMap<QString, QStringList> m_familyDevices;
    QMap<QString, QString> m_deviceToFamily;
    QMap<QString, QString> m_familyToVendor;
};

#endif // TARGETCHIPDIALOG_H
