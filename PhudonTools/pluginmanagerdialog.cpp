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
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

PluginManagerDialog::PluginManagerDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("插件与扩展中心 (Plugin & Extension Hub)"));
    resize(850, 520);
    setMinimumSize(700, 420);

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
    topLayout->setSpacing(10);

    m_installJsonBtn = new QPushButton(QStringLiteral("➕ 安装插件 (.json)"), this);
    m_installJsonBtn->setCursor(Qt::PointingHandCursor);
    m_installJsonBtn->setStyleSheet("background-color: #6c5ce7; color: white; font-weight: bold; padding: 6px 14px; border-radius: 6px;");
    connect(m_installJsonBtn, &QPushButton::clicked, this, &PluginManagerDialog::onInstallJsonClicked);

    m_createToolBtn = new QPushButton(QStringLiteral("🛠 注册外部工具"), this);
    m_createToolBtn->setCursor(Qt::PointingHandCursor);
    m_createToolBtn->setStyleSheet("background-color: #0984e3; color: white; font-weight: bold; padding: 6px 14px; border-radius: 6px;");
    connect(m_createToolBtn, &QPushButton::clicked, this, &PluginManagerDialog::onCreateCustomToolClicked);

    m_openDirBtn = new QPushButton(QStringLiteral("📂 插件目录"), this);
    m_openDirBtn->setCursor(Qt::PointingHandCursor);
    connect(m_openDirBtn, &QPushButton::clicked, this, &PluginManagerDialog::onOpenPluginsDirClicked);

    m_refreshBtn = new QPushButton(QStringLiteral("🔄 刷新"), this);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshBtn, &QPushButton::clicked, this, &PluginManagerDialog::refreshPluginList);

    m_countLabel = new QLabel(this);
    m_countLabel->setStyleSheet("color: #95a5a6; font-size: 12px;");

    topLayout->addWidget(m_installJsonBtn);
    topLayout->addWidget(m_createToolBtn);
    topLayout->addWidget(m_openDirBtn);
    topLayout->addWidget(m_refreshBtn);
    topLayout->addStretch();
    topLayout->addWidget(m_countLabel);

    mainLayout->addLayout(topLayout);

    // ==========================================
    // 插件列表表格
    // ==========================================
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(7);
    m_tableWidget->setHorizontalHeaderLabels(QStringList()
        << QStringLiteral("状态")
        << QStringLiteral("应用 / 插件名称")
        << QStringLiteral("类型")
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
    int pluginCount = 0;

    for (const AppInfo &app : apps) {
        if (!app.isBuiltIn) pluginCount++;

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
        nameItem->setToolTip(app.description);
        m_tableWidget->setItem(row, 1, nameItem);

        // 3. 类型
        QString typeStr;
        if (app.isBuiltIn) {
            typeStr = QStringLiteral("内置组件");
        } else if (app.pluginType == AppPluginType::ExternalExecutable) {
            typeStr = QStringLiteral("外部程序");
        } else {
            typeStr = QStringLiteral("脚本工具");
        }
        QTableWidgetItem *typeItem = new QTableWidgetItem(typeStr);
        typeItem->setTextAlignment(Qt::AlignCenter);
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

        // 7. 操作按钮
        QWidget *actionWidget = new QWidget(this);
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(4, 2, 4, 2);
        actionLayout->setAlignment(Qt::AlignCenter);

        if (!app.isBuiltIn) {
            QPushButton *uninstallBtn = new QPushButton(QStringLiteral("卸载"), actionWidget);
            uninstallBtn->setCursor(Qt::PointingHandCursor);
            uninstallBtn->setStyleSheet("color: #e74c3c; background: rgba(231, 76, 60, 0.1); border: 1px solid rgba(231, 76, 60, 0.3); border-radius: 4px; padding: 2px 8px;");
            connect(uninstallBtn, &QPushButton::clicked, this, [this, appId]() {
                onUninstallClicked(appId);
            });
            actionLayout->addWidget(uninstallBtn);
        } else {
            QLabel *lockedLabel = new QLabel(QStringLiteral("核心系统"), actionWidget);
            lockedLabel->setStyleSheet("color: #7f8c8d; font-size: 11px;");
            actionLayout->addWidget(lockedLabel);
        }

        m_tableWidget->setCellWidget(row, 6, actionWidget);
        row++;
    }

    m_countLabel->setText(QStringLiteral("已注册应用: %1 个 (内置: %2, 扩展插件: %3)")
        .arg(apps.size()).arg(apps.size() - pluginCount).arg(pluginCount));
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

void PluginManagerDialog::onUninstallClicked(const QString &appId) {
    auto res = QMessageBox::question(this, QStringLiteral("确认卸载"),
        QStringLiteral("确定要卸载插件 [%1] 吗？").arg(appId),
        QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::Yes) {
        AppManager::instance()->uninstallPlugin(appId);
    }
}

void PluginManagerDialog::onEnableToggled(const QString &appId, bool enabled) {
    AppManager::instance()->setAppEnabled(appId, enabled);
}
