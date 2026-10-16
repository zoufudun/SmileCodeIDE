#include "STM32Profile.h"
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QRegularExpression>

STM32ProfileManager& STM32ProfileManager::instance()
{
    static STM32ProfileManager mgr;
    return mgr;
}

STM32ProfileManager::STM32ProfileManager()
{
    initDatabase();
}

void STM32ProfileManager::initDatabase()
{
    // 1. STM32F1 系列
    {
        STM32ChipProfile p;
        p.chipName = "STM32F103C8";
        p.family = "STM32F1";
        p.core = "cortex-m3";
        p.hasFpu = false;
        p.defaultMacro = "STM32F103xB";
        p.extraMacros << "USE_HAL_DRIVER";
        p.flashSizeBytes = 64 * 1024;
        p.ramSizeBytes = 20 * 1024;
        p.flashOrigin = "0x08000000";
        p.ramOrigin = "0x20000000";
        p.openocdTargetCfg = "target/stm32f1x.cfg";
        p.jlinkDeviceName = "STM32F103C8";
        p.defaultLdFile = "STM32F103C8Tx_FLASH.ld";
        m_database["STM32F103C8"] = p;
    }
    {
        STM32ChipProfile p;
        p.chipName = "STM32F103RC";
        p.family = "STM32F1";
        p.core = "cortex-m3";
        p.hasFpu = false;
        p.defaultMacro = "STM32F103xE";
        p.extraMacros << "USE_HAL_DRIVER";
        p.flashSizeBytes = 256 * 1024;
        p.ramSizeBytes = 48 * 1024;
        p.openocdTargetCfg = "target/stm32f1x.cfg";
        p.jlinkDeviceName = "STM32F103RC";
        p.defaultLdFile = "STM32F103RCTx_FLASH.ld";
        m_database["STM32F103RC"] = p;
    }
    {
        STM32ChipProfile p;
        p.chipName = "STM32F103ZE";
        p.family = "STM32F1";
        p.core = "cortex-m3";
        p.hasFpu = false;
        p.defaultMacro = "STM32F103xE";
        p.extraMacros << "USE_HAL_DRIVER";
        p.flashSizeBytes = 512 * 1024;
        p.ramSizeBytes = 64 * 1024;
        p.openocdTargetCfg = "target/stm32f1x.cfg";
        p.jlinkDeviceName = "STM32F103ZE";
        p.defaultLdFile = "STM32F103ZETx_FLASH.ld";
        m_database["STM32F103ZE"] = p;
    }

    // 2. STM32F4 系列
    {
        STM32ChipProfile p;
        p.chipName = "STM32F407ZG";
        p.family = "STM32F4";
        p.core = "cortex-m4";
        p.hasFpu = true;
        p.fpuFlags = "-mfpu=fpv4-sp-d16 -mfloat-abi=hard";
        p.defaultMacro = "STM32F407xx";
        p.extraMacros << "USE_HAL_DRIVER";
        p.flashSizeBytes = 1024 * 1024;
        p.ramSizeBytes = 128 * 1024;
        p.openocdTargetCfg = "target/stm32f4x.cfg";
        p.jlinkDeviceName = "STM32F407ZG";
        p.defaultLdFile = "STM32F407ZGTx_FLASH.ld";
        m_database["STM32F407ZG"] = p;
        m_database["STM32F407VG"] = p;
        m_database["STM32F407VE"] = p;
    }
    {
        STM32ChipProfile p;
        p.chipName = "STM32F401CC";
        p.family = "STM32F4";
        p.core = "cortex-m4";
        p.hasFpu = true;
        p.fpuFlags = "-mfpu=fpv4-sp-d16 -mfloat-abi=hard";
        p.defaultMacro = "STM32F401xC";
        p.extraMacros << "USE_HAL_DRIVER";
        p.flashSizeBytes = 256 * 1024;
        p.ramSizeBytes = 64 * 1024;
        p.openocdTargetCfg = "target/stm32f4x.cfg";
        p.jlinkDeviceName = "STM32F401CC";
        p.defaultLdFile = "STM32F401CCTx_FLASH.ld";
        m_database["STM32F401CC"] = p;
    }

    // 3. STM32F7 系列
    {
        STM32ChipProfile p;
        p.chipName = "STM32F767IG";
        p.family = "STM32F7";
        p.core = "cortex-m7";
        p.hasFpu = true;
        p.fpuFlags = "-mfpu=fpv5-d16 -mfloat-abi=hard";
        p.defaultMacro = "STM32F767xx";
        p.extraMacros << "USE_HAL_DRIVER";
        p.flashSizeBytes = 1024 * 1024;
        p.ramSizeBytes = 512 * 1024;
        p.openocdTargetCfg = "target/stm32f7x.cfg";
        p.jlinkDeviceName = "STM32F767IG";
        p.defaultLdFile = "STM32F767IGTx_FLASH.ld";
        m_database["STM32F767IG"] = p;
    }

    // 4. STM32H7 系列
    {
        STM32ChipProfile p;
        p.chipName = "STM32H743VI";
        p.family = "STM32H7";
        p.core = "cortex-m7";
        p.hasFpu = true;
        p.fpuFlags = "-mfpu=fpv5-d16 -mfloat-abi=hard";
        p.defaultMacro = "STM32H743xx";
        p.extraMacros << "USE_HAL_DRIVER";
        p.flashSizeBytes = 2048 * 1024;
        p.ramSizeBytes = 1024 * 1024;
        // 0x20000000 起始的 DTCM 只有 128KB，其余 RAM 位于其他地址段。
        // 默认链接到 DTCM，避免把总 RAM 误当成连续空间而产生无效栈顶 0x20100000。
        p.linkerRamSizeBytes = 128 * 1024;
        p.openocdTargetCfg = "target/stm32h7x.cfg";
        p.jlinkDeviceName = "STM32H743VI";
        p.defaultLdFile = "STM32H743VITx_FLASH.ld";
        p.linkerResourcePath = ":/stm32/linker/STM32H743VITX_FLASH.ld";
        p.linkerSha256 = "a80674f7089fed7e7e0924e9f6742d70915cfb259d908a7b8eaa7f4ef4779bcf";
        m_database["STM32H743VI"] = p;

        // 兼容通用 H7 目标与 FLM 烧录算法标识
        {
            STM32ChipProfile p2 = p;
            p2.chipName = "STM32H7x_2M";
            m_database["STM32H7X_2M"] = p2;
        }
        {
            STM32ChipProfile p1 = p;
            p1.chipName = "STM32H7x_1M";
            p1.flashSizeBytes = 1024 * 1024;
            m_database["STM32H7X_1M"] = p1;
        }
    }

    // 5. STM32G0 / G4 系列
    {
        STM32ChipProfile p;
        p.chipName = "STM32G474CE";
        p.family = "STM32G4";
        p.core = "cortex-m4";
        p.hasFpu = true;
        p.fpuFlags = "-mfpu=fpv4-sp-d16 -mfloat-abi=hard";
        p.defaultMacro = "STM32G474xx";
        p.extraMacros << "USE_HAL_DRIVER";
        p.flashSizeBytes = 512 * 1024;
        p.ramSizeBytes = 128 * 1024;
        p.openocdTargetCfg = "target/stm32g4x.cfg";
        p.jlinkDeviceName = "STM32G474CE";
        p.defaultLdFile = "STM32G474CETx_FLASH.ld";
        m_database["STM32G474CE"] = p;
        {
            STM32ChipProfile pg = p;
            pg.chipName = "STM32G4xx_512";
            m_database["STM32G4XX_512"] = pg;
        }
    }

    // 扩展通用系列算法标识
    if (m_database.contains("STM32F103C8")) {
        STM32ChipProfile pf1 = m_database["STM32F103C8"];
        pf1.chipName = "STM32F10x_128";
        pf1.flashSizeBytes = 128 * 1024;
        m_database["STM32F10X_128"] = pf1;

        pf1.chipName = "STM32F10x_64";
        pf1.flashSizeBytes = 64 * 1024;
        m_database["STM32F10X_64"] = pf1;
    }
    if (m_database.contains("STM32F103ZE")) {
        STM32ChipProfile pf1h = m_database["STM32F103ZE"];
        pf1h.chipName = "STM32F10x_512";
        pf1h.flashSizeBytes = 512 * 1024;
        m_database["STM32F10X_512"] = pf1h;
    }
    if (m_database.contains("STM32F407ZG")) {
        STM32ChipProfile pf4 = m_database["STM32F407ZG"];
        pf4.chipName = "STM32F4xx_1024";
        m_database["STM32F4XX_1024"] = pf4;
    }
    if (m_database.contains("STM32F401CC")) {
        STM32ChipProfile pf4s = m_database["STM32F401CC"];
        pf4s.chipName = "STM32F4xx_512";
        pf4s.flashSizeBytes = 512 * 1024;
        pf4s.ramSizeBytes = 96 * 1024;
        m_database["STM32F4XX_512"] = pf4s;
    }
    if (m_database.contains("STM32F767IG")) {
        STM32ChipProfile pf7 = m_database["STM32F767IG"];
        pf7.chipName = "STM32F7x_2M";
        pf7.flashSizeBytes = 2048 * 1024;
        m_database["STM32F7X_2M"] = pf7;

        pf7.chipName = "STM32F7x_1M";
        pf7.flashSizeBytes = 1024 * 1024;
        m_database["STM32F7X_1M"] = pf7;
    }

    m_supportedChips = m_database.keys();
    loadBundledCatalog();
}

QStringList STM32ProfileManager::supportedChips() const
{
    return m_supportedChips;
}

void STM32ProfileManager::loadBundledCatalog()
{
    QFile manifest(":/stm32/full/manifest.json");
    if (!manifest.open(QIODevice::ReadOnly)) return;
    const QJsonDocument document = QJsonDocument::fromJson(manifest.readAll());
    if (!document.isObject()) return;
    const QJsonObject targets = document.object().value("targets").toObject();
    for (auto it = targets.begin(); it != targets.end(); ++it) {
        const QJsonObject target = it.value().toObject();
        const QString reference = target.value("referenceName").toString();
        const QString family = target.value("family").toString();
        if (reference.isEmpty() || family.isEmpty()) continue;
        const QJsonArray scripts = target.value("scripts").toArray();
        QList<QJsonObject> flashScripts;
        for (const QJsonValue &value : scripts) {
            const QJsonObject script = value.toObject();
            if (QFileInfo(script.value("path").toString()).fileName()
                    .contains("FLASH", Qt::CaseInsensitive))
                flashScripts << script;
        }
        for (const QJsonObject &script : flashScripts) {
            const QString path = script.value("path").toString();
            const QString sourcePath = script.value("sourceRelativePath").toString().toUpper();
            QString context;
            if (sourcePath.contains("/CM4/") || sourcePath.contains("_CM4_")) context = "CM4";
            else if (sourcePath.contains("/CM7/") || sourcePath.contains("_CM7_")) context = "CM7";
            const QString key = flashScripts.size() == 1 ? reference
                : reference + "@" + (context.isEmpty() ? QFileInfo(path).baseName() : context);
            STM32ChipProfile profile;
            profile.chipName = reference;
            profile.family = family;
            const QString core = context.isEmpty()
                ? target.value("core").toString() : "Arm Cortex-M" + context.right(1);
            if (core.contains("M3")) profile.core = "cortex-m3";
            else if (core.contains("M4")) profile.core = "cortex-m4";
            else if (core.contains("M7")) profile.core = "cortex-m7";
            else continue;
            profile.flashSizeBytes = target.value("flashKiB").toString().toUInt() * 1024;
            profile.ramSizeBytes = target.value("ramKiB").toString().toUInt() * 1024;
            profile.openocdTargetCfg = getOpenocdTarget(family);
            profile.jlinkDeviceName = reference;
            profile.linkerContext = context;
            profile.defaultLdFile = context.isEmpty()
                ? QFileInfo(path).fileName()
                : context + "/" + QFileInfo(path).fileName();
            profile.linkerResourcePath = ":/stm32/full/" + path;
            profile.linkerSha256 = script.value("sha256").toString();
            QFile linker(profile.linkerResourcePath);
            if (!linker.open(QIODevice::ReadOnly)) continue;
            const QByteArray linkerBytes = linker.readAll();
            if (QString::fromLatin1(QCryptographicHash::hash(
                    linkerBytes, QCryptographicHash::Sha256).toHex())
                    .compare(profile.linkerSha256, Qt::CaseInsensitive) != 0) continue;
            static const QRegularExpression flashRegion(
                R"((?m)^\s*FLASH[A-Z0-9_]*\s*\([^\r\n]*\)\s*:\s*ORIGIN\s*=\s*(0x[0-9A-Fa-f]+))",
                QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch originMatch =
                flashRegion.match(QString::fromLatin1(linkerBytes));
            if (originMatch.hasMatch()) profile.flashOrigin = originMatch.captured(1);
            const QString fpu = script.value("fpu").toString();
            const QString floatAbi = script.value("floatAbi").toString();
            if (!fpu.isEmpty() && !floatAbi.isEmpty()) {
                profile.hasFpu = true;
                profile.fpuFlags = "-mfpu=" + fpu + " -mfloat-abi=" + floatAbi;
            }
            for (const QJsonValue &define : script.value("defines").toArray()) {
                const QString symbol = define.toString();
                if (symbol.startsWith("STM32") && profile.defaultMacro.isEmpty())
                    profile.defaultMacro = symbol;
                else if (!symbol.isEmpty())
                    profile.extraMacros << symbol;
            }
            if (path.isEmpty() || profile.linkerSha256.size() != 64) continue;
            m_database.insert(key.toUpper(), profile);
            if (!m_supportedChips.contains(key)) m_supportedChips << key;
            if (context == "CM7") {
                m_database.insert(reference.toUpper(), profile);
                if (!m_supportedChips.contains(reference)) m_supportedChips << reference;
            }
            for (const QJsonValue &alias : target.value("cpnAliases").toArray()) {
                const QString partNumber = alias.toString();
                if (partNumber.isEmpty()) continue;
                const QString choice = context.isEmpty() ? partNumber
                                                        : partNumber + "@" + context;
                m_database.insert(choice.toUpper(), profile);
                if (!m_supportedChips.contains(choice)) m_supportedChips << choice;
                // Dual-core order codes without a suffix select the primary CM7 core.
                if (context.isEmpty() || context == "CM7") {
                    m_database.insert(partNumber.toUpper(), profile);
                    if (!m_supportedChips.contains(partNumber)) m_supportedChips << partNumber;
                }
            }
        }
    }
    m_supportedChips.sort(Qt::CaseInsensitive);
}

STM32ChipProfile STM32ProfileManager::getProfile(const QString &chipName) const
{
    QString trimmed = chipName.trimmed().toUpper();
    if (m_database.contains(trimmed)) {
        return m_database.value(trimmed);
    }

    return STM32ChipProfile();
}

QString STM32ProfileManager::getOpenocdTarget(const QString &familyOrChip)
{
    QString upper = familyOrChip.toUpper();
    if (upper.startsWith("STM32C0")) return "target/stm32c0x.cfg";
    if (upper.startsWith("STM32F0")) return "target/stm32f0x.cfg";
    if (upper.startsWith("STM32F1")) return "target/stm32f1x.cfg";
    if (upper.startsWith("STM32F2")) return "target/stm32f2x.cfg";
    if (upper.startsWith("STM32F3")) return "target/stm32f3x.cfg";
    if (upper.startsWith("STM32F4")) return "target/stm32f4x.cfg";
    if (upper.startsWith("STM32F7")) return "target/stm32f7x.cfg";
    if (upper.startsWith("STM32G0")) return "target/stm32g0x.cfg";
    if (upper.startsWith("STM32G4")) return "target/stm32g4x.cfg";
    if (upper.startsWith("STM32H7")) return "target/stm32h7x.cfg";
    if (upper.startsWith("STM32L0")) return "target/stm32l0.cfg";
    if (upper.startsWith("STM32L1")) return "target/stm32l1.cfg";
    if (upper.startsWith("STM32L4")) return "target/stm32l4x.cfg";
    if (upper.startsWith("STM32L5")) return "target/stm32l5x.cfg";
    if (upper.startsWith("STM32U5")) return "target/stm32u5x.cfg";
    if (upper.startsWith("STM32WBA")) return "target/stm32wbax.cfg";
    if (upper.startsWith("STM32WB")) return "target/stm32wbx.cfg";
    return QString();
}

QString STM32ProfileManager::generateLinkerScript(const STM32ChipProfile &profile)
{
    QString ld;
    QTextStream out(&ld);

    QString flashK = QString::number(profile.flashSizeBytes / 1024) + "K";
    const quint32 linkerRamSize = profile.linkerRamSizeBytes > 0
                                      ? profile.linkerRamSizeBytes
                                      : profile.ramSizeBytes;
    QString ramK = QString::number(linkerRamSize / 1024) + "K";
    QString totalRamK = QString::number(profile.ramSizeBytes / 1024) + "K";

    out << "/*\n";
    out << " * SmileCodeIDE Auto-Generated Linker Script for " << profile.chipName << "\n";
    out << " * Core: " << profile.core << "\n";
    out << " * FLASH: " << flashK << " (" << profile.flashOrigin << ")\n";
    out << " * RAM:   " << ramK << " (" << profile.ramOrigin << ")\n";
    if (linkerRamSize != profile.ramSizeBytes)
        out << " * Total on-chip RAM: " << totalRamK << " (split across multiple regions)\n";
    out << " */\n\n";

    out << "ENTRY(Reset_Handler)\n\n";
    out << "_estack = ORIGIN(RAM) + LENGTH(RAM);\n";
    out << "_Min_Heap_Size = 0x200;\n";
    out << "_Min_Stack_Size = 0x400;\n\n";

    out << "MEMORY\n{\n";
    out << "  RAM    (xrw) : ORIGIN = " << profile.ramOrigin << ", LENGTH = " << ramK << "\n";
    out << "  FLASH  (rx)  : ORIGIN = " << profile.flashOrigin << ", LENGTH = " << flashK << "\n";
    out << "}\n\n";

    out << "SECTIONS\n{\n";
    out << "  .isr_vector :\n  {\n";
    out << "    . = ALIGN(4);\n";
    out << "    KEEP(*(.isr_vector))\n";
    out << "    . = ALIGN(4);\n";
    out << "  } >FLASH\n\n";

    out << "  .text :\n  {\n";
    out << "    . = ALIGN(4);\n";
    out << "    *(.text)\n";
    out << "    *(.text*)\n";
    out << "    *(.glue_7)\n";
    out << "    *(.glue_7t)\n";
    out << "    *(.eh_frame)\n";
    out << "    KEEP (*(.init))\n";
    out << "    KEEP (*(.fini))\n";
    out << "    . = ALIGN(4);\n";
    out << "    _etext = .;\n";
    out << "  } >FLASH\n\n";

    out << "  .rodata :\n  {\n";
    out << "    . = ALIGN(4);\n";
    out << "    *(.rodata)\n";
    out << "    *(.rodata*)\n";
    out << "    . = ALIGN(4);\n";
    out << "  } >FLASH\n\n";

    out << "  .ARM.extab :\n  {\n";
    out << "    *(.ARM.extab* .gnu.linkonce.armextab.*)\n";
    out << "  } >FLASH\n\n";

    out << "  .ARM :\n  {\n";
    out << "    __exidx_start = .;\n";
    out << "    *(.ARM.exidx*)\n";
    out << "    __exidx_end = .;\n";
    out << "  } >FLASH\n\n";

    out << "  _sidata = LOADADDR(.data);\n";
    out << "  .data :\n  {\n";
    out << "    . = ALIGN(4);\n";
    out << "    _sdata = .;\n";
    out << "    *(.data)\n";
    out << "    *(.data*)\n";
    out << "    . = ALIGN(4);\n";
    out << "    _edata = .;\n";
    out << "  } >RAM AT> FLASH\n\n";

    out << "  .bss :\n  {\n";
    out << "    . = ALIGN(4);\n";
    out << "    _sbss = .;\n";
    out << "    __bss_start__ = _sbss;\n";
    out << "    *(.bss)\n";
    out << "    *(.bss*)\n";
    out << "    *(COMMON)\n";
    out << "    . = ALIGN(4);\n";
    out << "    _ebss = .;\n";
    out << "    __bss_end__ = _ebss;\n";
    out << "  } >RAM\n\n";

    out << "  ._user_heap_stack :\n  {\n";
    out << "    . = ALIGN(8);\n";
    out << "    PROVIDE ( end = . );\n";
    out << "    PROVIDE ( _end = . );\n";
    out << "    . = . + _Min_Heap_Size;\n";
    out << "    . = . + _Min_Stack_Size;\n";
    out << "    . = ALIGN(8);\n";
    out << "  } >RAM\n\n";

    out << "  /DISCARD/ :\n  {\n";
    out << "    libc.a ( * )\n";
    out << "    libm.a ( * )\n";
    out << "    libgcc.a ( * )\n";
    out << "  }\n";
    out << "}\n";

    return ld;
}
