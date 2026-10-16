#include "TargetChipDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QCompleter>
#include <QSplitter>
#include <QScrollArea>
#include <QListView>

TargetChipDialog::TargetChipDialog(QWidget *parent, QSettings *settings, const QString &initialChip)
    : QDialog(parent), m_settings(settings), m_selectedChip(initialChip)
{
    setWindowTitle(QStringLiteral("目标芯片选择 (Target Device Selection)"));
    resize(980, 640);
    setMinimumSize(800, 520);

    initChipData();
    setupUI();
    applyTheme();

    if (m_selectedChip.isEmpty() && m_settings) {
        m_settings->beginGroup("STM32Toolchain");
        m_selectedChip = m_settings->value("selectedChip", "STM32H7x_2M").toString();
        m_settings->endGroup();
    }
    if (m_selectedChip.isEmpty()) {
        m_selectedChip = "STM32H7x_2M";
    }

    setSelectedChip(m_selectedChip);
}

void TargetChipDialog::initChipData()
{
    m_vendors.clear();
    m_vendorFamilies.clear();
    m_familyDevices.clear();
    m_deviceToFamily.clear();
    m_familyToVendor.clear();

    const QString vendorST = QStringLiteral("ST(意法半导体)");
    const QString vendorGD = QStringLiteral("GD(兆易创新)");
    const QString vendorGeneric = QStringLiteral("通用/ARM Cortex-M");

    m_vendors << vendorST << vendorGD << vendorGeneric;

    // 1. ST 意法半导体系列
    const QString famH7 = "STM32H7xx";
    const QString famF4 = "STM32F4xx";
    const QString famF1 = "STM32F10x";
    const QString famF7 = "STM32F7xx";
    const QString famG4 = "STM32G4xx";
    const QString famG0 = "STM32G0xx";
    const QString famF0 = "STM32F0xx";
    const QString famF2 = "STM32F2xx";
    const QString famF3 = "STM32F3xx";
    const QString famL0 = "STM32L0xx";
    const QString famL4 = "STM32L4xx";
    const QString famU5 = "STM32U5xx";
    const QString famWB = "STM32WBxx";

    QStringList stFamilies = { famH7, famF4, famF1, famF7, famG4, famG0,
                               famF0, famF2, famF3, famL0, famL4, famU5, famWB };
    m_vendorFamilies[vendorST] = stFamilies;
    for (const QString &f : stFamilies) {
        m_familyToVendor[f] = vendorST;
    }

    // 常用器件优先置顶排布
    m_familyDevices[famH7] = {
        "STM32H7x_2M", "STM32H7x_1M", "STM32H743VI", "STM32H743VIT6",
        "STM32H743ZI", "STM32H743XI", "STM32H750XB", "STM32H742VI",
        "STM32H747XI", "STM32H7A3VI", "STM32H7B3VI"
    };

    m_familyDevices[famF1] = {
        "STM32F10x_128", "STM32F10x_64", "STM32F10x_512",
        "STM32F103C8", "STM32F103RC", "STM32F103ZE",
        "STM32F103CB", "STM32F103RB", "STM32F103VE"
    };

    m_familyDevices[famF4] = {
        "STM32F4xx_1024", "STM32F4xx_512",
        "STM32F407ZG", "STM32F407VG", "STM32F407VE",
        "STM32F401CC", "STM32F411CE", "STM32F429IG", "STM32F429ZI"
    };

    m_familyDevices[famF7] = {
        "STM32F7x_2M", "STM32F7x_1M", "STM32F767IG", "STM32F767ZI", "STM32F746NG"
    };

    m_familyDevices[famG4] = {
        "STM32G4xx_512", "STM32G474CE", "STM32G431CB", "STM32G474RE"
    };

    m_familyDevices[famG0] = {
        "STM32G070CB", "STM32G030F6", "STM32G071RB"
    };

    m_familyDevices[famF0] = {
        "STM32F051C8", "STM32F072RB", "STM32F030F4"
    };

    m_familyDevices[famF2] = { "STM32F207ZG", "STM32F205RG" };
    m_familyDevices[famF3] = { "STM32F303CC", "STM32F303RE", "STM32F334C8" };
    m_familyDevices[famL0] = { "STM32L053R8", "STM32L073RZ", "STM32L031K6" };
    m_familyDevices[famL4] = { "STM32L476RG", "STM32L431RC", "STM32L496ZG" };
    m_familyDevices[famU5] = { "STM32U575ZI", "STM32U585AI" };
    m_familyDevices[famWB] = { "STM32WB55RG", "STM32WB15CC" };

    // 扫描 STM32ProfileManager 支持列表以合并扩展型号
    const QStringList allChips = STM32ProfileManager::instance().supportedChips();
    for (const QString &chip : allChips) {
        QString targetFamily;
        if (chip.startsWith("STM32H7", Qt::CaseInsensitive)) targetFamily = famH7;
        else if (chip.startsWith("STM32F1", Qt::CaseInsensitive)) targetFamily = famF1;
        else if (chip.startsWith("STM32F4", Qt::CaseInsensitive)) targetFamily = famF4;
        else if (chip.startsWith("STM32F7", Qt::CaseInsensitive)) targetFamily = famF7;
        else if (chip.startsWith("STM32G4", Qt::CaseInsensitive)) targetFamily = famG4;
        else if (chip.startsWith("STM32G0", Qt::CaseInsensitive)) targetFamily = famG0;
        else if (chip.startsWith("STM32F0", Qt::CaseInsensitive)) targetFamily = famF0;
        else if (chip.startsWith("STM32F2", Qt::CaseInsensitive)) targetFamily = famF2;
        else if (chip.startsWith("STM32F3", Qt::CaseInsensitive)) targetFamily = famF3;
        else if (chip.startsWith("STM32L0", Qt::CaseInsensitive)) targetFamily = famL0;
        else if (chip.startsWith("STM32L4", Qt::CaseInsensitive)) targetFamily = famL4;
        else if (chip.startsWith("STM32U5", Qt::CaseInsensitive)) targetFamily = famU5;
        else if (chip.startsWith("STM32WB", Qt::CaseInsensitive)) targetFamily = famWB;

        if (!targetFamily.isEmpty()) {
            if (!m_familyDevices[targetFamily].contains(chip)) {
                m_familyDevices[targetFamily] << chip;
            }
        }
    }

    // 2. GD32 兆易创新系列
    const QString gdF1 = "GD32F10x";
    const QString gdF3 = "GD32F30x";
    const QString gdF4 = "GD32F4xx";
    QStringList gdFamilies = { gdF1, gdF3, gdF4 };
    m_vendorFamilies[vendorGD] = gdFamilies;
    for (const QString &f : gdFamilies) {
        m_familyToVendor[f] = vendorGD;
    }
    m_familyDevices[gdF1] = { "GD32F103C8", "GD32F103RC", "GD32F103ZE", "GD32F103CB" };
    m_familyDevices[gdF3] = { "GD32F303CG", "GD32F303RC", "GD32F303ZE" };
    m_familyDevices[gdF4] = { "GD32F407ZG", "GD32F450ZK", "GD32F470ZI" };

    // 3. 通用 ARM Cortex-M 系列
    const QString genM0 = "ARM Cortex-M0/M0+";
    const QString genM3 = "ARM Cortex-M3";
    const QString genM4 = "ARM Cortex-M4";
    const QString genM7 = "ARM Cortex-M7";
    QStringList genFamilies = { genM0, genM3, genM4, genM7 };
    m_vendorFamilies[vendorGeneric] = genFamilies;
    for (const QString &f : genFamilies) {
        m_familyToVendor[f] = vendorGeneric;
    }
    m_familyDevices[genM0] = { "ARM_Cortex_M0", "ARM_Cortex_M0P" };
    m_familyDevices[genM3] = { "ARM_Cortex_M3" };
    m_familyDevices[genM4] = { "ARM_Cortex_M4" };
    m_familyDevices[genM7] = { "ARM_Cortex_M7" };

    // 建立反向索引：device -> family
    for (auto it = m_familyDevices.begin(); it != m_familyDevices.end(); ++it) {
        const QString family = it.key();
        for (const QString &dev : it.value()) {
            m_deviceToFamily[dev.toUpper()] = family;
        }
    }
}

void TargetChipDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(10);

    // =========================================================================
    // 1. 顶部三级级联选择区 (深色卡片设计，整体融为一体)
    // =========================================================================
    QWidget *headerCard = new QWidget(this);
    headerCard->setObjectName("chipHeaderCard");

    QHBoxLayout *headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(14, 10, 14, 12);
    headerLayout->setSpacing(14);

    // 1) 厂商/分类:
    QVBoxLayout *vendorCol = new QVBoxLayout();
    vendorCol->setSpacing(4);
    QLabel *vendorLabel = new QLabel(QStringLiteral("厂商/分类:"), headerCard);
    vendorLabel->setObjectName("chipHeaderTitle");
    m_vendorCombo = new QComboBox(headerCard);
    m_vendorCombo->setObjectName("chipHeaderCombo");
    m_vendorCombo->setView(new QListView(m_vendorCombo));
    m_vendorCombo->addItems(m_vendors);
    vendorCol->addWidget(vendorLabel);
    vendorCol->addWidget(m_vendorCombo);
    headerLayout->addLayout(vendorCol, 1);

    // 2) 系列:
    QVBoxLayout *familyCol = new QVBoxLayout();
    familyCol->setSpacing(4);
    QLabel *familyLabel = new QLabel(QStringLiteral("系列:"), headerCard);
    familyLabel->setObjectName("chipHeaderTitle");
    m_familyCombo = new QComboBox(headerCard);
    m_familyCombo->setObjectName("chipHeaderCombo");
    m_familyCombo->setView(new QListView(m_familyCombo));
    familyCol->addWidget(familyLabel);
    familyCol->addWidget(m_familyCombo);
    headerLayout->addLayout(familyCol, 1);

    // 3) 器件:
    QVBoxLayout *deviceCol = new QVBoxLayout();
    deviceCol->setSpacing(4);
    QLabel *deviceLabel = new QLabel(QStringLiteral("器件:"), headerCard);
    deviceLabel->setObjectName("chipHeaderTitle");
    m_deviceCombo = new QComboBox(headerCard);
    m_deviceCombo->setObjectName("chipHeaderCombo");
    m_deviceCombo->setView(new QListView(m_deviceCombo));
    m_deviceCombo->setEditable(true);
    m_deviceCombo->setInsertPolicy(QComboBox::NoInsert);
    if (m_deviceCombo->lineEdit()) {
        m_deviceCombo->lineEdit()->setPlaceholderText(QStringLiteral("选择或输入型号 (如 STM32H743VIT6)..."));
    }
    if (m_deviceCombo->completer()) {
        m_deviceCombo->completer()->setCaseSensitivity(Qt::CaseInsensitive);
        m_deviceCombo->completer()->setFilterMode(Qt::MatchContains);
        if (m_deviceCombo->completer()->popup()) {
            m_deviceCombo->completer()->popup()->setStyleSheet(
                "QListView { background-color: #181825; color: #cdd6f4; border: 1px solid #45475a; "
                "selection-background-color: #313244; selection-color: #89b4fa; padding: 4px; font-size: 12px; } "
                "QListView::item { padding: 4px 8px; min-height: 24px; } "
                "QListView::item:selected { background-color: #313244; color: #89b4fa; font-weight: bold; }");
        }
    }
    deviceCol->addWidget(deviceLabel);
    deviceCol->addWidget(m_deviceCombo);
    headerLayout->addLayout(deviceCol, 2);

    mainLayout->addWidget(headerCard);

    // =========================================================================
    // 2. 快速过滤与搜索工具栏
    // =========================================================================
    QHBoxLayout *searchRow = new QHBoxLayout();
    searchRow->setSpacing(8);
    QLabel *searchIcon = new QLabel(QStringLiteral("🔍 快速筛选:"), this);
    searchIcon->setObjectName("searchLabel");
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("searchEdit");
    m_searchEdit->setPlaceholderText(QStringLiteral("输入关键字快速匹配芯片 (如 H743, F103, 2M, C8T6)..."));
    m_searchEdit->setClearButtonEnabled(true);
    searchRow->addWidget(searchIcon);
    searchRow->addWidget(m_searchEdit, 1);
    mainLayout->addLayout(searchRow);

    // =========================================================================
    // 3. 核心区域：左侧器件列表表格 + 右侧硬件规格与调试参数卡片
    // =========================================================================
    QSplitter *centerSplitter = new QSplitter(Qt::Horizontal, this);
    centerSplitter->setObjectName("chipSplitter");

    // 左侧：器件表格
    m_deviceTable = new QTableWidget(centerSplitter);
    m_deviceTable->setObjectName("deviceTable");
    m_deviceTable->setColumnCount(5);
    m_deviceTable->setHorizontalHeaderLabels({
        QStringLiteral("器件型号"),
        QStringLiteral("所属系列"),
        QStringLiteral("CPU 内核"),
        QStringLiteral("Flash 容量"),
        QStringLiteral("RAM 容量")
    });
    m_deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_deviceTable->setAlternatingRowColors(true);
    m_deviceTable->setShowGrid(true);
    m_deviceTable->verticalHeader()->setVisible(false);
    m_deviceTable->verticalHeader()->setDefaultSectionSize(30);
    m_deviceTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_deviceTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_deviceTable->horizontalHeader()->setHighlightSections(false);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_deviceTable->horizontalHeader()->setMinimumSectionSize(65);
    m_deviceTable->setColumnWidth(0, 150);
    m_deviceTable->setColumnWidth(1, 95);
    m_deviceTable->setColumnWidth(2, 135);
    m_deviceTable->setColumnWidth(3, 85);
    m_deviceTable->setColumnWidth(4, 85);

    // 右侧：规格参数详情面板
    QGroupBox *specGroup = new QGroupBox(QStringLiteral("选中芯片硬件参数与调试配置 (Device Profile)"), centerSplitter);
    specGroup->setObjectName("specGroupBox");
    QVBoxLayout *specLayout = new QVBoxLayout(specGroup);
    specLayout->setContentsMargins(14, 16, 14, 14);
    specLayout->setSpacing(10);

    QGridLayout *specGrid = new QGridLayout();
    specGrid->setSpacing(8);
    specGrid->setColumnStretch(1, 1);

    auto addSpecRow = [&](int row, const QString &title, QLabel *&labelPtr) {
        QLabel *titleLabel = new QLabel(title, specGroup);
        titleLabel->setObjectName("specTitleLabel");
        titleLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        labelPtr = new QLabel(QStringLiteral("-"), specGroup);
        labelPtr->setTextInteractionFlags(Qt::TextSelectableByMouse);
        labelPtr->setWordWrap(true);
        labelPtr->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        specGrid->addWidget(titleLabel, row, 0);
        specGrid->addWidget(labelPtr, row, 1);
    };

    addSpecRow(0, QStringLiteral("🎯 目标器件:"), m_specChipLabel);
    addSpecRow(1, QStringLiteral("🏢 厂商/系列:"), m_specVendorFamilyLabel);
    addSpecRow(2, QStringLiteral("⚡ CPU 内核:"), m_specCoreLabel);
    addSpecRow(3, QStringLiteral("💾 Flash 规格:"), m_specFlashLabel);
    addSpecRow(4, QStringLiteral("🧠 RAM 规格:"), m_specRamLabel);
    addSpecRow(5, QStringLiteral("⚙ 编译器宏:"), m_specMacroLabel);
    addSpecRow(6, QStringLiteral("📄 链接脚本:"), m_specLdLabel);
    addSpecRow(7, QStringLiteral("🔌 调试器目标:"), m_specDebuggerLabel);

    specLayout->addLayout(specGrid);
    specLayout->addStretch(1);

    QScrollArea *specScroll = new QScrollArea(centerSplitter);
    specScroll->setObjectName("specScrollArea");
    specScroll->setWidgetResizable(true);
    specScroll->setFrameShape(QFrame::NoFrame);
    specScroll->setWidget(specGroup);

    centerSplitter->addWidget(m_deviceTable);
    centerSplitter->addWidget(specScroll);
    centerSplitter->setStretchFactor(0, 5);
    centerSplitter->setStretchFactor(1, 4);
    centerSplitter->setSizes({ 570, 390 });

    mainLayout->addWidget(centerSplitter, 1);

    // =========================================================================
    // 4. 底部状态与操作按钮
    // =========================================================================
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    QLabel *tipLabel = new QLabel(QStringLiteral("💡 双击表格型号或点击【确定选择】将该芯片设为当前工程目标"), this);
    tipLabel->setObjectName("tipLabel");
    btnLayout->addWidget(tipLabel);
    btnLayout->addStretch();

    m_okBtn = new QPushButton(QStringLiteral("确定选择"), this);
    m_okBtn->setObjectName("okBtn");
    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    m_cancelBtn->setObjectName("cancelBtn");

    btnLayout->addWidget(m_okBtn);
    btnLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(btnLayout);

    // =========================================================================
    // 信号槽连接
    // =========================================================================
    connect(m_vendorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TargetChipDialog::onVendorChanged);
    connect(m_familyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TargetChipDialog::onFamilyChanged);
    connect(m_deviceCombo, &QComboBox::currentTextChanged,
            this, &TargetChipDialog::onDeviceChanged);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &TargetChipDialog::onSearchTextChanged);
    connect(m_deviceTable, &QTableWidget::itemSelectionChanged,
            this, &TargetChipDialog::onTableSelectionChanged);
    connect(m_deviceTable, &QTableWidget::cellDoubleClicked,
            this, &TargetChipDialog::onTableDoubleClicked);

    connect(m_okBtn, &QPushButton::clicked, this, &TargetChipDialog::onAcceptClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void TargetChipDialog::applyTheme()
{
    setStyleSheet(
        "QDialog {"
        "    background-color: #1e1e2e;"
        "    color: #cdd6f4;"
        "    font-family: 'Segoe UI', 'Microsoft YaHei', 'PingFang SC', sans-serif;"
        "}"
        // 顶部级联选择卡片
        "QWidget#chipHeaderCard {"
        "    background-color: #181825;"
        "    border: 1px solid #313244;"
        "    border-radius: 8px;"
        "}"
        "QLabel#chipHeaderTitle {"
        "    color: #89b4fa;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "}"
        // 下拉选择框整体风格：深色融为一体，高对比度清晰文字
        "QComboBox#chipHeaderCombo {"
        "    background-color: #1e1e2e;"
        "    color: #f5f6fa;"
        "    border: 1px solid #45475a;"
        "    border-radius: 6px;"
        "    padding: 4px 10px;"
        "    min-height: 28px;"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "    font-family: 'Consolas', 'Segoe UI', 'Microsoft YaHei', sans-serif;"
        "}"
        "QComboBox#chipHeaderCombo:hover {"
        "    border: 1px solid #89b4fa;"
        "    background-color: #24273a;"
        "}"
        "QComboBox#chipHeaderCombo:focus {"
        "    border: 1px solid #89b4fa;"
        "    background-color: #24273a;"
        "}"
        "QComboBox#chipHeaderCombo::drop-down {"
        "    subcontrol-origin: padding;"
        "    subcontrol-position: top right;"
        "    width: 22px;"
        "    border-left: 1px solid #313244;"
        "    border-top-right-radius: 5px;"
        "    border-bottom-right-radius: 5px;"
        "    background: transparent;"
        "}"
        "QComboBox#chipHeaderCombo::down-arrow {"
        "    image: url(:/stm32/ui/arrow_down.png);"
        "    width: 10px;"
        "    height: 10px;"
        "    margin-right: 6px;"
        "}"
        // 下拉菜单弹出视图
        "QComboBox QAbstractItemView {"
        "    background-color: #181825;"
        "    color: #cdd6f4;"
        "    border: 1px solid #45475a;"
        "    border-radius: 6px;"
        "    selection-background-color: #313244;"
        "    selection-color: #89b4fa;"
        "    outline: none;"
        "    padding: 4px;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "    min-height: 26px;"
        "    padding: 4px 8px;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "    background-color: #313244;"
        "    color: #89b4fa;"
        "    font-weight: bold;"
        "}"
        // 可编辑下拉框内部单行编辑框
        "QComboBox#chipHeaderCombo QLineEdit {"
        "    background-color: transparent;"
        "    color: #f5f6fa;"
        "    border: none;"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "    font-family: 'Consolas', monospace;"
        "    selection-background-color: #585b70;"
        "    selection-color: #ffffff;"
        "}"
        // 快速筛选栏
        "QLabel#searchLabel {"
        "    color: #89b4fa;"
        "    font-weight: bold;"
        "    font-size: 13px;"
        "}"
        "QLineEdit#searchEdit {"
        "    background-color: #181825;"
        "    border: 1px solid #313244;"
        "    border-radius: 6px;"
        "    padding: 6px 12px;"
        "    color: #f5f6fa;"
        "    font-size: 12px;"
        "    selection-background-color: #585b70;"
        "    selection-color: #ffffff;"
        "}"
        "QLineEdit#searchEdit:hover {"
        "    border: 1px solid #45475a;"
        "}"
        "QLineEdit#searchEdit:focus {"
        "    border: 1px solid #89b4fa;"
        "    background-color: #1e1e2e;"
        "}"
        // 分割条
        "QSplitter::handle {"
        "    background-color: #313244;"
        "    width: 3px;"
        "}"
        "QSplitter::handle:hover {"
        "    background-color: #89b4fa;"
        "}"
        // 左侧器件表格
        "QTableWidget#deviceTable {"
        "    background-color: #11111b;"
        "    alternate-background-color: #181825;"
        "    color: #cdd6f4;"
        "    border: 1px solid #313244;"
        "    border-radius: 8px;"
        "    gridline-color: #26293b;"
        "    outline: none;"
        "    selection-background-color: #2b3b5c;"
        "    selection-color: #ffffff;"
        "}"
        "QTableWidget#deviceTable::item {"
        "    padding: 4px 8px;"
        "    border: none;"
        "}"
        "QTableWidget#deviceTable::item:selected {"
        "    background-color: #2b3b5c;"
        "    color: #ffffff;"
        "    font-weight: bold;"
        "}"
        "QTableWidget#deviceTable::item:hover {"
        "    background-color: #1f2233;"
        "}"
        "QHeaderView::section {"
        "    background-color: #181825;"
        "    color: #89b4fa;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "    padding: 6px 8px;"
        "    border: none;"
        "    border-right: 1px solid #313244;"
        "    border-bottom: 1px solid #313244;"
        "}"
        "QTableCornerButton::section {"
        "    background-color: #181825;"
        "    border: none;"
        "}"
        // 滚动条样式（与暗色主题融为一体）
        "QScrollBar:vertical {"
        "    background-color: #181825;"
        "    width: 8px;"
        "    margin: 0px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background-color: #45475a;"
        "    min-height: 24px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background-color: #585b70;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "    border: none;"
        "    background: none;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "    background: none;"
        "}"
        "QScrollBar:horizontal {"
        "    background-color: #181825;"
        "    height: 8px;"
        "    margin: 0px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background-color: #45475a;"
        "    min-width: 24px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "    background-color: #585b70;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "    width: 0px;"
        "    border: none;"
        "    background: none;"
        "}"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
        "    background: none;"
        "}"
        // 右侧硬件详情面板
        "QScrollArea#specScrollArea {"
        "    background-color: transparent;"
        "    border: none;"
        "}"
        "QGroupBox#specGroupBox {"
        "    background-color: #181825;"
        "    border: 1px solid #313244;"
        "    border-radius: 8px;"
        "    margin-top: 10px;"
        "    padding-top: 18px;"
        "    padding-left: 12px;"
        "    padding-right: 12px;"
        "    padding-bottom: 12px;"
        "    color: #89b4fa;"
        "    font-weight: bold;"
        "    font-size: 13px;"
        "}"
        "QGroupBox#specGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 12px;"
        "    padding: 0 6px;"
        "    background-color: #181825;"
        "    color: #89b4fa;"
        "}"
        "QLabel#specTitleLabel {"
        "    color: #a6adc8;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "}"
        // 底部提示与按钮
        "QLabel#tipLabel {"
        "    color: #a6adc8;"
        "    font-size: 12px;"
        "}"
        "QPushButton#okBtn {"
        "    background-color: #3867d6;"
        "    color: #ffffff;"
        "    border: 1px solid #4b7bec;"
        "    border-radius: 6px;"
        "    padding: 7px 22px;"
        "    font-weight: bold;"
        "    font-size: 13px;"
        "}"
        "QPushButton#okBtn:hover {"
        "    background-color: #4b7bec;"
        "}"
        "QPushButton#okBtn:pressed {"
        "    background-color: #2b56b8;"
        "}"
        "QPushButton#cancelBtn {"
        "    background-color: #313244;"
        "    color: #cdd6f4;"
        "    border: 1px solid #45475a;"
        "    border-radius: 6px;"
        "    padding: 7px 18px;"
        "    font-size: 13px;"
        "}"
        "QPushButton#cancelBtn:hover {"
        "    background-color: #45475a;"
        "    color: #ffffff;"
        "}"
        "QPushButton#cancelBtn:pressed {"
        "    background-color: #252739;"
        "}"
        // 优雅深色滚动条
        "QScrollBar:vertical {"
        "    background: #11111b;"
        "    width: 8px;"
        "    margin: 0;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: #313244;"
        "    min-height: 24px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: #45475a;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0;"
        "}"
        "QScrollBar:horizontal {"
        "    background: #11111b;"
        "    height: 8px;"
        "    margin: 0;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background: #313244;"
        "    min-width: 24px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "    background: #45475a;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "    width: 0;"
        "}"
    );
}

void TargetChipDialog::setSelectedChip(const QString &chip)
{
    if (chip.trimmed().isEmpty()) return;
    m_selectedChip = chip.trimmed();

    QString upper = m_selectedChip.toUpper();
    QString family;
    if (m_deviceToFamily.contains(upper)) {
        family = m_deviceToFamily.value(upper);
    } else {
        // 尝试前缀匹配识别系列
        if (upper.startsWith("STM32H7")) family = "STM32H7xx";
        else if (upper.startsWith("STM32F4")) family = "STM32F4xx";
        else if (upper.startsWith("STM32F1")) family = "STM32F10x";
        else if (upper.startsWith("STM32F7")) family = "STM32F7xx";
        else if (upper.startsWith("STM32G4")) family = "STM32G4xx";
        else if (upper.startsWith("STM32G0")) family = "STM32G0xx";
        else if (upper.startsWith("STM32F0")) family = "STM32F0xx";
        else if (upper.startsWith("GD32F1")) family = "GD32F10x";
        else if (upper.startsWith("GD32F3")) family = "GD32F30x";
        else if (upper.startsWith("GD32F4")) family = "GD32F4xx";
        else family = "STM32H7xx";
    }

    QString vendor = m_familyToVendor.value(family, QStringLiteral("ST(意法半导体)"));

    m_updatingUi = true;
    int vIdx = m_vendorCombo->findText(vendor);
    if (vIdx >= 0) m_vendorCombo->setCurrentIndex(vIdx);
    updateFamilyCombo();

    int fIdx = m_familyCombo->findText(family);
    if (fIdx >= 0) m_familyCombo->setCurrentIndex(fIdx);
    updateDeviceCombo();

    int dIdx = m_deviceCombo->findText(m_selectedChip);
    if (dIdx >= 0) {
        m_deviceCombo->setCurrentIndex(dIdx);
    } else {
        m_deviceCombo->setCurrentText(m_selectedChip);
    }

    updateDeviceTable();
    updateChipSpecs(m_selectedChip);
    m_updatingUi = false;
}

QString TargetChipDialog::getSelectedChip() const
{
    return m_selectedChip;
}

bool TargetChipDialog::selectChip(QWidget *parent, QString &chipInOut, QSettings *settings)
{
    TargetChipDialog dlg(parent, settings, chipInOut);
    if (dlg.exec() == QDialog::Accepted) {
        chipInOut = dlg.getSelectedChip();
        return true;
    }
    return false;
}

void TargetChipDialog::onVendorChanged(int index)
{
    Q_UNUSED(index);
    if (m_updatingUi) return;
    updateFamilyCombo();
    updateDeviceCombo();
    updateDeviceTable();
}

void TargetChipDialog::onFamilyChanged(int index)
{
    Q_UNUSED(index);
    if (m_updatingUi) return;
    updateDeviceCombo();
    updateDeviceTable();
}

void TargetChipDialog::updateFamilyCombo()
{
    QString vendor = m_vendorCombo->currentText();
    QStringList families = m_vendorFamilies.value(vendor);

    m_familyCombo->blockSignals(true);
    m_familyCombo->clear();
    m_familyCombo->addItems(families);
    m_familyCombo->blockSignals(false);
}

void TargetChipDialog::updateDeviceCombo()
{
    QString family = m_familyCombo->currentText();
    QStringList devices = m_familyDevices.value(family);

    m_deviceCombo->blockSignals(true);
    m_deviceCombo->clear();
    m_deviceCombo->addItems(devices);
    if (!devices.isEmpty()) {
        m_deviceCombo->setCurrentIndex(0);
        m_selectedChip = devices.first();
    }
    m_deviceCombo->blockSignals(false);

    updateChipSpecs(m_deviceCombo->currentText());
}

void TargetChipDialog::onDeviceChanged(const QString &device)
{
    if (m_updatingUi) return;
    if (device.trimmed().isEmpty()) return;
    m_selectedChip = device.trimmed();
    updateChipSpecs(m_selectedChip);

    // 同步表格选中行
    for (int r = 0; r < m_deviceTable->rowCount(); ++r) {
        QTableWidgetItem *item = m_deviceTable->item(r, 0);
        if (item && item->text().compare(m_selectedChip, Qt::CaseInsensitive) == 0) {
            m_deviceTable->blockSignals(true);
            m_deviceTable->selectRow(r);
            m_deviceTable->scrollToItem(item);
            m_deviceTable->blockSignals(false);
            break;
        }
    }
}

void TargetChipDialog::updateDeviceTable()
{
    QString family = m_familyCombo->currentText();
    QStringList devices = m_familyDevices.value(family);

    m_deviceTable->blockSignals(true);
    m_deviceTable->setRowCount(devices.size());

    int selectedRow = -1;
    for (int i = 0; i < devices.size(); ++i) {
        const QString &dev = devices.at(i);
        STM32ChipProfile p = STM32ProfileManager::instance().getProfile(dev);

        QString core = !p.core.isEmpty() ? p.core : (p.hasFpu ? "cortex-m7" : "cortex-m");
        QString displayCore = core;
        if (displayCore.startsWith("cortex-", Qt::CaseInsensitive)) {
            displayCore = "Cortex-" + displayCore.mid(7).toUpper();
        } else if (displayCore.startsWith("arm_", Qt::CaseInsensitive)) {
            displayCore = displayCore.mid(4).replace('_', ' ');
        }
        if (p.hasFpu && !displayCore.contains("FPU", Qt::CaseInsensitive)) {
            displayCore += " (FPU)";
        }

        QString flashStr = p.flashSizeBytes > 0
            ? QString("%1 KB").arg(p.flashSizeBytes / 1024)
            : QStringLiteral("通用/自动");
        QString ramStr = p.ramSizeBytes > 0
            ? QString("%1 KB").arg(p.ramSizeBytes / 1024)
            : QStringLiteral("通用/自动");

        QTableWidgetItem *itemDev = new QTableWidgetItem(dev);
        itemDev->setFont(QFont("Consolas", 10, QFont::Bold));
        itemDev->setForeground(QColor("#ffffff"));
        itemDev->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        QTableWidgetItem *itemFam = new QTableWidgetItem(family);
        itemFam->setFont(QFont("Segoe UI", 9));
        itemFam->setForeground(QColor("#a6adc8"));
        itemFam->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem *itemCore = new QTableWidgetItem(displayCore);
        itemCore->setFont(QFont("Consolas", 9, QFont::Bold));
        itemCore->setForeground(QColor("#89b4fa"));
        itemCore->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem *itemFlash = new QTableWidgetItem(flashStr);
        itemFlash->setFont(QFont("Consolas", 9));
        itemFlash->setForeground(QColor("#a6e3a1"));
        itemFlash->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem *itemRam = new QTableWidgetItem(ramStr);
        itemRam->setFont(QFont("Consolas", 9));
        itemRam->setForeground(QColor("#a6e3a1"));
        itemRam->setTextAlignment(Qt::AlignCenter);

        m_deviceTable->setItem(i, 0, itemDev);
        m_deviceTable->setItem(i, 1, itemFam);
        m_deviceTable->setItem(i, 2, itemCore);
        m_deviceTable->setItem(i, 3, itemFlash);
        m_deviceTable->setItem(i, 4, itemRam);

        if (dev.compare(m_selectedChip, Qt::CaseInsensitive) == 0) {
            selectedRow = i;
        }
    }

    if (selectedRow >= 0) {
        m_deviceTable->selectRow(selectedRow);
    } else if (devices.size() > 0) {
        m_deviceTable->selectRow(0);
    }

    m_deviceTable->blockSignals(false);

    // 重新应用搜索筛选
    if (!m_searchEdit->text().trimmed().isEmpty()) {
        onSearchTextChanged(m_searchEdit->text());
    }
}

void TargetChipDialog::onSearchTextChanged(const QString &text)
{
    QString filter = text.trimmed();
    bool hasFilter = !filter.isEmpty();

    for (int r = 0; r < m_deviceTable->rowCount(); ++r) {
        bool match = !hasFilter;
        if (hasFilter) {
            for (int c = 0; c < m_deviceTable->columnCount(); ++c) {
                QTableWidgetItem *item = m_deviceTable->item(r, c);
                if (item && item->text().contains(filter, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
        }
        m_deviceTable->setRowHidden(r, !match);
    }
}

void TargetChipDialog::onTableSelectionChanged()
{
    int row = m_deviceTable->currentRow();
    if (row < 0) return;
    QTableWidgetItem *item = m_deviceTable->item(row, 0);
    if (!item) return;

    QString dev = item->text();
    m_selectedChip = dev;

    m_deviceCombo->blockSignals(true);
    int idx = m_deviceCombo->findText(dev);
    if (idx >= 0) m_deviceCombo->setCurrentIndex(idx);
    else m_deviceCombo->setCurrentText(dev);
    m_deviceCombo->blockSignals(false);

    updateChipSpecs(dev);
}

void TargetChipDialog::onTableDoubleClicked(int row, int column)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    onAcceptClicked();
}

void TargetChipDialog::updateChipSpecs(const QString &chipName)
{
    m_specChipLabel->setText(chipName.isEmpty() ? QStringLiteral("未选择") : chipName);
    m_specChipLabel->setStyleSheet(
        "color: #89dceb; font-weight: bold; font-family: Consolas, monospace; font-size: 13px; "
        "background-color: #11111b; border: 1px solid #313244; border-radius: 4px; padding: 3px 8px;");

    m_specVendorFamilyLabel->setText(
        QString("%1 · %2").arg(m_vendorCombo->currentText()).arg(m_familyCombo->currentText()));
    m_specVendorFamilyLabel->setStyleSheet("color: #cdd6f4; font-weight: bold; font-size: 12px;");

    STM32ChipProfile p = STM32ProfileManager::instance().getProfile(chipName);
    if (!p.chipName.isEmpty()) {
        QString coreStr = p.core;
        if (coreStr.startsWith("cortex-", Qt::CaseInsensitive)) {
            coreStr = "Cortex-" + coreStr.mid(7).toUpper();
        }
        if (p.hasFpu) {
            coreStr += " (硬件浮点 FPU: " + p.fpuFlags + ")";
        }
        m_specCoreLabel->setText(coreStr);
        m_specCoreLabel->setStyleSheet("color: #f9e2af; font-weight: bold; font-family: Consolas, monospace; font-size: 12px;");

        QString flashStr = QString("%1 KB").arg(p.flashSizeBytes / 1024);
        if (p.flashSizeBytes >= 1024 * 1024) {
            flashStr += QString(" (%1 MB)").arg(p.flashSizeBytes / (1024.0 * 1024), 0, 'f', 1);
        }
        flashStr += " @ " + p.flashOrigin;
        m_specFlashLabel->setText(flashStr);
        m_specFlashLabel->setStyleSheet("color: #a6e3a1; font-weight: bold; font-family: Consolas, monospace; font-size: 12px;");

        QString ramStr = QString("%1 KB").arg(p.ramSizeBytes / 1024);
        if (p.linkerRamSizeBytes > 0 && p.linkerRamSizeBytes != p.ramSizeBytes) {
            ramStr += QString(" (DTCM连续段: %1 KB)").arg(p.linkerRamSizeBytes / 1024);
        }
        ramStr += " @ " + p.ramOrigin;
        m_specRamLabel->setText(ramStr);
        m_specRamLabel->setStyleSheet("color: #a6e3a1; font-weight: bold; font-family: Consolas, monospace; font-size: 12px;");

        QString macroStr = p.defaultMacro;
        if (!p.extraMacros.isEmpty()) {
            macroStr += " " + p.extraMacros.join(" ");
        }
        m_specMacroLabel->setText(macroStr.isEmpty() ? QStringLiteral("默认无") : macroStr);
        m_specMacroLabel->setStyleSheet("color: #fab387; font-weight: bold; font-family: Consolas, monospace; font-size: 11px;");

        QString ldStr = p.defaultLdFile;
        if (!p.linkerResourcePath.isEmpty()) {
            ldStr += QStringLiteral(" (CubeMX 官方原版内置脚本)");
        } else {
            ldStr += QStringLiteral(" (SmileCode GCC 规范智能链接脚本)");
        }
        m_specLdLabel->setText(ldStr);
        m_specLdLabel->setStyleSheet("color: #89b4fa; font-family: Consolas, monospace; font-size: 11px;");

        QString dbgStr = QString("OpenOCD: %1\nJ-Link: %2")
            .arg(p.openocdTargetCfg.isEmpty() ? QStringLiteral("自动适配") : p.openocdTargetCfg)
            .arg(p.jlinkDeviceName.isEmpty() ? p.chipName : p.jlinkDeviceName);
        m_specDebuggerLabel->setText(dbgStr);
        m_specDebuggerLabel->setStyleSheet("color: #cba6f7; font-family: Consolas, monospace; font-size: 11px;");
    } else {
        // 外部通用或尚未定义具体宏的型号
        m_specCoreLabel->setText(QStringLiteral("ARM Cortex-M (根据工程配置自适应)"));
        m_specCoreLabel->setStyleSheet("color: #f9e2af; font-family: Consolas, monospace; font-size: 12px;");

        m_specFlashLabel->setText(QStringLiteral("自动检测 / 连续 0x08000000 区域"));
        m_specFlashLabel->setStyleSheet("color: #a6e3a1; font-family: Consolas, monospace; font-size: 12px;");

        m_specRamLabel->setText(QStringLiteral("自动检测 / 连续 0x20000000 区域"));
        m_specRamLabel->setStyleSheet("color: #a6e3a1; font-family: Consolas, monospace; font-size: 12px;");

        m_specMacroLabel->setText(QStringLiteral("标准 HAL / CMSIS 头文件宏"));
        m_specMacroLabel->setStyleSheet("color: #fab387; font-family: Consolas, monospace; font-size: 11px;");

        m_specLdLabel->setText(QStringLiteral("自动生成标准 GNU LD 链接脚本"));
        m_specLdLabel->setStyleSheet("color: #89b4fa; font-family: Consolas, monospace; font-size: 11px;");

        m_specDebuggerLabel->setText(QStringLiteral("OpenOCD: target cfg (自动匹配)\nJ-Link / CMSIS-DAP / ST-Link"));
        m_specDebuggerLabel->setStyleSheet("color: #cba6f7; font-family: Consolas, monospace; font-size: 11px;");
    }
}

void TargetChipDialog::onAcceptClicked()
{
    QString chip = m_deviceCombo->currentText().trimmed();
    if (chip.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择或输入有效的目标芯片型号!"));
        return;
    }

    m_selectedChip = chip;
    if (m_settings) {
        m_settings->beginGroup("STM32Toolchain");
        m_settings->setValue("selectedChip", m_selectedChip);
        m_settings->endGroup();
    }

    accept();
}
