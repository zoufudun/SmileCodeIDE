#include "pluginmanagerdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextBrowser>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QFileInfo>

PluginManagerDialog::PluginManagerDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("插件与扩展中心 (Plugin & Extension Hub)"));
    resize(920, 560);
    setMinimumSize(780, 460);

    setupUi();
    refreshPluginList();

    connect(AppManager::instance(), &AppManager::pluginListChanged, this, &PluginManagerDialog::refreshPluginList);
}

void PluginManagerDialog::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // ==========================================
    // 顶部操作工具栏
    // ==========================================
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(8);

    m_installDllBtn = new QPushButton(QStringLiteral("⚡ 加载动态插件 (.dll)"), this);
    m_installDllBtn->setCursor(Qt::PointingHandCursor);
    m_installDllBtn->setStyleSheet("background-color: #00b894; color: white; font-weight: bold; padding: 6px 14px; border-radius: 6px;");
    connect(m_installDllBtn, &QPushButton::clicked, this, &PluginManagerDialog::onInstallDllClicked);

    m_installJsonBtn = new QPushButton(QStringLiteral("➕ 配置插件 (.json)"), this);
    m_installJsonBtn->setCursor(Qt::PointingHandCursor);
    m_installJsonBtn->setStyleSheet("background-color: #6c5ce7; color: white; font-weight: bold; padding: 6px 12px; border-radius: 6px;");
    connect(m_installJsonBtn, &QPushButton::clicked, this, &PluginManagerDialog::onInstallJsonClicked);

    m_createToolBtn = new QPushButton(QStringLiteral("🛠 外部工具"), this);
    m_createToolBtn->setCursor(Qt::PointingHandCursor);
    m_createToolBtn->setStyleSheet("background-color: #0984e3; color: white; font-weight: bold; padding: 6px 12px; border-radius: 6px;");
    connect(m_createToolBtn, &QPushButton::clicked, this, &PluginManagerDialog::onCreateCustomToolClicked);

    m_openDirBtn = new QPushButton(QStringLiteral("📂 打开插件目录"), this);
    m_openDirBtn->setCursor(Qt::PointingHandCursor);
    connect(m_openDirBtn, &QPushButton::clicked, this, &PluginManagerDialog::onOpenPluginsDirClicked);

    m_refreshBtn = new QPushButton(QStringLiteral("🔄 重新扫描"), this);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshBtn, &QPushButton::clicked, this, &PluginManagerDialog::onRescanClicked);

    m_guideBtn = new QPushButton(QStringLiteral("💡 开发指南"), this);
    m_guideBtn->setCursor(Qt::PointingHandCursor);
    m_guideBtn->setStyleSheet("color: #fdcb6e; border: 1px solid rgba(253, 203, 110, 0.4); border-radius: 6px; padding: 6px 12px;");
    connect(m_guideBtn, &QPushButton::clicked, this, &PluginManagerDialog::onDevGuideClicked);

    m_countLabel = new QLabel(this);
    m_countLabel->setStyleSheet("color: #95a5a6; font-size: 12px;");

    topLayout->addWidget(m_installDllBtn);
    topLayout->addWidget(m_installJsonBtn);
    topLayout->addWidget(m_createToolBtn);
    topLayout->addWidget(m_openDirBtn);
    topLayout->addWidget(m_refreshBtn);
    topLayout->addWidget(m_guideBtn);
    topLayout->addStretch();
    topLayout->addWidget(m_countLabel);

    mainLayout->addLayout(topLayout);

    // ==========================================
    // 插件列表表格
    // ==========================================
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(7);
    m_tableWidget->setHorizontalHeaderLabels(QStringList()
        << QStringLiteral("启用")
        << QStringLiteral("应用 / 插件名称")
        << QStringLiteral("架构类型")
        << QStringLiteral("分类")
        << QStringLiteral("版本")
        << QStringLiteral("作者")
        << QStringLiteral("操作"));

    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);

    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setStyleSheet("QTableWidget { gridline-color: rgba(255,255,255,0.06); }");

    mainLayout->addWidget(m_tableWidget);
}

void PluginManagerDialog::refreshPluginList() {
    m_tableWidget->setRowCount(0);
    QVector<AppInfo> apps = AppManager::instance()->getAllApps();

    int row = 0;
    int dynamicCount = 0;
    int externalCount = 0;

    for (const AppInfo &app : apps) {
        if (app.pluginType == AppPluginType::QtDynamicPlugin) dynamicCount++;
        else if (!app.isBuiltIn) externalCount++;

        m_tableWidget->insertRow(row);

        // 1. 启用状态开关
        QWidget *switchWidget = new QWidget(this);
        QHBoxLayout *switchLayout = new QHBoxLayout(switchWidget);
        switchLayout->setContentsMargins(0, 0, 0, 0);
        switchLayout->setAlignment(Qt::AlignCenter);
        QCheckBox *check = new QCheckBox(switchWidget);
        check->setChecked(app.isEnabled);
        check->setEnabled(!app.isBuiltIn); // 内置应用默认锁定启用
        QString appId = app.id;
        connect(check, &QCheckBox::toggled, this, [this, appId](bool checked) {
            onEnableToggled(appId, checked);
        });
        switchLayout->addWidget(check);
        m_tableWidget->setCellWidget(row, 0, switchWidget);

        // 2. 名称与 ID
        QTableWidgetItem *nameItem = new QTableWidgetItem(QString("%1 (%2)").arg(app.name).arg(app.id));
        nameItem->setToolTip(app.description + (app.dllPath.isEmpty() ? "" : "\n文件: " + app.dllPath));
        m_tableWidget->setItem(row, 1, nameItem);

        // 3. 架构类型
        QString typeStr;
        if (app.isBuiltIn) {
            typeStr = QStringLiteral("内置核心");
        } else if (app.pluginType == AppPluginType::QtDynamicPlugin) {
            typeStr = QStringLiteral("Qt 动态插件 (.dll)");
        } else if (app.pluginType == AppPluginType::ExternalExecutable) {
            typeStr = QStringLiteral("外部程序");
        } else {
            typeStr = QStringLiteral("脚本工具");
        }
        QTableWidgetItem *typeItem = new QTableWidgetItem(typeStr);
        typeItem->setTextAlignment(Qt::AlignCenter);
        if (app.pluginType == AppPluginType::QtDynamicPlugin) {
            typeItem->setForeground(QColor("#00cec9"));
        }
        m_tableWidget->setItem(row, 2, typeItem);

        // 4. 分类
        QTableWidgetItem *catItem = new QTableWidgetItem(app.categoryName);
        catItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 3, catItem);

        // 5. 版本
        QTableWidgetItem *verItem = new QTableWidgetItem(app.version);
        verItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 4, verItem);

        // 6. 作者
        QTableWidgetItem *authorItem = new QTableWidgetItem(app.author.isEmpty() ? QStringLiteral("系统") : app.author);
        authorItem->setTextAlignment(Qt::AlignCenter);
        m_tableWidget->setItem(row, 5, authorItem);

        // 7. 操作按钮 (详情 / 卸载)
        QWidget *actionWidget = new QWidget(this);
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(4, 2, 4, 2);
        actionLayout->setSpacing(6);
        actionLayout->setAlignment(Qt::AlignCenter);

        QPushButton *detailBtn = new QPushButton(QStringLiteral("详情"), actionWidget);
        detailBtn->setCursor(Qt::PointingHandCursor);
        detailBtn->setStyleSheet("color: #74b9ff; background: rgba(116, 185, 255, 0.1); border: 1px solid rgba(116, 185, 255, 0.3); border-radius: 4px; padding: 2px 8px; font-size: 11px;");
        connect(detailBtn, &QPushButton::clicked, this, [this, appId]() {
            onPluginDetailsClicked(appId);
        });
        actionLayout->addWidget(detailBtn);

        if (!app.isBuiltIn) {
            QPushButton *uninstallBtn = new QPushButton(QStringLiteral("卸载"), actionWidget);
            uninstallBtn->setCursor(Qt::PointingHandCursor);
            uninstallBtn->setStyleSheet("color: #e74c3c; background: rgba(231, 76, 60, 0.1); border: 1px solid rgba(231, 76, 60, 0.3); border-radius: 4px; padding: 2px 8px; font-size: 11px;");
            connect(uninstallBtn, &QPushButton::clicked, this, [this, appId]() {
                onUninstallClicked(appId);
            });
            actionLayout->addWidget(uninstallBtn);
        }

        m_tableWidget->setCellWidget(row, 6, actionWidget);
        row++;
    }

    m_countLabel->setText(QStringLiteral("已注册应用: %1 个 (内置: %2, Qt动态库: %3, 外部工具: %4)")
        .arg(apps.size())
        .arg(apps.size() - dynamicCount - externalCount)
        .arg(dynamicCount)
        .arg(externalCount));
}

void PluginManagerDialog::onInstallDllClicked() {
    QString filter = QStringLiteral("Qt 动态插件 (*.dll *.so *.dylib);;所有文件 (*.*)");
    QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择要加载的 Qt 动态库插件"), QString(), filter);
    if (filePath.isEmpty()) return;

    QString pluginsDir = AppManager::instance()->getPluginsDirectory();
    QDir().mkpath(pluginsDir);

    QFileInfo fi(filePath);
    QString targetPath = pluginsDir + "/" + fi.fileName();

    // 如果不在 plugins 目录下，自动复制到 plugins 目录以实现持久化
    if (fi.absoluteFilePath() != QFileInfo(targetPath).absoluteFilePath()) {
        if (QFile::exists(targetPath)) {
            auto ret = QMessageBox::question(this, QStringLiteral("文件已存在"),
                QStringLiteral("目标插件目录已存在同名文件 [%1]，是否覆盖？").arg(fi.fileName()),
                QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                QFile::remove(targetPath);
                QFile::copy(filePath, targetPath);
            } else {
                return;
            }
        } else {
            QFile::copy(filePath, targetPath);
        }
        targetPath = QFileInfo(targetPath).absoluteFilePath();
    } else {
        targetPath = fi.absoluteFilePath();
    }

    QString errorMsg;
    if (AppManager::instance()->loadDynamicPlugin(targetPath, &errorMsg)) {
        QMessageBox::information(this, QStringLiteral("加载成功"), QStringLiteral("🎉 Qt 动态库插件已成功验证并加载到应用市场！"));
        refreshPluginList();
    } else {
        QMessageBox::critical(this, QStringLiteral("加载失败"), QStringLiteral("无法加载该动态库插件:\n%1").arg(errorMsg));
    }
}

void PluginManagerDialog::onInstallJsonClicked() {
    QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("选择插件描述文件"), QString(), QStringLiteral("插件配置 (*.json)"));
    if (filePath.isEmpty()) return;

    QString errorMsg;
    if (AppManager::instance()->installPluginFromJson(filePath, &errorMsg)) {
        QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("插件安装成功！"));
        refreshPluginList();
    } else {
        QMessageBox::critical(this, QStringLiteral("安装失败"), errorMsg);
    }
}

void PluginManagerDialog::onCreateCustomToolClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("注册外部自定义工具"));
    dlg.resize(480, 320);

    QFormLayout *form = new QFormLayout(&dlg);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(10);

    QLineEdit *idEdit = new QLineEdit(&dlg);
    QLineEdit *nameEdit = new QLineEdit(&dlg);
    QLineEdit *subEdit = new QLineEdit(&dlg);
    QLineEdit *pathEdit = new QLineEdit(&dlg);
    QPushButton *browseBtn = new QPushButton(QStringLiteral("浏览..."), &dlg);
    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->addWidget(pathEdit);
    pathLayout->addWidget(browseBtn);

    connect(browseBtn, &QPushButton::clicked, [&dlg, pathEdit]() {
        QString file = QFileDialog::getOpenFileName(&dlg, QStringLiteral("选择可执行文件"), QString(), QStringLiteral("可执行程序 (*.exe *.bat *.cmd);;所有文件 (*.*)"));
        if (!file.isEmpty()) pathEdit->setText(file);
    });

    QLineEdit *argsEdit = new QLineEdit(&dlg);
    argsEdit->setPlaceholderText(QStringLiteral("选填，多个参数以空格分隔"));

    QLineEdit *descEdit = new QLineEdit(&dlg);
    QComboBox *catCombo = new QComboBox(&dlg);
    catCombo->addItem(QStringLiteral("实用工具"));
    catCombo->addItem(QStringLiteral("总线与通信"));
    catCombo->addItem(QStringLiteral("测量与分析"));
    catCombo->addItem(QStringLiteral("固件与烧录"));

    form->addRow(QStringLiteral("工具 ID (唯一字母数字):"), idEdit);
    form->addRow(QStringLiteral("工具名称:"), nameEdit);
    form->addRow(QStringLiteral("简述/副标题:"), subEdit);
    form->addRow(QStringLiteral("可执行文件路径:"), pathLayout);
    form->addRow(QStringLiteral("启动参数:"), argsEdit);
    form->addRow(QStringLiteral("所属分类:"), catCombo);
    form->addRow(QStringLiteral("功能介绍:"), descEdit);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(box);

    if (dlg.exec() == QDialog::Accepted) {
        QString id = idEdit->text().trimmed();
        QString name = nameEdit->text().trimmed();
        QString path = pathEdit->text().trimmed();

        if (id.isEmpty() || name.isEmpty() || path.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("ID、名称和可执行文件路径为必填项！"));
            return;
        }

        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["subtitle"] = subEdit->text().trimmed();
        obj["category"] = catCombo->currentText();
        obj["description"] = descEdit->text().trimmed();
        obj["version"] = "v1.0.0";
        obj["author"] = QStringLiteral("自定义扩展");
        obj["type"] = "executable";
        obj["execPath"] = path;
        
        QString args = argsEdit->text().trimmed();
        if (!args.isEmpty()) {
            QJsonArray arr;
            for (const QString &a : args.split(' ', Qt::SkipEmptyParts)) {
                arr.append(a);
            }
            obj["execArgs"] = arr;
        }

        QString pluginsDir = AppManager::instance()->getPluginsDirectory();
        QDir().mkpath(pluginsDir);
        QString jsonPath = pluginsDir + "/" + id + ".json";
        
        QFile file(jsonPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(QJsonDocument(obj).toJson());
            file.close();
            AppManager::instance()->installPluginFromJson(jsonPath);
            QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("外部工具注册成功！"));
            refreshPluginList();
        }
    }
}

void PluginManagerDialog::onOpenPluginsDirClicked() {
    QString pluginsDir = AppManager::instance()->getPluginsDirectory();
    QDir().mkpath(pluginsDir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(pluginsDir));
}

void PluginManagerDialog::onRescanClicked() {
    int count = AppManager::instance()->rescanPlugins();
    refreshPluginList();
    AppManager::instance()->showToast(QStringLiteral("扫描完成，发现 %1 个新插件").arg(count), "info");
}

void PluginManagerDialog::onDevGuideClicked() {
    QDialog guideDlg(this);
    guideDlg.setWindowTitle(QStringLiteral("💡 Qt Plugin 动态插件开发极简指引"));
    guideDlg.resize(680, 500);

    QVBoxLayout *layout = new QVBoxLayout(&guideDlg);
    QTextBrowser *browser = new QTextBrowser(&guideDlg);
    browser->setOpenExternalLinks(true);

    QString guideHtml = QStringLiteral(
        "<h2>🌟 SmileCode Qt Plugin 零修改扩展开发</h2>"
        "<p>开发者只需编写一个继承 <code>IAppPlugin</code> 的 Qt 动态库工程并编译为 <code>.dll</code>，直接拷贝至 <code>plugins/</code> 目录即可自动上线！</p>"
        "<h3>1. 引入核心头文件</h3>"
        "<pre>#include &quot;include/iappplugin.h&quot;</pre>"
        "<h3>2. 实现 IAppPlugin 接口并声明元数据</h3>"
        "<pre>"
        "class MyPlugin : public QObject, public IAppPlugin {\n"
        "    Q_OBJECT\n"
        "    Q_PLUGIN_METADATA(IID IAppPlugin_IID FILE &quot;plugin.json&quot;)\n"
        "    Q_INTERFACES(IAppPlugin)\n"
        "public:\n"
        "    QString id() const override { return &quot;my_plugin&quot;; }\n"
        "    QString name() const override { return &quot;自定义波形分析器&quot;; }\n"
        "    QString category() const override { return &quot;测量与分析&quot;; }\n"
        "    bool initialize(IPluginContext *ctx) override { return true; }\n"
        "    void shutdown() override {}\n"
        "    QWidget* createWidget(QWidget *parent) override { return new MyWidget(parent); }\n"
        "};</pre>"
        "<h3>3. 工程配置 (.pro)</h3>"
        "<pre>"
        "TEMPLATE = lib\n"
        "CONFIG += plugin\n"
        "QT += core gui widgets\n"
        "DESTDIR = $$PWD/../../PhudonTools/plugins</pre>"
    );

    browser->setHtml(guideHtml);
    layout->addWidget(browser);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Close, &guideDlg);
    connect(btnBox, &QDialogButtonBox::rejected, &guideDlg, &QDialog::accept);
    layout->addWidget(btnBox);

    guideDlg.exec();
}

void PluginManagerDialog::onPluginDetailsClicked(const QString &appId) {
    if (!AppManager::instance()->hasApp(appId)) return;
    AppInfo info = AppManager::instance()->getAppInfo(appId);

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("应用 / 插件详情 - %1").arg(info.name));
    dlg.resize(480, 360);

    QFormLayout *form = new QFormLayout(&dlg);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(12);

    auto addRowText = [&](const QString &label, const QString &text) {
        QLineEdit *le = new QLineEdit(text, &dlg);
        le->setReadOnly(true);
        le->setStyleSheet("background: rgba(255,255,255,0.05); border: 1px solid rgba(255,255,255,0.1); border-radius: 4px; padding: 4px;");
        form->addRow(label, le);
    };

    addRowText(QStringLiteral("应用名称:"), info.name);
    addRowText(QStringLiteral("唯一标识 (ID):"), info.id);
    addRowText(QStringLiteral("版本号:"), info.version);
    addRowText(QStringLiteral("作者/团队:"), info.author);
    addRowText(QStringLiteral("所属分类:"), info.categoryName);
    
    QString typeName = (info.pluginType == AppPluginType::QtDynamicPlugin) ? QStringLiteral("Qt 动态插件 (.dll)") :
                       (info.isBuiltIn ? QStringLiteral("内置组件") : QStringLiteral("外部工具"));
    addRowText(QStringLiteral("架构类型:"), typeName);

    if (!info.dllPath.isEmpty()) {
        addRowText(QStringLiteral("动态库路径:"), info.dllPath);
        addRowText(QStringLiteral("接口 IID:"), info.interfaceIid.isEmpty() ? IAppPlugin_IID : info.interfaceIid);
    } else if (!info.execPath.isEmpty()) {
        addRowText(QStringLiteral("程序路径:"), info.execPath);
    }

    addRowText(QStringLiteral("功能描述:"), info.description);
    addRowText(QStringLiteral("启动次数:"), QString::number(info.launchCount));

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    form->addRow(box);

    dlg.exec();
}

void PluginManagerDialog::onUninstallClicked(const QString &appId) {
    auto res = QMessageBox::question(this, QStringLiteral("确认卸载"),
        QStringLiteral("确定要卸载插件 [%1] 吗？\n如果为动态库插件，将一并从磁盘中移除对应库文件。").arg(appId),
        QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::Yes) {
        AppManager::instance()->uninstallPlugin(appId);
        refreshPluginList();
    }
}

void PluginManagerDialog::onEnableToggled(const QString &appId, bool enabled) {
    AppManager::instance()->setAppEnabled(appId, enabled);
}
