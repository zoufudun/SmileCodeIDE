#include "networktool.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QScrollBar>

NetworkTool::NetworkTool(QWidget *parent) : QWidget(parent) {
    setObjectName("networkToolRoot");
    setWindowTitle(QStringLiteral("网络通信调试助手 Pro"));
    setWindowIcon(QIcon(":/icons/xptools2.png"));
    resize(960, 680);
    setMinimumSize(800, 550);

    m_tcpClientSocket = new QTcpSocket(this);
    connect(m_tcpClientSocket, &QTcpSocket::connected, this, &NetworkTool::onTcpClientConnected);
    connect(m_tcpClientSocket, &QTcpSocket::disconnected, this, &NetworkTool::onTcpClientDisconnected);
    connect(m_tcpClientSocket, &QTcpSocket::readyRead, this, &NetworkTool::onTcpClientReadyRead);

    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &NetworkTool::onTcpServerNewConnection);

    m_udpSocket = new QUdpSocket(this);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &NetworkTool::onUdpReadyRead);

    m_autoSendTimer = new QTimer(this);
    connect(m_autoSendTimer, &QTimer::timeout, this, &NetworkTool::onAutoSendTimerTimeout);

    setupUi();
    onProtocolChanged(0);
}

void NetworkTool::applyTheme(const QString &themeId) {
    Q_UNUSED(themeId);
    update();
}

NetworkTool::~NetworkTool() {
    if (m_autoSendTimer->isActive()) m_autoSendTimer->stop();
    if (m_tcpClientSocket->isOpen()) m_tcpClientSocket->close();
    if (m_tcpServer->isListening()) m_tcpServer->close();
    for (QTcpSocket *client : m_connectedClients) {
        if (client) client->close();
    }
    if (m_udpSocket->isOpen()) m_udpSocket->close();
}

void NetworkTool::setupUi() {
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(10, 10, 10, 10);
    rootLayout->setSpacing(8);

    // ==========================================
    // 顶部设置区域
    // ==========================================
    QGroupBox *configGroup = new QGroupBox(QStringLiteral("网络参数设置"), this);
    QHBoxLayout *configLayout = new QHBoxLayout(configGroup);
    configLayout->setContentsMargins(12, 10, 12, 10);
    configLayout->setSpacing(12);

    QLabel *protoLabel = new QLabel(QStringLiteral("协议模式:"), this);
    m_protocolCombo = new QComboBox(this);
    m_protocolCombo->addItem(QStringLiteral("TCP 客户端 (Client)"), static_cast<int>(ProtocolType::TCP_Client));
    m_protocolCombo->addItem(QStringLiteral("TCP 服务端 (Server)"), static_cast<int>(ProtocolType::TCP_Server));
    m_protocolCombo->addItem(QStringLiteral("UDP 通信 (Socket)"), static_cast<int>(ProtocolType::UDP));
    connect(m_protocolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NetworkTool::onProtocolChanged);

    QLabel *localIpLabel = new QLabel(QStringLiteral("本地 IP:"), this);
    m_localIpEdit = new QLineEdit(this);
    m_localIpEdit->setText("0.0.0.0");
    m_localIpEdit->setFixedWidth(120);

    QLabel *localPortLabel = new QLabel(QStringLiteral("本地端口:"), this);
    m_localPortSpin = new QSpinBox(this);
    m_localPortSpin->setRange(1, 65535);
    m_localPortSpin->setValue(8888);
    m_localPortSpin->setFixedWidth(80);

    QLabel *remoteIpLabel = new QLabel(QStringLiteral("目标 IP:"), this);
    m_remoteIpEdit = new QLineEdit(this);
    m_remoteIpEdit->setText("127.0.0.1");
    m_remoteIpEdit->setFixedWidth(120);

    QLabel *remotePortLabel = new QLabel(QStringLiteral("目标端口:"), this);
    m_remotePortSpin = new QSpinBox(this);
    m_remotePortSpin->setRange(1, 65535);
    m_remotePortSpin->setValue(8080);
    m_remotePortSpin->setFixedWidth(80);

    m_connectBtn = new QPushButton(QStringLiteral("连接网络"), this);
    m_connectBtn->setCursor(Qt::PointingHandCursor);
    m_connectBtn->setFixedHeight(32);
    m_connectBtn->setStyleSheet("font-weight: bold; padding: 0 16px;");
    connect(m_connectBtn, &QPushButton::clicked, this, &NetworkTool::onConnectClicked);

    configLayout->addWidget(protoLabel);
    configLayout->addWidget(m_protocolCombo);
    configLayout->addWidget(localIpLabel);
    configLayout->addWidget(m_localIpEdit);
    configLayout->addWidget(localPortLabel);
    configLayout->addWidget(m_localPortSpin);
    configLayout->addWidget(remoteIpLabel);
    configLayout->addWidget(m_remoteIpEdit);
    configLayout->addWidget(remotePortLabel);
    configLayout->addWidget(m_remotePortSpin);
    configLayout->addWidget(m_connectBtn);
    configLayout->addStretch();

    rootLayout->addWidget(configGroup);

    // ==========================================
    // 中部：接收区域
    // ==========================================
    QGroupBox *recvGroup = new QGroupBox(QStringLiteral("数据接收区"), this);
    QVBoxLayout *recvLayout = new QVBoxLayout(recvGroup);
    recvLayout->setContentsMargins(10, 8, 10, 8);
    recvLayout->setSpacing(6);

    // 接收控制栏
    QHBoxLayout *recvCtrlLayout = new QHBoxLayout();
    m_hexReceiveCheck = new QCheckBox(QStringLiteral("HEX 接收"), this);
    m_timestampCheck = new QCheckBox(QStringLiteral("显示时间戳"), this);
    m_timestampCheck->setChecked(true);
    m_pauseReceiveCheck = new QCheckBox(QStringLiteral("暂停接收"), this);

    m_serverClientsLabel = new QLabel(QStringLiteral("目标客户端:"), this);
    m_serverClientsCombo = new QComboBox(this);
    m_serverClientsCombo->addItem(QStringLiteral("全部客户端 (广播)"));
    m_serverClientsCombo->setMinimumWidth(180);

    m_clearReceiveBtn = new QPushButton(QStringLiteral("清空接收"), this);
    connect(m_clearReceiveBtn, &QPushButton::clicked, this, &NetworkTool::onClearReceiveClicked);

    m_saveLogBtn = new QPushButton(QStringLiteral("导出日志"), this);
    connect(m_saveLogBtn, &QPushButton::clicked, this, &NetworkTool::onSaveLogClicked);

    recvCtrlLayout->addWidget(m_hexReceiveCheck);
    recvCtrlLayout->addWidget(m_timestampCheck);
    recvCtrlLayout->addWidget(m_pauseReceiveCheck);
    recvCtrlLayout->addSpacing(16);
    recvCtrlLayout->addWidget(m_serverClientsLabel);
    recvCtrlLayout->addWidget(m_serverClientsCombo);
    recvCtrlLayout->addStretch();
    recvCtrlLayout->addWidget(m_clearReceiveBtn);
    recvCtrlLayout->addWidget(m_saveLogBtn);

    m_receiveText = new QTextEdit(this);
    m_receiveText->setReadOnly(true);
    m_receiveText->setFont(QFont("Consolas", 10));

    recvLayout->addLayout(recvCtrlLayout);
    recvLayout->addWidget(m_receiveText);

    rootLayout->addWidget(recvGroup, 3);

    // ==========================================
    // 底部：发送区域
    // ==========================================
    QGroupBox *sendGroup = new QGroupBox(QStringLiteral("数据发送区"), this);
    QVBoxLayout *sendLayout = new QVBoxLayout(sendGroup);
    sendLayout->setContentsMargins(10, 8, 10, 8);
    sendLayout->setSpacing(6);

    QHBoxLayout *sendCtrlLayout = new QHBoxLayout();
    m_hexSendCheck = new QCheckBox(QStringLiteral("HEX 发送"), this);
    m_appendNewlineCheck = new QCheckBox(QStringLiteral("末尾追加 \\r\\n"), this);
    m_autoSendCheck = new QCheckBox(QStringLiteral("定时发送"), this);
    connect(m_autoSendCheck, &QCheckBox::toggled, this, &NetworkTool::onAutoSendToggled);

    QLabel *intervalLabel = new QLabel(QStringLiteral("间隔(ms):"), this);
    m_autoSendIntervalSpin = new QSpinBox(this);
    m_autoSendIntervalSpin->setRange(10, 60000);
    m_autoSendIntervalSpin->setValue(1000);
    m_autoSendIntervalSpin->setSingleStep(100);

    m_clearSendBtn = new QPushButton(QStringLiteral("清空发送"), this);
    connect(m_clearSendBtn, &QPushButton::clicked, this, &NetworkTool::onClearSendClicked);

    m_sendBtn = new QPushButton(QStringLiteral("发送数据"), this);
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    m_sendBtn->setFixedHeight(30);
    m_sendBtn->setStyleSheet("font-weight: bold; padding: 0 20px;");
    connect(m_sendBtn, &QPushButton::clicked, this, &NetworkTool::onSendClicked);

    sendCtrlLayout->addWidget(m_hexSendCheck);
    sendCtrlLayout->addWidget(m_appendNewlineCheck);
    sendCtrlLayout->addSpacing(16);
    sendCtrlLayout->addWidget(m_autoSendCheck);
    sendCtrlLayout->addWidget(intervalLabel);
    sendCtrlLayout->addWidget(m_autoSendIntervalSpin);
    sendCtrlLayout->addStretch();
    sendCtrlLayout->addWidget(m_clearSendBtn);
    sendCtrlLayout->addWidget(m_sendBtn);

    m_sendText = new QTextEdit(this);
    m_sendText->setFont(QFont("Consolas", 10));
    m_sendText->setMaximumHeight(100);
    m_sendText->setPlaceholderText(QStringLiteral("请输入要发送的字符串或十六进制数据（例：48 65 6C 6C 6F）..."));

    sendLayout->addLayout(sendCtrlLayout);
    sendLayout->addWidget(m_sendText);

    rootLayout->addWidget(sendGroup, 1);

    // ==========================================
    // 状态栏
    // ==========================================
    QHBoxLayout *statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(QStringLiteral("状态: 就绪 (未连接)"), this);
    m_rxCountLabel = new QLabel(QStringLiteral("接收: 0 Bytes"), this);
    m_txCountLabel = new QLabel(QStringLiteral("发送: 0 Bytes"), this);

    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_rxCountLabel);
    statusLayout->addSpacing(20);
    statusLayout->addWidget(m_txCountLabel);

    rootLayout->addLayout(statusLayout);
}

void NetworkTool::onProtocolChanged(int index) {
    m_currentProtocol = static_cast<ProtocolType>(index);
    if (m_isConnected) {
        onConnectClicked(); // 如果正在连接则先断开
    }

    if (m_currentProtocol == ProtocolType::TCP_Client) {
        m_localIpEdit->setEnabled(false);
        m_localPortSpin->setEnabled(false);
        m_remoteIpEdit->setEnabled(true);
        m_remotePortSpin->setEnabled(true);
        m_serverClientsLabel->setVisible(false);
        m_serverClientsCombo->setVisible(false);
        m_connectBtn->setText(QStringLiteral("连接服务器"));
    } else if (m_currentProtocol == ProtocolType::TCP_Server) {
        m_localIpEdit->setEnabled(true);
        m_localPortSpin->setEnabled(true);
        m_remoteIpEdit->setEnabled(false);
        m_remotePortSpin->setEnabled(false);
        m_serverClientsLabel->setVisible(true);
        m_serverClientsCombo->setVisible(true);
        m_connectBtn->setText(QStringLiteral("启动监听"));
    } else if (m_currentProtocol == ProtocolType::UDP) {
        m_localIpEdit->setEnabled(true);
        m_localPortSpin->setEnabled(true);
        m_remoteIpEdit->setEnabled(true);
        m_remotePortSpin->setEnabled(true);
        m_serverClientsLabel->setVisible(false);
        m_serverClientsCombo->setVisible(false);
        m_connectBtn->setText(QStringLiteral("绑定端口"));
    }
}

void NetworkTool::onConnectClicked() {
    if (m_isConnected) {
        // 断开连接
        if (m_currentProtocol == ProtocolType::TCP_Client) {
            m_tcpClientSocket->disconnectFromHost();
        } else if (m_currentProtocol == ProtocolType::TCP_Server) {
            m_tcpServer->close();
            for (QTcpSocket *client : m_connectedClients) {
                if (client) client->close();
            }
            m_connectedClients.clear();
            m_serverClientsCombo->clear();
            m_serverClientsCombo->addItem(QStringLiteral("全部客户端 (广播)"));
        } else if (m_currentProtocol == ProtocolType::UDP) {
            m_udpSocket->close();
        }
        updateUiState(false);
    } else {
        // 建立连接 / 监听 / 绑定
        if (m_currentProtocol == ProtocolType::TCP_Client) {
            QString ip = m_remoteIpEdit->text().trimmed();
            quint16 port = static_cast<quint16>(m_remotePortSpin->value());
            m_tcpClientSocket->connectToHost(ip, port);
            m_statusLabel->setText(QStringLiteral("状态: 正在连接 %1:%2 ...").arg(ip).arg(port));
        } else if (m_currentProtocol == ProtocolType::TCP_Server) {
            quint16 port = static_cast<quint16>(m_localPortSpin->value());
            QHostAddress addr = QHostAddress::Any;
            QString localIp = m_localIpEdit->text().trimmed();
            if (!localIp.isEmpty() && localIp != "0.0.0.0") {
                addr = QHostAddress(localIp);
            }
            if (m_tcpServer->listen(addr, port)) {
                updateUiState(true);
                m_statusLabel->setText(QStringLiteral("状态: TCP 服务端已就绪，正在监听端口 %1").arg(port));
            } else {
                QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("启动监听失败: %1").arg(m_tcpServer->errorString()));
            }
        } else if (m_currentProtocol == ProtocolType::UDP) {
            quint16 port = static_cast<quint16>(m_localPortSpin->value());
            if (m_udpSocket->bind(QHostAddress::Any, port)) {
                updateUiState(true);
                m_statusLabel->setText(QStringLiteral("状态: UDP Socket 已绑定端口 %1").arg(port));
            } else {
                QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("UDP 绑定端口失败: %1").arg(m_udpSocket->errorString()));
            }
        }
    }
}

void NetworkTool::updateUiState(bool connected) {
    m_isConnected = connected;
    m_protocolCombo->setEnabled(!connected);
    m_localPortSpin->setEnabled(!connected && (m_currentProtocol != ProtocolType::TCP_Client));
    m_localIpEdit->setEnabled(!connected && (m_currentProtocol != ProtocolType::TCP_Client));

    if (m_currentProtocol == ProtocolType::TCP_Client) {
        m_remoteIpEdit->setEnabled(!connected);
        m_remotePortSpin->setEnabled(!connected);
        m_connectBtn->setText(connected ? QStringLiteral("断开连接") : QStringLiteral("连接服务器"));
    } else if (m_currentProtocol == ProtocolType::TCP_Server) {
        m_connectBtn->setText(connected ? QStringLiteral("停止监听") : QStringLiteral("启动监听"));
    } else if (m_currentProtocol == ProtocolType::UDP) {
        m_connectBtn->setText(connected ? QStringLiteral("解除绑定") : QStringLiteral("绑定端口"));
    }

    if (!connected) {
        m_statusLabel->setText(QStringLiteral("状态: 已断开 / 就绪"));
        if (m_autoSendCheck->isChecked()) {
            m_autoSendCheck->setChecked(false);
        }
    }
}

void NetworkTool::onTcpClientConnected() {
    updateUiState(true);
    m_statusLabel->setText(QStringLiteral("状态: 已连接至 %1:%2").arg(m_remoteIpEdit->text()).arg(m_remotePortSpin->value()));
}

void NetworkTool::onTcpClientDisconnected() {
    updateUiState(false);
}

void NetworkTool::onTcpClientReadyRead() {
    if (m_pauseReceiveCheck->isChecked()) return;
    QByteArray data = m_tcpClientSocket->readAll();
    if (!data.isEmpty()) {
        m_rxBytes += data.size();
        m_rxCountLabel->setText(QStringLiteral("接收: %1 Bytes").arg(m_rxBytes));
        appendLog(QStringLiteral("TCP 服务端"), data, false);
    }
}

void NetworkTool::onTcpClientError() {
    m_statusLabel->setText(QStringLiteral("错误: %1").arg(m_tcpClientSocket->errorString()));
}

void NetworkTool::onTcpServerNewConnection() {
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket *client = m_tcpServer->nextPendingConnection();
        m_connectedClients.append(client);
        QString clientAddr = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
        m_serverClientsCombo->addItem(clientAddr);

        connect(client, &QTcpSocket::readyRead, this, &NetworkTool::onServerClientReadyRead);
        connect(client, &QTcpSocket::disconnected, this, &NetworkTool::onServerClientDisconnected);

        appendLog(QStringLiteral("系统"), QString("客户端 [%1] 已连接").arg(clientAddr).toUtf8(), false);
    }
}

void NetworkTool::onServerClientReadyRead() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client || m_pauseReceiveCheck->isChecked()) return;

    QByteArray data = client->readAll();
    if (!data.isEmpty()) {
        m_rxBytes += data.size();
        m_rxCountLabel->setText(QStringLiteral("接收: %1 Bytes").arg(m_rxBytes));
        QString clientAddr = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
        appendLog(clientAddr, data, false);
    }
}

void NetworkTool::onServerClientDisconnected() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QString clientAddr = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
    int idx = m_serverClientsCombo->findText(clientAddr);
    if (idx != -1) {
        m_serverClientsCombo->removeItem(idx);
    }
    m_connectedClients.removeOne(client);
    client->deleteLater();
    appendLog(QStringLiteral("系统"), QString("客户端 [%1] 已断开").arg(clientAddr).toUtf8(), false);
}

void NetworkTool::onUdpReadyRead() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_udpSocket->pendingDatagramSize()));
        QHostAddress senderAddr;
        quint16 senderPort;
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddr, &senderPort);

        if (!m_pauseReceiveCheck->isChecked() && !datagram.isEmpty()) {
            m_rxBytes += datagram.size();
            m_rxCountLabel->setText(QStringLiteral("接收: %1 Bytes").arg(m_rxBytes));
            QString source = QString("%1:%2").arg(senderAddr.toString()).arg(senderPort);
            appendLog(source, datagram, false);
        }
    }
}

QByteArray NetworkTool::parseSendData() {
    QString text = m_sendText->toPlainText();
    if (text.isEmpty()) return QByteArray();

    QByteArray data;
    if (m_hexSendCheck->isChecked()) {
        QString hexStr = text;
        hexStr.remove(' ');
        hexStr.remove('\n');
        hexStr.remove('\r');
        hexStr.remove('\t');
        data = QByteArray::fromHex(hexStr.toLatin1());
    } else {
        data = text.toUtf8();
    }

    if (m_appendNewlineCheck->isChecked()) {
        data.append("\r\n");
    }
    return data;
}

void NetworkTool::onSendClicked() {
    if (!m_isConnected) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先建立网络连接或启动服务！"));
        return;
    }

    QByteArray data = parseSendData();
    if (data.isEmpty()) return;

    qint64 bytesWritten = 0;

    if (m_currentProtocol == ProtocolType::TCP_Client) {
        if (m_tcpClientSocket->state() == QAbstractSocket::ConnectedState) {
            bytesWritten = m_tcpClientSocket->write(data);
        }
    } else if (m_currentProtocol == ProtocolType::TCP_Server) {
        int index = m_serverClientsCombo->currentIndex();
        if (index == 0) { // 广播
            for (QTcpSocket *client : m_connectedClients) {
                if (client && client->state() == QAbstractSocket::ConnectedState) {
                    client->write(data);
                }
            }
            bytesWritten = data.size();
        } else {
            int clientIdx = index - 1;
            if (clientIdx >= 0 && clientIdx < m_connectedClients.size()) {
                QTcpSocket *client = m_connectedClients[clientIdx];
                if (client && client->state() == QAbstractSocket::ConnectedState) {
                    bytesWritten = client->write(data);
                }
            }
        }
    } else if (m_currentProtocol == ProtocolType::UDP) {
        QString targetIp = m_remoteIpEdit->text().trimmed();
        quint16 targetPort = static_cast<quint16>(m_remotePortSpin->value());
        bytesWritten = m_udpSocket->writeDatagram(data, QHostAddress(targetIp), targetPort);
    }

    if (bytesWritten > 0) {
        m_txBytes += data.size();
        m_txCountLabel->setText(QStringLiteral("发送: %1 Bytes").arg(m_txBytes));
        appendLog(QStringLiteral("我"), data, true);
    }
}

void NetworkTool::appendLog(const QString &source, const QByteArray &data, bool isSend) {
    QString logLine;
    if (m_timestampCheck->isChecked()) {
        logLine += QString("[%1] ").arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz"));
    }

    logLine += QString("[%1%2] ").arg(isSend ? QStringLiteral("发送 -> ") : QStringLiteral("接收 <- ")).arg(source);

    if (m_hexReceiveCheck->isChecked()) {
        logLine += QString::fromLatin1(data.toHex(' ').toUpper());
    } else {
        logLine += QString::fromUtf8(data);
    }

    m_receiveText->append(logLine);
    m_receiveText->verticalScrollBar()->setValue(m_receiveText->verticalScrollBar()->maximum());
}

void NetworkTool::onClearReceiveClicked() {
    m_receiveText->clear();
    m_rxBytes = 0;
    m_rxCountLabel->setText(QStringLiteral("接收: 0 Bytes"));
}

void NetworkTool::onClearSendClicked() {
    m_sendText->clear();
    m_txBytes = 0;
    m_txCountLabel->setText(QStringLiteral("发送: 0 Bytes"));
}

void NetworkTool::onSaveLogClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("保存接收日志"), "NetworkLog.txt", QStringLiteral("文本文件 (*.txt);;所有文件 (*.*)"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(m_receiveText->toPlainText().toUtf8());
        file.close();
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("日志保存成功！"));
    }
}

void NetworkTool::onAutoSendToggled(bool checked) {
    if (checked) {
        if (!m_isConnected) {
            m_autoSendCheck->setChecked(false);
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先建立网络连接或启动服务后再开启定时发送！"));
            return;
        }
        m_autoSendTimer->start(m_autoSendIntervalSpin->value());
    } else {
        m_autoSendTimer->stop();
    }
}

void NetworkTool::onAutoSendTimerTimeout() {
    if (m_isConnected) {
        onSendClicked();
    } else {
        m_autoSendCheck->setChecked(false);
    }
}
