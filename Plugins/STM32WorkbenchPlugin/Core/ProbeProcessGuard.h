#ifndef PROBEPROCESSGUARD_H
#define PROBEPROCESSGUARD_H

#include <QCoreApplication>
#include <QDir>
#include <QLockFile>
#include <QPointer>
#include <QProcess>
#include <QStandardPaths>
#include <QVariant>
#include <functional>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

// Header-only so the host, plugin and standalone all use the same policy.
// The application property is shared across DLLs; the lock file spans IDE instances.
class ProbeProcessGuard : public QObject {
public:
    explicit ProbeProcessGuard(QProcess *process, std::function<void(const QString &)> report)
        : QObject(process), m_process(process), m_report(std::move(report)),
          m_lock(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                     .filePath(QStringLiteral("SmileCodeIDE-hardware-access.lock")))
    {
        // A live download must never lose its lock merely because it takes a long time.
        m_lock.setStaleLockTime(0);
#ifdef Q_OS_WIN
        HANDLE job = CreateJobObjectW(nullptr, nullptr);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (job && SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                           &limits, sizeof(limits))) m_job = job;
        else if (job) CloseHandle(job);
#endif
        connect(process, &QProcess::started, this, [this] {
            QString error;
            bool protectedProcess = acquire(&error);
#ifdef Q_OS_WIN
            HANDLE child = OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE,
                                       static_cast<DWORD>(m_process->processId()));
            if (!m_job || !child || !AssignProcessToJobObject(m_job, child)) {
                const DWORD code = GetLastError();
                error = QStringLiteral("无法启用硬件工具退出清理保障 (Windows 错误 %1)，已中止启动。").arg(code);
                protectedProcess = false;
            }
            if (child) CloseHandle(child);
#endif
            if (!protectedProcess) {
                m_process->kill();
                m_process->waitForFinished(3000);
                m_report(error);
            }
        });
        connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                this, [this] { release(); });
        connect(process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) release();
        });
    }

    ~ProbeProcessGuard() override {
#ifdef Q_OS_WIN
        if (m_job) CloseHandle(m_job);
#endif
    }

    bool acquire(QString *error) {
        if (m_lock.isLocked() || m_lock.tryLock(0)) return true;
        qint64 pid = 0;
        QString host, app;
        if (m_lock.getLockInfo(&pid, &host, &app)) {
            if (error) *error = QStringLiteral("硬件工具正由 %1 (PID %2) 使用，请先结束该实例的调试或下载后重试。").arg(app).arg(pid);
        } else if (error) {
            *error = QStringLiteral("无法取得硬件访问锁，请检查临时目录的访问权限。");
        }
        return false;
    }

    void release() {
        if (m_process->state() == QProcess::NotRunning) m_lock.unlock();
    }

    static void registerDebugServer(QObject *server, bool add) {
        auto *app = QCoreApplication::instance();
        if (!app) return;
        QVariantList entries = app->property(registryKey()).toList();
        if (add) entries.append(QVariant::fromValue(server));
        else {
            for (qsizetype i = entries.size(); i-- > 0;)
                if (entries.at(i).value<QObject *>() == server) entries.removeAt(i);
        }
        app->setProperty(registryKey(), entries);
    }

    static bool releaseDebugServers(QObject *except = nullptr) {
        auto *app = QCoreApplication::instance();
        if (!app) return true;
        QList<QPointer<QObject>> servers;
        for (const auto &entry : app->property(registryKey()).toList())
            servers.append(entry.value<QObject *>());
        bool released = true;
        for (const auto &server : servers) {
            if (!server || server == except) continue;
            // All public hardware entry points run on the GUI thread.
            released = QMetaObject::invokeMethod(server, "stopServer", Qt::DirectConnection) && released;
            if (server) {
                auto *process = server->findChild<QProcess *>();
                if (process && process->state() != QProcess::NotRunning) released = false;
            }
        }
        return released;
    }

    static QString accessFailureHint(const QString &output) {
        if (!output.contains(QStringLiteral("LIBUSB_ERROR_ACCESS"), Qt::CaseInsensitive) &&
            !output.contains(QStringLiteral("LIBUSB_ERROR_BUSY"), Qt::CaseInsensitive)) return {};
        return QStringLiteral("探针 USB 访问被拒绝：请关闭其他 IDE、OpenOCD、ST-Link Server 或烧录软件的连接；"
                              "若无其他占用，请重新插拔探针，并检查设备管理器中的探针驱动及设备访问权限。"
                              "本次下载已停止，请排除原因后重新下载；不会自动重试写入。");
    }

private:
    static const char *registryKey() { return "_smilecode_debug_server_registry_v1"; }
    QProcess *m_process;
    std::function<void(const QString &)> m_report;
    QLockFile m_lock;
#ifdef Q_OS_WIN
    HANDLE m_job = nullptr;
#endif
};

#endif
