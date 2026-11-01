#ifndef SIGNALGENERATORWIDGET_H
#define SIGNALGENERATORWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QVector>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

class WaveformCanvas : public QWidget {
    Q_OBJECT
public:
    explicit WaveformCanvas(QWidget *parent = nullptr);
    void setWaveformParams(int type, double freq, double amp, double phase, double offset, double noise);
    void updateAnimation();
    void setDarkTheme(bool dark);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_type = 0;       // 0: 正弦波, 1: 方波, 2: 三角波, 3: 锯齿波, 4: 白噪声
    double m_freq = 10.0; // Hz
    double m_amp = 3.3;   // V
    double m_phase = 0.0; // deg
    double m_offset = 0.0;// V
    double m_noise = 0.0; // %
    double m_timeOffset = 0.0;
    bool m_isDark = true;
};

class SignalGeneratorWidget : public QWidget {
    Q_OBJECT

public:
    explicit SignalGeneratorWidget(QWidget *parent = nullptr);
    ~SignalGeneratorWidget() override = default;

    void applyTheme(const QString &themeId);

private slots:
    void onParamsChanged();
    void onPlayPauseClicked();
    void onResetClicked();
    void onExportClicked();

private:
    void setupUi();
    void updateStats();

    WaveformCanvas *m_canvas;
    QTimer *m_animTimer;
    bool m_isPlaying = true;

    // Controls
    QComboBox *m_typeCombo;
    QDoubleSpinBox *m_freqSpin;
    QSlider *m_freqSlider;
    QDoubleSpinBox *m_ampSpin;
    QSlider *m_ampSlider;
    QDoubleSpinBox *m_phaseSpin;
    QDoubleSpinBox *m_offsetSpin;
    QSlider *m_noiseSlider;
    QLabel *m_noiseValLabel;

    QPushButton *m_playBtn;
    QPushButton *m_resetBtn;
    QPushButton *m_exportBtn;

    // Stats
    QLabel *m_vppLabel;
    QLabel *m_vrmsLabel;
    QLabel *m_freqLabel;
    QLabel *m_statusLabel;
};

#endif // SIGNALGENERATORWIDGET_H

