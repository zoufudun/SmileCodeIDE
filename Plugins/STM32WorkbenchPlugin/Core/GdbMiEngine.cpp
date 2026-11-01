#include "GdbMiEngine.h"
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>

GdbMiEngine::GdbMiEngine(QObject *parent)
    : QObject(parent), m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyRead, this, &GdbMiEngine::onProcessReadyRead);
    connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &GdbMiEngine::onProcessFinished);
}

GdbMiEngine::~GdbMiEngine()
{
    stopSession();
}

bool GdbMiEngine::startSession(const QString &gdbPath,
                               const QString &elfPath,
                               const QString &host,
                               int port)
{
    if (m_process->state() != QProcess::NotRunning) {
        stopSession();
    }

    m_elfPath = elfPath;
    m_host = host;
    m_port = port;
    m_breakpoints.clear();

    QString gdbExec = gdbPath.isEmpty() ? "arm-none-eabi-gdb" : gdbPath;

    QStringList args;
    args << "--interpreter=mi2"
         << "-q"; // 静默模式

    emit consoleOutput(QString("启动 GDB 客户端: %1 %2\n").arg(gdbExec, args.join(" ")));
    m_state = GdbTargetState::Connecting;
    emit stateChanged(m_state);

    m_process->start(gdbExec, args);
    if (!m_process->waitForStarted(2000)) {
        emit consoleOutput("无法启动 arm-none-eabi-gdb: " + m_process->errorString() + "\n");
        m_state = GdbTargetState::Disconnected;
        emit stateChanged(m_state);
        return false;
    }

    // 初始化 GDB 设置与连接
    sendMiCommand("-gdb-set target-async on");
    sendMiCommand("-gdb-set pagination off");

    if (!elfPath.isEmpty()) {
        QString normElf = elfPath;
        normElf.replace('\\', '/');
        sendMiCommand(QString("-file-exec-and-symbols \"%1\"").arg(normElf));
    }

    sendMiCommand(QString("-target-select remote %1:%2").arg(host).arg(port));
    sendMiCommand("monitor reset halt");

    return true;
}

void GdbMiEngine::stopSession()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        sendMiCommand("-gdb-exit");
        if (!m_process->waitForFinished(800)) {
            m_process->kill();
        }
    }
    m_state = GdbTargetState::Disconnected;
    emit stateChanged(m_state);
    emit sessionFinished();
}

void GdbMiEngine::sendMiCommand(const QString &cmd)
{
    if (!m_process || m_process->state() == QProcess::NotRunning) return;

    QString fullCmd = cmd.trimmed();
    if (!fullCmd.endsWith('\n')) {
        fullCmd += "\n";
    }

    emit consoleOutput(">> " + fullCmd);
    m_process->write(fullCmd.toUtf8());
}

void GdbMiEngine::continueExec()
{
    sendMiCommand("-exec-continue");
}

void GdbMiEngine::pauseExec()
{
    sendMiCommand("-exec-interrupt");
}

void GdbMiEngine::stepOver()
{
    sendMiCommand("-exec-next");
}

void GdbMiEngine::stepInto()
{
    sendMiCommand("-exec-step");
}

void GdbMiEngine::stepOut()
{
    sendMiCommand("-exec-finish");
}

void GdbMiEngine::resetHalt()
{
    sendMiCommand("monitor reset halt");
    requestStackTrace();
    requestLocals();
    requestRegisters();
}

void GdbMiEngine::loadFirmware()
{
    sendMiCommand("-target-download");
}

void GdbMiEngine::insertBreakpoint(const QString &file, int line)
{
    QString normFile = QFileInfo(file).fileName();
    sendMiCommand(QString("-break-insert -f %1:%2").arg(normFile).arg(line));
}

void GdbMiEngine::removeBreakpoint(int id)
{
    sendMiCommand(QString("-break-delete %1").arg(id));
    if (m_breakpoints.contains(id)) {
        GdbBreakpoint b = m_breakpoints.take(id);
        emit breakpointChanged(b, false);
    }
}

void GdbMiEngine::toggleBreakpoint(const QString &file, int line)
{
    for (auto it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it) {
        if (it.value().file.endsWith(QFileInfo(file).fileName()) && it.value().line == line) {
            removeBreakpoint(it.key());
            return;
        }
    }
    insertBreakpoint(file, line);
}

void GdbMiEngine::requestStackTrace()
{
    sendMiCommand("-stack-list-frames");
}

void GdbMiEngine::requestLocals()
{
    sendMiCommand("-stack-list-locals --simple-values");
}

void GdbMiEngine::requestRegisters()
{
    sendMiCommand("-data-list-register-values x");
}

void GdbMiEngine::evaluateExpression(const QString &expr)
{
    sendMiCommand(QString("-data-evaluate-expression \"%1\"").arg(expr));
}

void GdbMiEngine::sendRawCommand(const QString &cmd)
{
    if (cmd.startsWith("-")) {
        sendMiCommand(cmd);
    } else {
        // 使用 interpreter-exec 执行原生 CLI 指令
        sendMiCommand(QString("-interpreter-exec console \"%1\"").arg(cmd));
    }
}

void GdbMiEngine::onProcessReadyRead()
{
    QByteArray raw = m_process->readAll();
    QString text = QString::fromLocal8Bit(raw);

    QStringList lines = text.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        parseMiLine(line);
    }
}

void GdbMiEngine::parseMiLine(const QString &line)
{
    if (line.isEmpty() || line == "(gdb) ") return;

    QChar prefix = line.at(0);

    // 控制台回显 ~"..."
    if (prefix == '~') {
        QString content = line.mid(1);
        if (content.startsWith('"') && content.endsWith('"')) {
            content = content.mid(1, content.length() - 2);
            content.replace("\\n", "\n").replace("\\t", "\t").replace("\\\"", "\"");
        }
        emit consoleOutput(content);
        return;
    }

    // 目标输出 @"..." 或 日志 &"..."
    if (prefix == '@' || prefix == '&') {
        QString content = line.mid(1);
        if (content.startsWith('"') && content.endsWith('"')) {
            content = content.mid(1, content.length() - 2);
            content.replace("\\n", "\n").replace("\\\"", "\"");
        }
        emit consoleOutput(content);
        return;
    }

    // 异步状态更新 *stopped / *running
    if (prefix == '*') {
        if (line.startsWith("*running")) {
            m_state = GdbTargetState::Running;
            emit stateChanged(m_state);
            emit targetRunning();
        } else if (line.startsWith("*stopped")) {
            m_state = GdbTargetState::Stopped;
            emit stateChanged(m_state);
            handleStoppedNotification(line);
        }
        return;
    }

    // 事件通知 =breakpoint-created / =breakpoint-deleted
    if (prefix == '=') {
        if (line.startsWith("=breakpoint-created")) {
            handleBreakpointCreated(line);
        }
        return;
    }

    // 指令响应结果 ^done / ^running / ^error / ^connected
    if (prefix == '^') {
        if (line.startsWith("^connected")) {
            m_state = GdbTargetState::Connected;
            emit stateChanged(m_state);
            emit consoleOutput("✅ GDB 已成功连接至远程目标硬件\n");
        } else if (line.startsWith("^running")) {
            m_state = GdbTargetState::Running;
            emit stateChanged(m_state);
            emit targetRunning();
        } else if (line.startsWith("^error")) {
            QString msg = extractFieldValue(line, "msg");
            emit consoleOutput("❌ GDB 报错: " + (msg.isEmpty() ? line : msg) + "\n");
        } else if (line.startsWith("^done")) {
            // 解析 stack-list-frames 结果
            if (line.contains("stack=[frame=")) {
                handleStackTraceResult(line);
            } else if (line.contains("locals=[")) {
                handleLocalsResult(line);
            } else if (line.contains("register-values=[")) {
                handleRegistersResult(line);
            } else if (line.contains("value=\"")) {
                QString val = extractFieldValue(line, "value");
                emit expressionEvaluated("", val);
            } else if (line.contains("bkpt={")) {
                handleBreakpointCreated(line);
            }
        }
        return;
    }
}

void GdbMiEngine::handleStoppedNotification(const QString &payload)
{
    QString reason = extractFieldValue(payload, "reason");
    QString file = extractFieldValue(payload, "file");
    QString fullname = extractFieldValue(payload, "fullname");
    QString lineStr = extractFieldValue(payload, "line");
    QString func = extractFieldValue(payload, "func");

    int lineNum = lineStr.toInt();
    QString targetFile = fullname.isEmpty() ? file : fullname;

    emit consoleOutput(QString("\n⏸ [目标硬件已暂停] 原因: %1, 文件: %2:%3 (%4)\n")
                       .arg(reason.isEmpty() ? "单步/中断" : reason, targetFile, QString::number(lineNum), func));

    emit targetStopped(targetFile, lineNum, func, reason);

    // 自动刷新调用栈、局部变量与寄存器
    requestStackTrace();
    requestLocals();
    requestRegisters();
}

void GdbMiEngine::handleBreakpointCreated(const QString &payload)
{
    GdbBreakpoint b;
    b.id = extractFieldValue(payload, "number").toInt();
    if (b.id == 0) b.id = extractFieldValue(payload, "bkptno").toInt();
    b.file = extractFieldValue(payload, "file");
    b.line = extractFieldValue(payload, "line").toInt();
    b.address = extractFieldValue(payload, "addr");
    b.func = extractFieldValue(payload, "func");
    b.enabled = (extractFieldValue(payload, "enabled") != "n");

    if (b.id > 0) {
        m_breakpoints[b.id] = b;
        emit breakpointChanged(b, true);
        emit consoleOutput(QString("断点 #%1 已设置于 %2:%3 (%4)\n").arg(b.id).arg(b.file).arg(b.line).arg(b.address));
    }
}

void GdbMiEngine::handleStackTraceResult(const QString &payload)
{
    QVector<GdbStackFrame> frames;
    // 匹配 frame={level="0",addr="0x08000456",func="main",file="main.c",fullname="...",line="25"}
    static QRegularExpression frameRegex(R"(frame=\{([^}]+)\})");
    QRegularExpressionMatchIterator it = frameRegex.globalMatch(payload);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString frameBody = m.captured(1);

        GdbStackFrame f;
        f.level = extractFieldValue(frameBody, "level").toInt();
        f.address = extractFieldValue(frameBody, "addr");
        f.func = extractFieldValue(frameBody, "func");
        f.file = extractFieldValue(frameBody, "file");
        f.fullname = extractFieldValue(frameBody, "fullname");
        f.line = extractFieldValue(frameBody, "line").toInt();
        frames.append(f);
    }
    emit stackTraceReceived(frames);
}

void GdbMiEngine::handleLocalsResult(const QString &payload)
{
    QVector<GdbVariable> vars;
    // 匹配 {name="xxx",type="int",value="10"}
    static QRegularExpression varRegex(QStringLiteral("\\{name=\"([^\"]+)\",(?:type=\"([^\"]+)\",)?value=\"([^\"]+)\"\\}"));
    QRegularExpressionMatchIterator it = varRegex.globalMatch(payload);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        GdbVariable v;
        v.name = m.captured(1);
        v.type = m.captured(2).isEmpty() ? "auto" : m.captured(2);
        v.value = m.captured(3);
        vars.append(v);
    }
    emit localsReceived(vars);
}

void GdbMiEngine::handleRegistersResult(const QString &payload)
{
    QVector<GdbRegister> regList;
    // ARM Cortex-M 常见寄存器名称映射
    static const QStringList regNames = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
        "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc",
        "xpsr", "msp", "psp", "primask", "basepri", "faultmask", "control"
    };

    static QRegularExpression regRegex(QStringLiteral("\\{number=\"(\\d+)\",value=\"([^\"]+)\"\\}"));
    QRegularExpressionMatchIterator it = regRegex.globalMatch(payload);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        GdbRegister r;
        r.number = m.captured(1).toInt();
        r.value = m.captured(2);
        if (r.number >= 0 && r.number < regNames.size()) {
            r.name = regNames.at(r.number);
        } else {
            r.name = QString("r%1").arg(r.number);
        }
        regList.append(r);
    }
    emit registersReceived(regList);
}

QString GdbMiEngine::extractFieldValue(const QString &src, const QString &key)
{
    QRegularExpression re(key + "=\"([^\"]*)\"");
    QRegularExpressionMatch m = re.match(src);
    if (m.hasMatch()) {
        return m.captured(1);
    }
    return QString();
}

void GdbMiEngine::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    m_state = GdbTargetState::Exited;
    emit stateChanged(m_state);
    emit consoleOutput(QStringLiteral("\n[GDB 客户端进程已退出]\n"));
    emit sessionFinished();
}
