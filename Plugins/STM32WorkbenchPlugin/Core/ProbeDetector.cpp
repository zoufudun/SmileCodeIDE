#include "ProbeDetector.h"
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>

#ifdef Q_OS_WIN
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#endif

ProbeType ProbeDetector::classify(const QString &hardwareId, const QString &name)
{
    const QString id = hardwareId.toUpper();
    const QString label = name.toUpper();
    if (label.contains("ST-LINK") || label.contains("STLINK") || (id.contains("VID_0483") &&
        QRegularExpression("PID_37(?:44|48|4A|4B|4D|4E|4F|52|53|54|57)").match(id).hasMatch()))
        return ProbeType::STLink;
    if (id.contains("VID_1366") || label.contains("J-LINK") || label.contains("JLINK"))
        return ProbeType::JLink;
    if (id.contains("VID_0D28&PID_0204") || label.contains("CMSIS-DAP") ||
        label.contains("DAPLINK") || label.contains("DAP-LINK"))
        return ProbeType::CmsisDap;
    return ProbeType::Unknown;
}

QString ProbeDetector::serialFromInstanceId(const QString &instanceId)
{
    if (instanceId.contains("&MI_", Qt::CaseInsensitive)) return {};
    static const QRegularExpression usbId(
        R"(^USB\\VID_[0-9A-F]{4}&PID_[0-9A-F]{4}(?:&[^\\]*)?\\([^\\]+)$)",
        QRegularExpression::CaseInsensitiveOption);
    const auto match = usbId.match(instanceId);
    if (!match.hasMatch()) return {};
    const QString serial = match.captured(1);
    // Composite interfaces typically have a Windows-generated instance key, not a USB serial.
    return serial.contains('&') ? QString() : serial;
}

QString ProbeDetector::typeName(ProbeType type)
{
    switch (type) {
    case ProbeType::STLink: return QStringLiteral("ST-LINK");
    case ProbeType::CmsisDap: return QStringLiteral("CMSIS-DAP / DAPLink");
    case ProbeType::JLink: return QStringLiteral("J-Link");
    default: return QStringLiteral("未知");
    }
}

#ifdef Q_OS_WIN
static QString deviceProperty(HDEVINFO devices, SP_DEVINFO_DATA &device, DWORD property)
{
    wchar_t value[1024] = {};
    DWORD kind = 0;
    if (!SetupDiGetDeviceRegistryPropertyW(devices, &device, property, &kind,
                                            reinterpret_cast<PBYTE>(value), sizeof(value), nullptr))
        return {};
    return QString::fromWCharArray(value);
}

static QString deviceId(DEVINST instance)
{
    wchar_t value[MAX_DEVICE_ID_LEN] = {};
    if (CM_Get_Device_IDW(instance, value, MAX_DEVICE_ID_LEN, 0) != CR_SUCCESS) return {};
    return QString::fromWCharArray(value);
}

static QString nodeProperty(DEVINST instance, ULONG property)
{
    wchar_t value[1024] = {};
    ULONG bytes = sizeof(value);
    if (CM_Get_DevNode_Registry_PropertyW(instance, property, nullptr, value, &bytes, 0) != CR_SUCCESS)
        return {};
    return QString::fromWCharArray(value);
}
#else
static QString fileValue(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()).trimmed() : QString();
}
#endif

QList<ProbeInfo> ProbeDetector::scan()
{
    QList<ProbeInfo> results;
    QSet<QString> seen;
#ifdef Q_OS_WIN
    HDEVINFO devices = SetupDiGetClassDevsW(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (devices == INVALID_HANDLE_VALUE) return results;
    for (DWORD index = 0; ; ++index) {
        SP_DEVINFO_DATA device = {};
        device.cbSize = sizeof(device);
        if (!SetupDiEnumDeviceInfo(devices, index, &device)) break;
        const QString name = deviceProperty(devices, device, SPDRP_FRIENDLYNAME);
        const QString description = deviceProperty(devices, device, SPDRP_DEVICEDESC);
        const QString manufacturer = deviceProperty(devices, device, SPDRP_MFG);
        const QString hardwareId = deviceProperty(devices, device, SPDRP_HARDWAREID);
        QString visibleName = name.isEmpty() ? description : name;
        QString visibleManufacturer = manufacturer;
        DEVINST node = device.DevInst;
        QString usbInstance;
        QString serial;
        ProbeType type = classify(hardwareId, visibleName);
        for (int depth = 0; depth < 6; ++depth) {
            const QString currentId = deviceId(node);
            if (currentId.startsWith("USB\\VID_", Qt::CaseInsensitive)) {
                QString parentName = nodeProperty(node, CM_DRP_FRIENDLYNAME);
                if (parentName.isEmpty()) parentName = nodeProperty(node, CM_DRP_DEVICEDESC);
                if (classify(currentId, parentName) != ProbeType::Unknown) visibleName = parentName;
                if (visibleManufacturer.isEmpty()) visibleManufacturer = nodeProperty(node, CM_DRP_MFG);
                // Composite interface IDs contain MI_XX. Stop at the physical USB device;
                // climbing higher could mistake an upstream hub's serial for the probe's.
                usbInstance = currentId;
                if (type == ProbeType::Unknown) type = classify(currentId, visibleName);
                if (!currentId.contains("&MI_", Qt::CaseInsensitive)) {
                    serial = serialFromInstanceId(currentId);
                    break;
                }
            }
            DEVINST parent = 0;
            if (CM_Get_Parent(&parent, node, 0) != CR_SUCCESS) break;
            node = parent;
        }
        if (type == ProbeType::Unknown || usbInstance.isEmpty()) continue;
        const QString key = QString::number(static_cast<int>(type)) + ':' + usbInstance.toUpper();
        if (seen.contains(key)) continue;
        seen.insert(key);
        ProbeInfo info;
        info.type = type;
        info.name = visibleName.isEmpty() ? typeName(type) : visibleName;
        info.manufacturer = visibleManufacturer;
        info.serial = serial;
        info.instanceId = usbInstance;
        static const QRegularExpression ids("VID_([0-9A-F]{4})&PID_([0-9A-F]{4})", QRegularExpression::CaseInsensitiveOption);
        const auto match = ids.match(usbInstance);
        if (match.hasMatch()) { info.vendorId = match.captured(1).toUpper(); info.productId = match.captured(2).toUpper(); }
        results.append(info);
    }
    SetupDiDestroyDeviceInfoList(devices);
#elif defined(Q_OS_LINUX)
    const QDir usb("/sys/bus/usb/devices");
    for (const QString &entry : usb.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString base = usb.absoluteFilePath(entry);
        const QString vid = fileValue(base + "/idVendor").toUpper();
        const QString pid = fileValue(base + "/idProduct").toUpper();
        if (vid.isEmpty() || pid.isEmpty()) continue;
        const QString name = fileValue(base + "/product");
        const ProbeType type = classify("VID_" + vid + "&PID_" + pid, name);
        if (type == ProbeType::Unknown) continue;
        ProbeInfo info;
        info.type = type;
        info.name = name.isEmpty() ? typeName(type) : name;
        info.manufacturer = fileValue(base + "/manufacturer");
        info.vendorId = vid;
        info.productId = pid;
        info.serial = fileValue(base + "/serial");
        info.instanceId = base;
        results.append(info);
    }
#endif
    return results;
}
