#ifndef TOOLCHAINCONFIGDIALOG_H
#define TOOLCHAINCONFIGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QSettings>
#include <QGroupBox>
#include "../Core/ToolchainDetector.h"
#include "../Core/STM32Profile.h"

/**
 * @brief 现代化工具链与烧录器自检配置对话框
 */
class ToolchainConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ToolchainConfigDialog(QWidget *parent = nullptr, QSettings *settings = nullptr,
                                   const QString &themeId = QString());
    void applyTheme(const QString &themeId = QString());
    ~ToolchainConfigDialog() override = default;

    ToolchainPaths getPaths() const;
    QString getSelectedChip() const;
    void setPaths(const ToolchainPaths &paths);
    void setChipSelectionVisible(bool visible);

protected:
    void done(int result) override;

private slots:
    void onAutoDetectClicked();
    void onBrowseGcc();
    void onBrowseGdb();
    void onBrowseMake();
    void onBrowseOpenocd();
    void onBrowseOpenocdScripts();
    void onBrowseJlink();
    void onBrowseStm32Prog();
    void onSaveClicked();
    void onOpenChipSelectDialog();

private:
    void setupUI();
    void updateFormFromPaths(const ToolchainPaths &paths);
    void startBackgroundCheck(bool scan);
    void setChecking(bool checking);

    QSettings *m_settings;
    ToolchainDetector *m_detector;
    ToolchainPaths m_paths;
    quint64 m_checkGeneration = 0;
    bool m_checking = false;
    bool m_chipSelectionVisible = true;
    QGroupBox *m_chipGroup;
    QGroupBox *m_toolchainGroup;
    QGroupBox *m_hardwareGroup;

    QLineEdit *m_gccEdit;
    QLineEdit *m_gdbEdit;
    QLineEdit *m_makeEdit;
    QLineEdit *m_openocdEdit;
    QLineEdit *m_openocdScriptsEdit;
    QLineEdit *m_jlinkEdit;
    QLineEdit *m_stm32ProgEdit;

    QComboBox *m_chipCombo;
    QPushButton *m_selectChipBtn;
    QTextEdit *m_diagnosticReportEdit;

    QPushButton *m_autoDetectBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};

#endif // TOOLCHAINCONFIGDIALOG_H
