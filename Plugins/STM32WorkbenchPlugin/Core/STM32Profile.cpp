#include "STM32Profile.h"
#include <QTextStream>

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
    }
}

QStringList STM32ProfileManager::supportedChips() const
{
    return m_database.keys();
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
