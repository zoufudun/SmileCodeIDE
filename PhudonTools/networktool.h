#ifndef NETWORKTOOL_H
#define NETWORKTOOL_H

#include <QWidget>
#include <QTcpSocket>
#include <QTcpServer>
#include <QUdpSocket>
#include <QTimer>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QCheckBox>
#include <QLabel>
#include <QListWidget>

class NetworkTool : public QWidget {
    Q_OBJECT

public:
    enum class ProtocolType {
        TCP_Client = 0,
        TCP_Server,
        UDP
    };

    explicit NetworkTool(QWidget *parent = nullptr);
    ~NetworkTool() override;
    void applyTheme(const QString &themeId);

private slots:
    void onProtocolChanged(int index);
    void onConnectClicked();
    void onSendClicked();
    void onClearReceiveClicked();
    void onClearSendClicked();
    void onSaveLogClicked();
    void onAutoSendToggled(bool checked);
    void onAutoSendTimerTimeout();

    // TCP Client slots
    void onTcpClientConnected();
    void onTcpClientDisconnected();
    void onTcpClientReadyRead();
    void onTcpClientError();

    // TCP Server slots
    void onTcpServerNewConnection();
    void onServerClientReadyRead();
    void onServerClientDisconnected();

    // UDP slots
    void onUdpReadyRead();

private:
    void setupUi();
    void updateUiState(bool connected);
    void appendLog(const QString &source, const QByteArray &data, bool isSend = false);
    QByteArray parseSendData();

    // UI Controls
    QComboBox *m_protocolCombo;
    QLineEdit *m_localIpEdit;
    QSpinBox *m_localPortSpin;
    QLineEdit *m_remoteIpEdit;
    QSpinBox *m_remotePortSpin;
    QPushButton *m_connectBtn;

    // TCP Server Client selector
    QComboBox *m_serverClientsCombo;
    QLabel *m_serverClientsLabel;

    // Display & Control
    QTextEdit *m_receiveText;
    QCheckBox *m_hexReceiveCheck;
    QCheckBox *m_timestampCheck;
    QCheckBox *m_pauseReceiveCheck;
    QPushButton *m_clearReceiveBtn;
    QPushButton *m_saveLogBtn;

    // Send Control
    QTextEdit *m_sendText;
    QCheckBox *m_hexSendCheck;
    QCheckBox *m_appendNewlineCheck;
    QCheckBox *m_autoSendCheck;
    QSpinBox *m_autoSendIntervalSpin;
    QPushButton *m_sendBtn;
    QPushButton *m_clearSendBtn;

    // Status
    QLabel *m_statusLabel;
    QLabel *m_rxCountLabel;
    QLabel *m_txCountLabel;

    // Sockets & Servers
    ProtocolType m_currentProtocol = ProtocolType::TCP_Client;
    bool m_isConnected = false;
    QTcpSocket *m_tcpClientSocket = nullptr;
    QTcpServer *m_tcpServer = nullptr;
    QList<QTcpSocket*> m_connectedClients;
    QUdpSocket *m_udpSocket = nullptr;

    QTimer *m_autoSendTimer;
    quint64 m_rxBytes = 0;
    quint64 m_txBytes = 0;
};

#endif // NETWORKTOOL_H
