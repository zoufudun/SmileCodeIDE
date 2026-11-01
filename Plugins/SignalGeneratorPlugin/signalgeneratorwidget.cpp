#include "signalgeneratorwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// Waveform Canvas (实时波形画布)
// =============================================================================

WaveformCanvas::WaveformCanvas(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

void WaveformCanvas::setWaveformParams(int type, double freq, double amp, double phase, double offset, double noise) {
    m_type = type;
    m_freq = freq;
    m_amp = amp;
    m_phase = phase;
    m_offset = offset;
    m_noise = noise;
    update();
}

void WaveformCanvas::updateAnimation() {
    // 每次时间步进 (根据频率动态调整相位步长)
    m_timeOffset += 0.03;
    if (m_timeOffset > 2.0 * M_PI * 1000.0) {
        m_timeOffset = 0.0;
    }
    update();
}

void WaveformCanvas::setDarkTheme(bool dark) {
    m_isDark = dark;
    update();
}

void WaveformCanvas::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();

    // 1. 绘制背景与网格
    QColor bgColor = m_isDark ? QColor("#1e1e2e") : QColor("#f5f6fa");
    painter.fillRect(rect(), bgColor);

    // 网格线
    QColor gridColor = m_isDark ? QColor(255, 255, 255, 18) : QColor(0, 0, 0, 18);
    QColor centerColor = m_isDark ? QColor(255, 255, 255, 45) : QColor(0, 0, 0, 45);

    painter.setPen(QPen(gridColor, 1, Qt::DotLine));
    int gridSpacing = 30;
    for (int x = 0; x < w; x += gridSpacing) {
        painter.drawLine(x, 0, x, h);
    }
    for (int y = 0; y < h; y += gridSpacing) {
        painter.drawLine(0, y, w, y);
    }

    // 中轴中心线 (0V 参考线)
    int centerY = h / 2;
    painter.setPen(QPen(centerColor, 1.2, Qt::DashLine));
    painter.drawLine(0, centerY, w, centerY);

    // 2. 绘制波形
    QPainterPath path;
    bool first = true;

    double maxRangeV = 10.0; // 满量程参考 ±10V
    double scaleY = (h * 0.42) / maxRangeV;
    double radPhase = m_phase * M_PI / 180.0;

    int points = qMax(w, 200);
    for (int i = 0; i < points; ++i) {
        double t = (double)i / points * 4.0 * M_PI; // 显示约 2 个周期的基准跨度
        double phase = t * (m_freq / 5.0) + m_timeOffset + radPhase;

        double val = 0.0;
        switch (m_type) {
        case 0: // 正弦波
            val = std::sin(phase);
            break;
        case 1: // 方波
            val = (std::sin(phase) >= 0.0) ? 1.0 : -1.0;
            break;
        case 2: // 三角波
            val = (2.0 / M_PI) * std::asin(std::sin(phase));
            break;
        case 3: // 锯齿波
            val = (2.0 / M_PI) * std::atan(std::tan(phase / 2.0));
            break;
        case 4: // 白噪声
            val = ((double)rand() / RAND_MAX * 2.0 - 1.0);
            break;
        }

        val = val * m_amp + m_offset;

        // 叠加上百分比噪声
        if (m_noise > 0.001) {
            double n = ((double)rand() / RAND_MAX * 2.0 - 1.0) * (m_noise / 100.0) * m_amp;
            val += n;
        }

        double py = centerY - val * scaleY;
        py = qBound(5.0, py, (double)h - 5.0);

        if (first) {
            path.moveTo(i * (double)w / points, py);
            first = false;
        } else {
            path.lineTo(i * (double)w / points, py);
        }
    }

    // 绘制发光阴影效果
    QColor glowColor(0, 184, 148, 40);
    painter.setPen(QPen(glowColor, 6));
    painter.drawPath(path);

    // 绘制主信号线
    QColor waveColor = m_isDark ? QColor("#00cec9") : QColor("#0984e3");
    painter.setPen(QPen(waveColor, 2.2));
    painter.drawPath(path);

    // 3. 绘制左上角信号信息标牌
    QString infoText = QStringLiteral("CH1: %1 Hz | %2 Vpp | %3 V DC")
        .arg(m_freq, 0, 'f', 1)
        .arg(m_amp * 2.0, 0, 'f', 2)
        .arg(m_offset, 0, 'f', 2);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 140));
    painter.drawRoundedRect(10, 10, 240, 26, 4, 4);

    painter.setPen(QColor("#00cec9"));
    QFont f = painter.font();
    f.setPixelSize(11);
    f.setBold(true);
    painter.setFont(f);
    painter.drawText(QRect(18, 10, 230, 26), Qt::AlignVCenter | Qt::AlignLeft, infoText);
}

// =============================================================================
// Signal Generator Widget (主面板)
// =============================================================================

SignalGeneratorWidget::SignalGeneratorWidget(QWidget *parent) : QWidget(parent) {
    setWindowTitle(QStringLiteral("高频信号发生与模拟器 (Signal Generator Pro)"));
    resize(760, 520);
    setupUi();

    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(33); // 30 FPS
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        if (m_isPlaying) {
            m_canvas->updateAnimation();
        }
    });
    m_animTimer->start();

    onParamsChanged();
}

void SignalGeneratorWidget::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(12);

    // ==========================================
    // 1. 上部：示波器波形画布
    // ==========================================
    QFrame *canvasFrame = new QFrame(this);
    canvasFrame->setFrameShape(QFrame::StyledPanel);
    canvasFrame->setStyleSheet("QFrame { border: 1px solid rgba(255,255,255,0.1); border-radius: 8px; }");
    QVBoxLayout *canvasLayout = new QVBoxLayout(canvasFrame);
    canvasLayout->setContentsMargins(0, 0, 0, 0);

    m_canvas = new WaveformCanvas(canvasFrame);
    canvasLayout->addWidget(m_canvas);
    mainLayout->addWidget(canvasFrame, 1);

    // ==========================================
    // 2. 中部：测量统计指标行
    // ==========================================
    QFrame *statsFrame = new QFrame(this);
    statsFrame->setStyleSheet("background: rgba(255,255,255,0.03); border-radius: 6px; padding: 4px;");
    QHBoxLayout *statsLayout = new QHBoxLayout(statsFrame);
    statsLayout->setContentsMargins(12, 6, 12, 6);

    auto makeStatLabel = [this](const QString &title, QLabel *&valLabel, const QString &initVal) -> QWidget* {
        QWidget *w = new QWidget(this);
        QHBoxLayout *l = new QHBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(6);
        QLabel *tLabel = new QLabel(title, w);
        tLabel->setStyleSheet("color: #95a5a6; font-size: 12px;");
        valLabel = new QLabel(initVal, w);
        valLabel->setStyleSheet("color: #00cec9; font-weight: bold; font-size: 13px;");
        l->addWidget(tLabel);
        l->addWidget(valLabel);
        return w;
    };

    statsLayout->addWidget(makeStatLabel(QStringLiteral("峰峰值 Vpp:"), m_vppLabel, "6.60 V"));
    statsLayout->addWidget(makeStatLabel(QStringLiteral("有效值 Vrms:"), m_vrmsLabel, "2.33 V"));
    statsLayout->addWidget(makeStatLabel(QStringLiteral("基波频率:"), m_freqLabel, "10.0 Hz"));
    statsLayout->addWidget(makeStatLabel(QStringLiteral("输出状态:"), m_statusLabel, QStringLiteral("🟢 发生中")));
    statsLayout->addStretch();

    mainLayout->addWidget(statsFrame);

    // ==========================================
    // 3. 下部：波形发生参数调节
    // ==========================================
    QGroupBox *paramBox = new QGroupBox(QStringLiteral("🎛 信号发生参数配置"), this);
    QGridLayout *grid = new QGridLayout(paramBox);
    grid->setContentsMargins(12, 14, 12, 12);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(10);

    // 波形类型
    QLabel *typeLabel = new QLabel(QStringLiteral("波形类型:"), paramBox);
    m_typeCombo = new QComboBox(paramBox);
    m_typeCombo->addItem(QStringLiteral("🌊 正弦波 (Sine)"), 0);
    m_typeCombo->addItem(QStringLiteral("⚡ 方波 (Square)"), 1);
    m_typeCombo->addItem(QStringLiteral("📐 三角波 (Triangle)"), 2);
    m_typeCombo->addItem(QStringLiteral("📈 锯齿波 (Sawtooth)"), 3);
    m_typeCombo->addItem(QStringLiteral("📻 白噪声 (Noise)"), 4);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SignalGeneratorWidget::onParamsChanged);

    // 频率
    QLabel *freqTitle = new QLabel(QStringLiteral("频率 (Hz):"), paramBox);
    m_freqSpin = new QDoubleSpinBox(paramBox);
    m_freqSpin->setRange(0.1, 1000.0);
    m_freqSpin->setValue(10.0);
    m_freqSpin->setSingleStep(1.0);
    m_freqSlider = new QSlider(Qt::Horizontal, paramBox);
    m_freqSlider->setRange(1, 1000);
    m_freqSlider->setValue(10);
    connect(m_freqSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        m_freqSlider->blockSignals(true);
        m_freqSlider->setValue((int)v);
        m_freqSlider->blockSignals(false);
        onParamsChanged();
    });
    connect(m_freqSlider, &QSlider::valueChanged, this, [this](int v) {
        m_freqSpin->blockSignals(true);
        m_freqSpin->setValue(v);
        m_freqSpin->blockSignals(false);
        onParamsChanged();
    });

    QHBoxLayout *freqLayout = new QHBoxLayout();
    freqLayout->addWidget(m_freqSpin);
    freqLayout->addWidget(m_freqSlider);

    // 幅度
    QLabel *ampTitle = new QLabel(QStringLiteral("幅值 (V):"), paramBox);
    m_ampSpin = new QDoubleSpinBox(paramBox);
    m_ampSpin->setRange(0.1, 10.0);
    m_ampSpin->setValue(3.3);
    m_ampSpin->setSingleStep(0.1);
    m_ampSlider = new QSlider(Qt::Horizontal, paramBox);
    m_ampSlider->setRange(1, 100);
    m_ampSlider->setValue(33);
    connect(m_ampSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        m_ampSlider->blockSignals(true);
        m_ampSlider->setValue((int)(v * 10));
        m_ampSlider->blockSignals(false);
        onParamsChanged();
    });
    connect(m_ampSlider, &QSlider::valueChanged, this, [this](int v) {
        m_ampSpin->blockSignals(true);
        m_ampSpin->setValue(v / 10.0);
        m_ampSpin->blockSignals(false);
        onParamsChanged();
    });

    QHBoxLayout *ampLayout = new QHBoxLayout();
    ampLayout->addWidget(m_ampSpin);
    ampLayout->addWidget(m_ampSlider);

    // 初相位 & DC 偏置
    QLabel *phaseTitle = new QLabel(QStringLiteral("初相位 (°):"), paramBox);
    m_phaseSpin = new QDoubleSpinBox(paramBox);
    m_phaseSpin->setRange(0.0, 360.0);
    m_phaseSpin->setValue(0.0);
    connect(m_phaseSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SignalGeneratorWidget::onParamsChanged);

    QLabel *offsetTitle = new QLabel(QStringLiteral("DC偏置 (V):"), paramBox);
    m_offsetSpin = new QDoubleSpinBox(paramBox);
    m_offsetSpin->setRange(-5.0, 5.0);
    m_offsetSpin->setValue(0.0);
    m_offsetSpin->setSingleStep(0.1);
    connect(m_offsetSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SignalGeneratorWidget::onParamsChanged);

    // 噪声系数
    QLabel *noiseTitle = new QLabel(QStringLiteral("叠加噪声 (%):"), paramBox);
    m_noiseSlider = new QSlider(Qt::Horizontal, paramBox);
    m_noiseSlider->setRange(0, 50);
    m_noiseSlider->setValue(0);
    m_noiseValLabel = new QLabel("0 %", paramBox);
    m_noiseValLabel->setFixedWidth(36);
    connect(m_noiseSlider, &QSlider::valueChanged, this, [this](int v) {
        m_noiseValLabel->setText(QString("%1 %").arg(v));
        onParamsChanged();
    });

    QHBoxLayout *noiseLayout = new QHBoxLayout();
    noiseLayout->addWidget(m_noiseSlider);
    noiseLayout->addWidget(m_noiseValLabel);

    // 布局组装
    grid->addWidget(typeLabel, 0, 0);
    grid->addWidget(m_typeCombo, 0, 1);
    grid->addWidget(freqTitle, 0, 2);
    grid->addLayout(freqLayout, 0, 3);

    grid->addWidget(ampTitle, 1, 0);
    grid->addLayout(ampLayout, 1, 1);
    grid->addWidget(noiseTitle, 1, 2);
    grid->addLayout(noiseLayout, 1, 3);

    grid->addWidget(phaseTitle, 2, 0);
    grid->addWidget(m_phaseSpin, 2, 1);
    grid->addWidget(offsetTitle, 2, 2);
    grid->addWidget(m_offsetSpin, 2, 3);

    mainLayout->addWidget(paramBox);

    // ==========================================
    // 4. 底部：控制按钮
    // ==========================================
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_playBtn = new QPushButton(QStringLiteral("⏸ 暂停发生"), this);
    m_playBtn->setCursor(Qt::PointingHandCursor);
    m_playBtn->setStyleSheet("background-color: #00b894; color: white; font-weight: bold; padding: 6px 16px; border-radius: 6px;");
    connect(m_playBtn, &QPushButton::clicked, this, &SignalGeneratorWidget::onPlayPauseClicked);

    m_resetBtn = new QPushButton(QStringLiteral("🔄 重置参数"), this);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    connect(m_resetBtn, &QPushButton::clicked, this, &SignalGeneratorWidget::onResetClicked);

    m_exportBtn = new QPushButton(QStringLiteral("💾 导出采样点 (CSV)"), this);
    m_exportBtn->setCursor(Qt::PointingHandCursor);
    connect(m_exportBtn, &QPushButton::clicked, this, &SignalGeneratorWidget::onExportClicked);

    btnLayout->addWidget(m_playBtn);
    btnLayout->addWidget(m_resetBtn);
    btnLayout->addWidget(m_exportBtn);
    btnLayout->addStretch();

    mainLayout->addLayout(btnLayout);
}

void SignalGeneratorWidget::onParamsChanged() {
    int type = m_typeCombo->currentData().toInt();
    double freq = m_freqSpin->value();
    double amp = m_ampSpin->value();
    double phase = m_phaseSpin->value();
    double offset = m_offsetSpin->value();
    double noise = m_noiseSlider->value();

    m_canvas->setWaveformParams(type, freq, amp, phase, offset, noise);
    updateStats();
}

void SignalGeneratorWidget::updateStats() {
    double amp = m_ampSpin->value();
    double freq = m_freqSpin->value();
    int type = m_typeCombo->currentData().toInt();

    double vpp = amp * 2.0;
    double vrms = 0.0;
    switch (type) {
    case 0: vrms = amp / std::sqrt(2.0); break; // 正弦
    case 1: vrms = amp; break;                  // 方波
    case 2: vrms = amp / std::sqrt(3.0); break; // 三角
    case 3: vrms = amp / std::sqrt(3.0); break; // 锯齿
    default: vrms = amp * 0.5; break;
    }

    m_vppLabel->setText(QString("%1 V").arg(vpp, 0, 'f', 2));
    m_vrmsLabel->setText(QString("%1 V").arg(vrms, 0, 'f', 2));
    m_freqLabel->setText(QString("%1 Hz").arg(freq, 0, 'f', 1));
}

void SignalGeneratorWidget::onPlayPauseClicked() {
    m_isPlaying = !m_isPlaying;
    if (m_isPlaying) {
        m_playBtn->setText(QStringLiteral("⏸ 暂停发生"));
        m_playBtn->setStyleSheet("background-color: #00b894; color: white; font-weight: bold; padding: 6px 16px; border-radius: 6px;");
        m_statusLabel->setText(QStringLiteral("🟢 发生中"));
    } else {
        m_playBtn->setText(QStringLiteral("▶ 继续发生"));
        m_playBtn->setStyleSheet("background-color: #0984e3; color: white; font-weight: bold; padding: 6px 16px; border-radius: 6px;");
        m_statusLabel->setText(QStringLiteral("⏸ 已暂停"));
    }
}

void SignalGeneratorWidget::onResetClicked() {
    m_typeCombo->setCurrentIndex(0);
    m_freqSpin->setValue(10.0);
    m_ampSpin->setValue(3.3);
    m_phaseSpin->setValue(0.0);
    m_offsetSpin->setValue(0.0);
    m_noiseSlider->setValue(0);
    onParamsChanged();
}

void SignalGeneratorWidget::onExportClicked() {
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出采样点为 CSV 文件"), "waveform_samples.csv", QStringLiteral("CSV 表格 (*.csv)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法保存文件: ") + path);
        return;
    }

    QTextStream out(&file);
    out << "Index,Time_ms,Voltage_V\n";

    double freq = m_freqSpin->value();
    double amp = m_ampSpin->value();
    double phaseRad = m_phaseSpin->value() * M_PI / 180.0;
    double offset = m_offsetSpin->value();
    int type = m_typeCombo->currentData().toInt();

    int sampleCount = 1000;
    double dt = 1.0 / (freq * 50.0); // 50x 过采样

    for (int i = 0; i < sampleCount; ++i) {
        double t = i * dt;
        double p = 2.0 * M_PI * freq * t + phaseRad;
        double val = 0.0;

        switch (type) {
        case 0: val = std::sin(p); break;
        case 1: val = (std::sin(p) >= 0.0) ? 1.0 : -1.0; break;
        case 2: val = (2.0 / M_PI) * std::asin(std::sin(p)); break;
        case 3: val = (2.0 / M_PI) * std::atan(std::tan(p / 2.0)); break;
        case 4: val = ((double)rand() / RAND_MAX * 2.0 - 1.0); break;
        }

        val = val * amp + offset;
        out << i << "," << QString::number(t * 1000.0, 'f', 4) << "," << QString::number(val, 'f', 4) << "\n";
    }

    file.close();
    QMessageBox::information(this, QStringLiteral("导出成功"), QStringLiteral("已成功导出 %1 个高精度采样点数据！").arg(sampleCount));
}

void SignalGeneratorWidget::applyTheme(const QString &themeId) {
    bool isDark = (themeId.compare("light", Qt::CaseInsensitive) != 0);
    m_canvas->setDarkTheme(isDark);
}

