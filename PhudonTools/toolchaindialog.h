#ifndef TOOLCHAINDIALOG_H
#define TOOLCHAINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QFileDialog>
#include <QSettings>

class ToolchainDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ToolchainDialog(QWidget *parent = nullptr, QSettings *settings = nullptr);
    ~ToolchainDialog();

    QString getGccPath() const { return m_gccPath->text(); }
    QString getOpenocdPath() const { return m_openocdPath->text(); }
    QString getOpenocdConfig() const { return m_openocdConfig->text(); }
    QString getJlinkPath() const { return m_jlinkPath->text(); }

private slots:
    void browseGccPath();
    void browseOpenocdPath();
    void browseOpenocdConfig();
    void browseJlinkPath();
    void saveSettings();

private:
    QLineEdit *m_gccPath;
    QLineEdit *m_openocdPath;
    QLineEdit *m_openocdConfig;
    QLineEdit *m_jlinkPath;
    
    QPushButton *m_browseGccButton;
    QPushButton *m_browseOpenocdButton;
    QPushButton *m_browseOpenocdConfigButton;
    QPushButton *m_browseJlinkButton;
    
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
    
    QSettings *m_settings;
};

#endif // TOOLCHAINDIALOG_H