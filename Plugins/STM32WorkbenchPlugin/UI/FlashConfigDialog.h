#ifndef FLASHCONFIGDIALOG_H
#define FLASHCONFIGDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include "../Core/FlashProgrammer.h"
#include "../Core/ProbeDetector.h"

/**
 * @brief 固件下载与烧录参数配置对话框
 */
class FlashConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FlashConfigDialog(QWidget *parent = nullptr, QSettings *settings = nullptr,
                               const STM32ChipProfile &profile = STM32ChipProfile());
    ~FlashConfigDialog() override = default;

    FlashOptions getOptions() const;

private slots:
    void onSaveClicked();
    void scanProbes();
    void onProbeChanged(int index);

private:
    void setupUI();
    void applyTheme();
    void loadFromSettings();
    void updateProbeInfo();
    void updateDriverOptions();

    QSettings *m_settings;
    STM32ChipProfile m_profile;
    QString m_addressSettingKey;

    QComboBox *m_driverCombo;
    QComboBox *m_probeCombo;
    QLabel *m_probeInfoLabel;
    QLineEdit *m_probeSerialEdit;
    QList<ProbeInfo> m_probes;
    QString m_savedProbeId;
    bool m_hasSavedProbeChoice = false;
    QLineEdit *m_targetAddrEdit;
    QCheckBox *m_verifyCheck;
    QCheckBox *m_resetCheck;
    QCheckBox *m_eraseAllCheck;
    QSpinBox *m_speedSpin;

    QLineEdit *m_customInterfaceEdit;
    QLineEdit *m_customTargetEdit;

    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};

#endif // FLASHCONFIGDIALOG_H
