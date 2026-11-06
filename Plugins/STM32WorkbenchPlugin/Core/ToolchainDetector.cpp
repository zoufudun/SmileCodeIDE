#include "ToolchainDetector.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>
#include <QProcessEnvironment>
#include <algorithm>

bool ToolchainPaths::isGccValid() const {
    return !gccPath.isEmpty() && QFileInfo::exists(gccPath);
}

bool ToolchainPaths::isMakeValid() const {
    return !makePath.isEmpty() && QFileInfo::exists(makePath);
}

bool ToolchainPaths::isGdbValid() const {
    return !gdbPath.isEmpty() && QFileInfo::exists(gdbPath);
}

bool ToolchainPaths::isOpenocdValid() const {
    return !openocdPath.isEmpty() && QFileInfo::exists(openocdPath);
}

bool ToolchainPaths::isJlinkValid() const {
    return !jlinkPath.isEmpty() && QFileInfo::exists(jlinkPath);
}

ToolchainDetector::ToolchainDetector(QObject *parent)
    : QObject(parent)
{
}

QString ToolchainDetector::findExecutable(const QString &name, const QStringList &searchDirs)
{
#ifdef Q_OS_WIN
    QString targetName = name.endsWith(".exe", Qt::CaseInsensitive) ? name : name + ".exe";
#else
    QString targetName = name;
#endif

    // 1. 先在明确指定的目录中搜索
    for (const QString &dir : searchDirs) {
        if (dir.isEmpty()) continue;
        QDir qdir(dir);
        QString fullPath = qdir.absoluteFilePath(targetName);
        if (QFileInfo::exists(fullPath) && QFileInfo(fullPath).isExecutable()) {
            return QDir::toNativeSeparators(fullPath);
        }
    }

    // 2. 在系统 PATH 中搜索
    QString pathInSys = QStandardPaths::findExecutable(name);
    if (!pathInSys.isEmpty()) {
        return QDir::toNativeSeparators(pathInSys);
    }

    return QString();
}

QString ToolchainDetector::detectJLinkPath(const QString &hintDir)
{
    QStringList jlinkDirs;
    QStringList keilFallbackDirs;

    const auto addDir = [](QStringList &list, const QString &path) {
        if (path.isEmpty()) return;
        QFileInfo info(path);
        const QString dirPath = info.isFile() ? info.absolutePath() : path;
        if (QDir(dirPath).exists()) {
            const QString cleaned = QDir::cleanPath(dirPath);
            if (!list.contains(cleaned)) {
                list << cleaned;
            }
            const QString binSub = QDir(cleaned).absoluteFilePath("bin");
            if (QDir(binSub).exists() && !list.contains(binSub)) {
                list << binSub;
            }
        }
    };

    const auto collectSeggerRoot = [&](const QString &seggerRoot) {
        QDir dir(seggerRoot);
        if (!dir.exists()) return;
        // 优先匹配版本号更高的 JLink_Vxxx 目录（按名称倒序排列）
        const QFileInfoList children = dir.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
        for (const QFileInfo &child : children) {
            const QString lower = child.fileName().toLower();
            if (lower.contains("jlink") || lower.contains("j-link")) {
                addDir(jlinkDirs, child.absoluteFilePath());
            }
        }
        addDir(jlinkDirs, dir.absoluteFilePath("JLink"));
        addDir(jlinkDirs, seggerRoot);
    };

    if (!hintDir.isEmpty()) {
        addDir(jlinkDirs, hintDir);
        collectSeggerRoot(hintDir);
        collectSeggerRoot(QDir(hintDir).absoluteFilePath("SEGGER"));
        addDir(keilFallbackDirs, QDir(hintDir).absoluteFilePath("ARM/Segger"));
    }

    const QProcessEnvironment system = QProcessEnvironment::systemEnvironment();
    for (const char *envKey : {"JLINK_HOME", "SEGGER_HOME", "JLINK_PATH"}) {
        const QString val = system.value(QString::fromLatin1(envKey));
        if (!val.isEmpty()) {
            addDir(jlinkDirs, val);
            collectSeggerRoot(val);
        }
    }

#ifdef Q_OS_WIN
    // 1. 查询 Windows 注册表中的 SEGGER J-Link 官方安装项
    const QStringList seggerRegKeys = {
        QStringLiteral("HKEY_CURRENT_USER\\Software\\SEGGER\\J-Link"),
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\SEGGER\\J-Link"),
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\SEGGER\\J-Link")
    };
    for (const QString &regPath : seggerRegKeys) {
        QSettings reg(regPath, QSettings::NativeFormat);
        const QString directInstall = reg.value("InstallPath").toString();
        if (!directInstall.isEmpty()) {
            addDir(jlinkDirs, directInstall);
        }
        QStringList groups = reg.childGroups();
        std::sort(groups.begin(), groups.end(), std::greater<QString>());
        for (const QString &grp : groups) {
            reg.beginGroup(grp);
            const QString subInstall = reg.value("InstallPath").toString();
            if (!subInstall.isEmpty()) {
                addDir(jlinkDirs, subInstall);
            }
            reg.endGroup();
        }
    }

    // 2. 查询 Windows 卸载注册表项 (SEGGER J-Link)
    const QStringList uninstallRegKeys = {
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall")
    };
    for (const QString &uninstRoot : uninstallRegKeys) {
        QSettings reg(uninstRoot, QSettings::NativeFormat);
        const QStringList subKeys = reg.childGroups();
        for (const QString &sub : subKeys) {
            const QString lowerSub = sub.toLower();
            if (!lowerSub.contains("jlink") && !lowerSub.contains("j-link") && !lowerSub.contains("segger")) {
                continue;
            }
            reg.beginGroup(sub);
            const QString loc = reg.value("InstallLocation").toString();
            if (!loc.isEmpty()) {
                addDir(jlinkDirs, loc);
            }
            const QString uninstStr = reg.value("UninstallString").toString().remove('"');
            if (!uninstStr.isEmpty()) {
                addDir(jlinkDirs, QFileInfo(uninstStr).absolutePath());
            }
            reg.endGroup();
        }
    }

    // 3. 查询 Keil MDK 注册表内置 Segger 目录（作为备选）
    const QStringList keilRegKeys = {
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Keil\\Products\\MDK"),
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Keil\\Products\\MDK")
    };
    for (const QString &keilKey : keilRegKeys) {
        QSettings reg(keilKey, QSettings::NativeFormat);
        const QString mdPath = reg.value("Path").toString();
        if (!mdPath.isEmpty()) {
            addDir(keilFallbackDirs, QDir(mdPath).absoluteFilePath("ARM/Segger"));
            addDir(keilFallbackDirs, QDir(mdPath).absoluteFilePath("../ARM/Segger"));
        }
    }

    // 4. 扫描各磁盘常见软件安装目录 (SEGGER / Soft / Tools / Program Files / Keil_v5)
    for (const QFileInfo &drive : QDir::drives()) {
        const QString base = drive.absoluteFilePath();
        const QStringList seggerBases = {
            base + "SEGGER",
            base + "Soft/SEGGER",
            base + "soft/SEGGER",
            base + "Software/SEGGER",
            base + "Tools/SEGGER",
            base + "Toolchains/SEGGER",
            base + "Program Files/SEGGER",
            base + "Program Files (x86)/SEGGER"
        };
        for (const QString &sb : seggerBases) {
            collectSeggerRoot(sb);
        }

        const QStringList directJlinkBases = {
            base + "JLink",
            base + "Soft/JLink",
            base + "Software/JLink",
            base + "Tools/JLink",
            base + "Toolchains/JLink"
        };
        for (const QString &jb : directJlinkBases) {
            addDir(jlinkDirs, jb);
            collectSeggerRoot(jb);
        }

        const QStringList parentFolders = {
            base + "Soft",
            base + "Software",
            base + "Tools",
            base + "Toolchains"
        };
        for (const QString &pf : parentFolders) {
            QDir pDir(pf);
            if (!pDir.exists()) continue;
            const QFileInfoList subList = pDir.entryInfoList(
                QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
            for (const QFileInfo &sub : subList) {
                const QString lower = sub.fileName().toLower();
                if (lower.contains("segger") || lower.contains("jlink") || lower.contains("j-link")) {
                    collectSeggerRoot(sub.absoluteFilePath());
                } else if (lower.contains("keil")) {
                    addDir(keilFallbackDirs, QDir(sub.absoluteFilePath()).absoluteFilePath("ARM/Segger"));
                }
            }
        }

        const QStringList keilPaths = {
            base + "Keil_v5/ARM/Segger",
            base + "Soft/Keil_v5/ARM/Segger",
            base + "Software/Keil_v5/ARM/Segger",
            base + "Arm/Keil_v5/ARM/Segger",
            base + "Soft/Arm/Keil_v5/ARM/Segger",
            base + "Program Files/Keil_v5/ARM/Segger",
            base + "Program Files (x86)/Keil_v5/ARM/Segger"
        };
        for (const QString &kp : keilPaths) {
            addDir(keilFallbackDirs, kp);
        }
    }
    addDir(keilFallbackDirs, QDir::homePath() + "/AppData/Local/Keil_v5/ARM/Segger");
#else
    collectSeggerRoot("/opt/SEGGER");
    collectSeggerRoot("/usr/local/SEGGER");
    collectSeggerRoot("/Applications/SEGGER");
    addDir(jlinkDirs, "/usr/bin");
    addDir(jlinkDirs, "/usr/local/bin");
#endif

    // 先在独立 SEGGER J-Link 目录与环境变量中查找，再回退到 Keil 内置 Segger 目录
    QString found = findExecutable("JLink", jlinkDirs);
    if (found.isEmpty()) {
        found = findExecutable("JLinkExe", jlinkDirs);
    }
    if (found.isEmpty() && !keilFallbackDirs.isEmpty()) {
        found = findExecutable("JLink", keilFallbackDirs);
        if (found.isEmpty()) {
            found = findExecutable("JLinkExe", keilFallbackDirs);
        }
    }
    return found;
}

ToolchainPaths ToolchainDetector::autoDetect(const QString &hintDir)
{
    ToolchainPaths paths;
    emit detectionProgress(QStringLiteral("正在搜寻系统交叉编译环境与嵌入式工具链..."));

    // 构造候选目录列表
    QStringList gccDirs;
    QStringList makeDirs;
    QStringList openocdDirs;
    QStringList jlinkDirs;
    QStringList stm32ProgDirs;

    if (!hintDir.isEmpty()) {
        gccDirs << hintDir << hintDir + "/bin";
    }

    const QProcessEnvironment system = QProcessEnvironment::systemEnvironment();
    const auto addRoot = [](QStringList &dirs, const QString &path) {
        if (path.isEmpty()) return;
        QFileInfo info(path);
        const QString root = info.isFile() ? info.absolutePath() : path;
        if (QDir(root).exists()) {
            dirs << QDir::cleanPath(root) << QDir(root).absoluteFilePath("bin");
        }
    };
    for (const char *key : {"ARM_GNU_TOOLCHAIN", "ARM_GNU_TOOLCHAIN_PATH", "GNUARMEMB_TOOLCHAIN_PATH", "GCC_ARM_PATH"}) {
        addRoot(gccDirs, system.value(QString::fromLatin1(key)));
    }
    addRoot(makeDirs, system.value(QStringLiteral("MAKE_HOME")));
    addRoot(makeDirs, system.value(QStringLiteral("MINGW_HOME")));
    addRoot(openocdDirs, system.value(QStringLiteral("OPENOCD_HOME")));
    addRoot(jlinkDirs, system.value(QStringLiteral("JLINK_HOME")));
    addRoot(jlinkDirs, system.value(QStringLiteral("SEGGER_HOME")));
    addRoot(stm32ProgDirs, system.value(QStringLiteral("STM32CUBE_PROGRAMMER_HOME")));

    // Search only likely installation roots and their immediate product folders.
    // A whole-disk recursive scan would freeze the settings dialog on large drives.
    QStringList roots;
    roots << system.value(QStringLiteral("ProgramFiles"))
          << system.value(QStringLiteral("ProgramFiles(x86)"))
          << system.value(QStringLiteral("ProgramW6432"))
          << system.value(QStringLiteral("LOCALAPPDATA"))
          << system.value(QStringLiteral("ChocolateyInstall"));
#ifdef Q_OS_WIN
    for (const QFileInfo &drive : QDir::drives()) {
        const QString base = drive.absoluteFilePath();
        roots << base + "Toolchains" << base + "Tools" << base + "Soft"
              << base + "Software" << base + "SEGGER" << base + "Soft/Qt/Tools"
              << base + "Program Files" << base + "Program Files (x86)";
    }
#else
    roots << "/opt" << "/usr/local" << QDir::homePath() + "/.local";
#endif
    for (const QString &root : roots) {
        QDir directory(root);
        if (!directory.exists()) continue;
        const auto classify = [&](const QString &path, const QString &name) {
            const QString lower = name.toLower();
            if (lower.contains("arm") || lower.contains("gcc") || lower.contains("stm32cubeide")) addRoot(gccDirs, path);
            if (lower.contains("mingw") || lower.contains("msys") || lower.contains("make")) addRoot(makeDirs, path);
            if (lower.contains("openocd") || lower.contains("stm32cubeide")) addRoot(openocdDirs, path);
            if (lower.contains("segger") || lower.contains("jlink")) addRoot(jlinkDirs, path);
            if (lower.contains("stm32cubeprogrammer")) addRoot(stm32ProgDirs, path);
        };
        classify(root, directory.dirName());
        const QFileInfoList children = directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &child : children) {
            classify(child.absoluteFilePath(), child.fileName());
            const QString lower = child.fileName().toLower();
            if (lower.contains("arm") || lower.contains("gcc") || lower.contains("stm32") ||
                lower.contains("segger") || lower.contains("jlink") || lower.contains("openocd") ||
                lower.contains("mingw") || lower.contains("qt")) {
                QDir childDir(child.absoluteFilePath());
                for (const QFileInfo &product : childDir.entryInfoList(
                         QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed)) {
                    classify(product.absoluteFilePath(), product.fileName());
                    addRoot(gccDirs, product.absoluteFilePath() + "/tools");
                    addRoot(openocdDirs, product.absoluteFilePath() + "/tools/openocd");
                }
            }
        }
    }

    // 常见嵌入式工具链安装目录（优先支持当前系统探测到的 D:\Toolchains 等）
    gccDirs << "D:/Toolchains/arm-gnu-toolchain/bin"
            << "D:/Toolchains/gcc-arm-none-eabi/bin"
            << "D:/Toolchains/arm-none-eabi/bin"
            << "E:/Toolchains/gcc-arm-none-eabi/bin"
            << "E:/Toolchains/arm-gnu-toolchain/bin"
            << "C:/Toolchains/arm-gnu-toolchain/bin"
            << "C:/Program Files (x86)/Arm GNU Toolchain/bin"
            << "C:/Program Files/Arm GNU Toolchain/bin"
            << "C:/Program Files (x86)/GNU Arm Embedded Toolchain/bin"
            << "C:/Program Files/GNU Arm Embedded Toolchain/bin";

    makeDirs << "D:/Toolchains/mingw64/bin"
             << "D:/Soft/Qt/Tools/mingw1310_64/bin"
             << "D:/Soft/Qt/Tools/mingw1120_64/bin"
             << "D:/Soft/Qt/Tools/mingw810_64/bin"
             << "E:/Toolchains/mingw/mingw641520/bin"
             << "E:/Toolchains/mingw/mingw641420/bin"
             << "E:/Toolchains/mingw/mingw641310/bin"
             << "C:/ProgramData/chocolatey/bin"
             << "C:/msys64/usr/bin"
             << "C:/msys64/mingw64/bin";

    openocdDirs << "D:/Toolchains/OpenOCD/bin"
                << "E:/Toolchains/OpenOCD/bin"
                << "C:/Toolchains/OpenOCD/bin"
                << "C:/openocd/bin"
                << "C:/Program Files/OpenOCD/bin";

    jlinkDirs << "C:/Program Files/SEGGER/JLink"
              << "C:/Program Files (x86)/SEGGER/JLink"
              << "D:/Toolchains/JLink"
              << "D:/Program Files/SEGGER/JLink";

    stm32ProgDirs << "C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin"
                  << "D:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin"
                  << "D:/Soft/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin";

    // 1. 查找 GCC 系列工具
    paths.gccPath = findExecutable("arm-none-eabi-gcc", gccDirs);
    if (!paths.gccPath.isEmpty()) {
        QString gccBinDir = QFileInfo(paths.gccPath).absolutePath();
        QStringList binDirList = QStringList() << gccBinDir;
        paths.gppPath = findExecutable("arm-none-eabi-g++", binDirList);
        paths.gdbPath = findExecutable("arm-none-eabi-gdb", binDirList + gccDirs);
        paths.objcopyPath = findExecutable("arm-none-eabi-objcopy", binDirList);
        paths.sizePath = findExecutable("arm-none-eabi-size", binDirList);
    } else {
        paths.gppPath = findExecutable("arm-none-eabi-g++", gccDirs);
        paths.gdbPath = findExecutable("arm-none-eabi-gdb", gccDirs);
        paths.objcopyPath = findExecutable("arm-none-eabi-objcopy", gccDirs);
        paths.sizePath = findExecutable("arm-none-eabi-size", gccDirs);
    }

    // 2. Windows prefers the native MinGW make, which does not need an MSYS shell.
#ifdef Q_OS_WIN
    paths.makePath = findExecutable("mingw32-make", makeDirs);
    if (paths.makePath.isEmpty()) paths.makePath = findExecutable("make", makeDirs);
#else
    paths.makePath = findExecutable("make", makeDirs);
    if (paths.makePath.isEmpty()) paths.makePath = findExecutable("mingw32-make", makeDirs);
#endif

    // 3. 查找 OpenOCD
    paths.openocdPath = findExecutable("openocd", openocdDirs);
    if (!paths.openocdPath.isEmpty()) {
        // 自动探测 scripts 路径 (通常位于 openocd.exe 同级的 ../share/openocd/scripts 或 ../scripts)
        QDir binDir(QFileInfo(paths.openocdPath).absolutePath());
        QString s1 = binDir.absoluteFilePath("../share/openocd/scripts");
        QString s2 = binDir.absoluteFilePath("../scripts");
        if (QDir(s1).exists()) {
            paths.openocdScriptsDir = QDir::toNativeSeparators(QDir::cleanPath(s1));
        } else if (QDir(s2).exists()) {
            paths.openocdScriptsDir = QDir::toNativeSeparators(QDir::cleanPath(s2));
        }
    }

    // 4. 查找 J-Link（深度探测注册表、SEGGER 目录、各盘 Soft/Tools 与 Keil 内置 Segger）
    paths.jlinkPath = detectJLinkPath(hintDir);
    if (paths.jlinkPath.isEmpty()) {
        paths.jlinkPath = findExecutable("JLink", jlinkDirs);
    }
    if (paths.jlinkPath.isEmpty()) {
        paths.jlinkPath = findExecutable("JLinkExe", jlinkDirs);
    }

    // 5. 查找 STM32CubeProgrammer CLI
    paths.stm32ProgPath = findExecutable("STM32_Programmer_CLI", stm32ProgDirs);

    m_paths = paths;
    emit detectionFinished(paths);
    return paths;
}

ToolchainPaths ToolchainDetector::loadSettings(QSettings *settings)
{
    ToolchainPaths paths;
    if (!settings) return paths;

    settings->beginGroup("STM32Toolchain");
    paths.gccPath = settings->value("gccPath", "").toString();
    paths.gppPath = settings->value("gppPath", "").toString();
    paths.gdbPath = settings->value("gdbPath", "").toString();
    paths.objcopyPath = settings->value("objcopyPath", "").toString();
    paths.sizePath = settings->value("sizePath", "").toString();
    paths.makePath = settings->value("makePath", "").toString();
    paths.openocdPath = settings->value("openocdPath", "").toString();
    paths.openocdScriptsDir = settings->value("openocdScriptsDir", "").toString();
    paths.jlinkPath = settings->value("jlinkPath", "").toString();
    paths.stm32ProgPath = settings->value("stm32ProgPath", "").toString();
    settings->endGroup();

    // Keep companion tools from the user's selected GCC installation together.
    ToolchainPaths companions;
    if (paths.isGccValid() &&
        validateArmGccDir(QFileInfo(paths.gccPath).absolutePath(), companions)) {
        if (paths.gppPath.isEmpty() || !QFileInfo::exists(paths.gppPath)) paths.gppPath = companions.gppPath;
        if (!paths.isGdbValid() && QFileInfo::exists(companions.gdbPath)) paths.gdbPath = companions.gdbPath;
        if (paths.objcopyPath.isEmpty() || !QFileInfo::exists(paths.objcopyPath)) paths.objcopyPath = companions.objcopyPath;
        if (paths.sizePath.isEmpty() || !QFileInfo::exists(paths.sizePath)) paths.sizePath = companions.sizePath;
    }

    // 如果未配置或部分丢失，执行自动探测补充
    if (!paths.isGccValid() || !paths.isMakeValid() || !paths.isGdbValid() ||
        !paths.isOpenocdValid() || !paths.isJlinkValid() ||
        paths.stm32ProgPath.isEmpty()) {
        ToolchainPaths detected = autoDetect();
        if (!paths.isGccValid()) {
            paths.gccPath = detected.gccPath;
        }
        if (paths.gppPath.isEmpty() || !QFileInfo::exists(paths.gppPath)) paths.gppPath = detected.gppPath;
        if (!paths.isGdbValid()) paths.gdbPath = detected.gdbPath;
        if (paths.objcopyPath.isEmpty() || !QFileInfo::exists(paths.objcopyPath)) paths.objcopyPath = detected.objcopyPath;
        if (paths.sizePath.isEmpty() || !QFileInfo::exists(paths.sizePath)) paths.sizePath = detected.sizePath;
        if (!paths.isMakeValid()) paths.makePath = detected.makePath;
        if (!paths.isOpenocdValid()) {
            paths.openocdPath = detected.openocdPath;
            paths.openocdScriptsDir = detected.openocdScriptsDir;
        }
        if (!paths.isJlinkValid()) paths.jlinkPath = detected.jlinkPath;
        if (paths.stm32ProgPath.isEmpty() || !QFileInfo::exists(paths.stm32ProgPath))
            paths.stm32ProgPath = detected.stm32ProgPath;
    }

    m_paths = paths;
    return paths;
}

void ToolchainDetector::saveSettings(QSettings *settings, const ToolchainPaths &paths)
{
    if (!settings) return;

    settings->beginGroup("STM32Toolchain");
    settings->setValue("gccPath", paths.gccPath);
    settings->setValue("gppPath", paths.gppPath);
    settings->setValue("gdbPath", paths.gdbPath);
    settings->setValue("objcopyPath", paths.objcopyPath);
    settings->setValue("sizePath", paths.sizePath);
    settings->setValue("makePath", paths.makePath);
    settings->setValue("openocdPath", paths.openocdPath);
    settings->setValue("openocdScriptsDir", paths.openocdScriptsDir);
    settings->setValue("jlinkPath", paths.jlinkPath);
    settings->setValue("stm32ProgPath", paths.stm32ProgPath);
    settings->endGroup();
    settings->sync();

    m_paths = paths;
}

QString ToolchainDetector::getToolVersion(const QString &executablePath)
{
    if (executablePath.isEmpty() || !QFileInfo::exists(executablePath)) {
        return QStringLiteral("未找到");
    }

    QProcess process;
    process.start(executablePath, QStringList() << "--version");
    if (!process.waitForFinished(2000)) {
        process.kill();
        return QStringLiteral("超时或无响应");
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    if (output.isEmpty()) {
        output = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
    }

    if (!output.isEmpty()) {
        QString firstLine = output.split('\n').first().trimmed();
        return firstLine;
    }

    return QStringLiteral("已就绪 (无版本字符串)");
}

bool ToolchainDetector::validateArmGccDir(const QString &dirPath, ToolchainPaths &outPaths)
{
    QDir dir(dirPath);
    if (!dir.exists()) return false;

    QString binDir = dirPath;
    if (dir.exists("bin")) {
        binDir = dir.absoluteFilePath("bin");
    }

    QString gcc = QDir(binDir).absoluteFilePath("arm-none-eabi-gcc.exe");
#ifndef Q_OS_WIN
    gcc = QDir(binDir).absoluteFilePath("arm-none-eabi-gcc");
#endif

    if (QFileInfo::exists(gcc)) {
        outPaths.gccPath = QDir::toNativeSeparators(gcc);
        const QString suffix =
#ifdef Q_OS_WIN
            ".exe";
#else
            "";
#endif
        outPaths.gppPath = QDir::toNativeSeparators(QDir(binDir).absoluteFilePath("arm-none-eabi-g++" + suffix));
        outPaths.gdbPath = QDir::toNativeSeparators(QDir(binDir).absoluteFilePath("arm-none-eabi-gdb" + suffix));
        outPaths.objcopyPath = QDir::toNativeSeparators(QDir(binDir).absoluteFilePath("arm-none-eabi-objcopy" + suffix));
        outPaths.sizePath = QDir::toNativeSeparators(QDir(binDir).absoluteFilePath("arm-none-eabi-size" + suffix));
        return true;
    }
    return false;
}

QString ToolchainDetector::generateDiagnosticReport(const ToolchainPaths &paths)
{
    QString report;
    report += "========== 嵌入式交叉编译与调试环境自检报告 ==========\n";
    report += QString("ARM-GCC 编译器:  %1\n  版本: %2\n")
              .arg(paths.gccPath.isEmpty() ? "[未配置]" : paths.gccPath)
              .arg(getToolVersion(paths.gccPath));

    report += QString("GDB 调试器:      %1\n  版本: %2\n")
              .arg(paths.gdbPath.isEmpty() ? "[未配置]" : paths.gdbPath)
              .arg(getToolVersion(paths.gdbPath));

    report += QString("Make 构建工具:   %1\n  版本: %2\n")
              .arg(paths.makePath.isEmpty() ? "[未配置]" : paths.makePath)
              .arg(getToolVersion(paths.makePath));

    report += QString("OpenOCD:         %1\n  版本: %2\n  Scripts路径: %3\n")
              .arg(paths.openocdPath.isEmpty() ? "[未配置]" : paths.openocdPath)
              .arg(getToolVersion(paths.openocdPath))
              .arg(paths.openocdScriptsDir.isEmpty() ? "[默认内置]" : paths.openocdScriptsDir);

    QString jlinkStatus = QStringLiteral("未找到");
    if (paths.isJlinkValid()) {
        QFileInfo jlinkInfo(paths.jlinkPath);
        QDir jlinkDir = jlinkInfo.absoluteDir();
        const QString folderVer = jlinkDir.dirName();
        const bool hasGdbServer =
            QFileInfo::exists(jlinkDir.absoluteFilePath("JLinkGDBServerCL.exe")) ||
            QFileInfo::exists(jlinkDir.absoluteFilePath("JLinkGDBServer.exe")) ||
            QFileInfo::exists(jlinkDir.absoluteFilePath("JLinkGDBServerCLExe")) ||
            QFileInfo::exists(jlinkDir.absoluteFilePath("JLinkGDBServerExe"));
        jlinkStatus = QStringLiteral("已就绪 (%1%2)")
                          .arg(folderVer)
                          .arg(hasGdbServer ? QStringLiteral(" · 含 JLinkGDBServer") : QString());
    }
    report += QString("J-Link:          %1\n  状态: %2\n")
              .arg(paths.jlinkPath.isEmpty() ? "[未配置]" : paths.jlinkPath)
              .arg(jlinkStatus);

    report += "-----------------------------------------------------\n";
    bool canBuild = paths.isGccValid() && paths.isMakeValid();
    bool canDebug = paths.isGdbValid() && (paths.isOpenocdValid() || paths.isJlinkValid());
    bool canFlash = paths.isOpenocdValid() || paths.isJlinkValid() || !paths.stm32ProgPath.isEmpty();

    report += QString("构建能力 (Build): %1\n").arg(canBuild ? "✅ 就绪" : "❌ 缺少必要工具 (GCC/Make)");
    report += QString("烧录能力 (Flash): %1\n").arg(canFlash ? "✅ 就绪" : "⚠️ 未检测到已配置的烧录器");
    report += QString("调试能力 (Debug): %1\n").arg(canDebug ? "✅ 就绪" : "⚠️ 缺少 GDB 或 OpenOCD/J-Link");
    report += "=====================================================\n";

    return report;
}
