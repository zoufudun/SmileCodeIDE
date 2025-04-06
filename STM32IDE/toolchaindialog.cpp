#include "toolchaindialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>

ToolchainDialog::ToolchainDialog(QWidget *parent, QSettings *settings)
    : QDialog(parent), m_settings(settings)
{
    setWindowTitle("配置工具链");
    setMinimumWidth(600);
    
    // 创建表单布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // GCC 工具链配置组
    QGroupBox *gccGroup = new QGroupBox("ARM GCC 工具链");
    QGridLayout *gccLayout = new QGridLayout(gccGroup);
    
    QLabel *gccLabel = new QLabel("GCC 路径:");
    m_gccPath = new QLineEdit();
    m_browseGccButton = new QPushButton("浏览...");
    
    gccLayout->addWidget(gccLabel, 0, 0);
    gccLayout->addWidget(m_gccPath, 0, 1);
    gccLayout->addWidget(m_browseGccButton, 0, 2);
    
    // OpenOCD 配置组
    QGroupBox *openocdGroup = new QGroupBox("OpenOCD 调试器");
    QGridLayout *openocdLayout = new QGridLayout(openocdGroup);
    
    QLabel *openocdLabel = new QLabel("OpenOCD 路径:");
    m_openocdPath = new QLineEdit();
    m_browseOpenocdButton = new QPushButton("浏览...");
    
    QLabel *configLabel = new QLabel("配置文件:");
    m_openocdConfig = new QLineEdit();
    m_browseOpenocdConfigButton = new QPushButton("浏览...");
    
    openocdLayout->addWidget(openocdLabel, 0, 0);
    openocdLayout->addWidget(m_openocdPath, 0, 1);
    openocdLayout->addWidget(m_browseOpenocdButton, 0, 2);
    
    openocdLayout->addWidget(configLabel, 1, 0);
    openocdLayout->addWidget(m_openocdConfig, 1, 1);
    openocdLayout->addWidget(m_browseOpenocdConfigButton, 1, 2);
    
    // J-Link 配置组
    QGroupBox *jlinkGroup = new QGroupBox("J-Link 下载器");
    QGridLayout *jlinkLayout = new QGridLayout(jlinkGroup);
    
    QLabel *jlinkLabel = new QLabel("J-Link 路径:");
    m_jlinkPath = new QLineEdit();
    m_browseJlinkButton = new QPushButton("浏览...");
    
    jlinkLayout->addWidget(jlinkLabel, 0, 0);
    jlinkLayout->addWidget(m_jlinkPath, 0, 1);
    jlinkLayout->addWidget(m_browseJlinkButton, 0, 2);
    
    // 按钮布局
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    
    // 添加所有组到主布局
    mainLayout->addWidget(gccGroup);
    mainLayout->addWidget(openocdGroup);
    mainLayout->addWidget(jlinkGroup);
    mainLayout->addWidget(buttonBox);
    
    // 连接信号和槽
    connect(m_browseGccButton, &QPushButton::clicked, this, &ToolchainDialog::browseGccPath);
    connect(m_browseOpenocdButton, &QPushButton::clicked, this, &ToolchainDialog::browseOpenocdPath);
    connect(m_browseOpenocdConfigButton, &QPushButton::clicked, this, &ToolchainDialog::browseOpenocdConfig);
    connect(m_browseJlinkButton, &QPushButton::clicked, this, &ToolchainDialog::browseJlinkPath);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ToolchainDialog::saveSettings);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    // 加载现有设置
    if (m_settings) {
        m_settings->beginGroup("Toolchain");
        m_gccPath->setText(m_settings->value("gccPath", "").toString());
        m_openocdPath->setText(m_settings->value("openocdPath", "").toString());
        m_openocdConfig->setText(m_settings->value("openocdConfig", "").toString());
        m_jlinkPath->setText(m_settings->value("jlinkPath", "").toString());
        m_settings->endGroup();
    }
}

ToolchainDialog::~ToolchainDialog()
{
}

void ToolchainDialog::browseGccPath()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择ARM GCC工具链目录", 
                                                  m_gccPath->text(),
                                                  QFileDialog::ShowDirsOnly | 
                                                  QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_gccPath->setText(dir);
    }
}

void ToolchainDialog::browseOpenocdPath()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择OpenOCD目录", 
                                                  m_openocdPath->text(),
                                                  QFileDialog::ShowDirsOnly | 
                                                  QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_openocdPath->setText(dir);
    }
}

void ToolchainDialog::browseOpenocdConfig()
{
    QString file = QFileDialog::getOpenFileName(this, "选择OpenOCD配置文件", 
                                              m_openocdConfig->text(),
                                              "配置文件 (*.cfg);;所有文件 (*.*)");
    if (!file.isEmpty()) {
        m_openocdConfig->setText(file);
    }
}

void ToolchainDialog::browseJlinkPath()
{
    QString file = QFileDialog::getOpenFileName(this, "选择J-Link可执行文件", 
                                              m_jlinkPath->text(),
                                              "可执行文件 (*.exe);;所有文件 (*.*)");
    if (!file.isEmpty()) {
        m_jlinkPath->setText(file);
    }
}

void ToolchainDialog::saveSettings()
{
    if (m_settings) {
        m_settings->beginGroup("Toolchain");
        m_settings->setValue("gccPath", m_gccPath->text());
        m_settings->setValue("openocdPath", m_openocdPath->text());
        m_settings->setValue("openocdConfig", m_openocdConfig->text());
        m_settings->setValue("jlinkPath", m_jlinkPath->text());
        m_settings->endGroup();
        m_settings->sync();
    }
    
    accept();
}