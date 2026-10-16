#include "ProbeDetector.h"
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <vector>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>

#ifdef Q_OS_WIN
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <usbioctl.h>
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
    if (serial.contains('&')) return {};
    // Legacy ST-LINK/V2 descriptors contain binary serial bytes. Windows may
    // truncate these at a NUL (e.g. instance "C"), whereas OpenOCD expects the
    // full 12-byte serial encoded as 24 hex digits. Such instance keys cannot
    // safely identify a probe; a single probe can be selected without a serial.
    if (instanceId.startsWith("USB\\VID_0483&PID_3748", Qt::CaseInsensitive)) {
        static const QRegularExpression stLinkSerial("^[0-9A-Fa-f]{24}$");
        if (!stLinkSerial.match(serial).hasMatch()) return {};
    }
    return serial;
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

QString ProbeDetector::stLinkSerialFromUsbDescriptor(const QByteArray &descriptor)
{
    if (descriptor.size() < 2) return {};
    const int length = static_cast<unsigned char>(descriptor.at(0));
    if (descriptor.at(1) != 3 || length > descriptor.size() ||
        (length != 26 && length != 50)) return {};
    QByteArray serial;
    for (int offset = 2; offset < length; offset += 2) serial.append(descriptor.at(offset));
    // Older ST-Link/V2 firmware exposes twelve binary bytes as a USB string.
    // Match OpenOCD's conversion, including embedded zero bytes.
    if (length == 26) return QString::fromLatin1(serial.toHex().toUpper());
    for (int offset = 3; offset < length; offset += 2)
        if (descriptor.at(offset) != 0) return {};
    const QString result = QString::fromLatin1(serial);
    static const QRegularExpression valid("^[0-9A-Fa-f]{24}$");
    return valid.match(result).hasMatch() ? result : QString();
}

#ifdef Q_OS_WIN
namespace {
struct NativeHandle {
    HANDLE value = INVALID_HANDLE_VALUE;
    ~NativeHandle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};

bool queryHub(HANDLE hub, DWORD code, QByteArray &buffer, DWORD &returned)
{
    NativeHandle event{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
    if (!event.value) return false;
    OVERLAPPED pending = {};
    pending.hEvent = event.value;
    if (DeviceIoControl(hub, code, buffer.data(), static_cast<DWORD>(buffer.size()),
                        buffer.data(), static_cast<DWORD>(buffer.size()), &returned, &pending))
        return true;
    if (GetLastError() != ERROR_IO_PENDING) return false;
    if (WaitForSingleObject(event.value, 250) != WAIT_OBJECT_0) {
        CancelIoEx(hub, &pending);
        // Keep the OVERLAPPED and buffer alive until cancellation completes.
        GetOverlappedResult(hub, &pending, &returned, TRUE);
        return false;
    }
    return GetOverlappedResult(hub, &pending, &returned, FALSE);
}

QByteArray hubDescriptor(HANDLE hub, ULONG port, UCHAR type, UCHAR index, USHORT language)
{
    QByteArray buffer(sizeof(USB_DESCRIPTOR_REQUEST) + 255, '\0');
    auto *request = reinterpret_cast<USB_DESCRIPTOR_REQUEST *>(buffer.data());
    request->ConnectionIndex = port;
    request->SetupPacket.bmRequest = 0x80;
    request->SetupPacket.bRequest = 6; // GET_DESCRIPTOR
    request->SetupPacket.wValue = static_cast<USHORT>((type << 8) | index);
    request->SetupPacket.wIndex = language;
    request->SetupPacket.wLength = 255;
    DWORD returned = 0;
    if (!queryHub(hub, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, buffer, returned) ||
        returned < sizeof(USB_DESCRIPTOR_REQUEST) + 2 || returned > static_cast<DWORD>(buffer.size()))
        return {};
    return buffer.mid(sizeof(USB_DESCRIPTOR_REQUEST), returned - sizeof(USB_DESCRIPTOR_REQUEST));
}
}

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

static QString readStLinkUsbSerialDescriptor(DEVINST physicalDevice, const QString &productId)
{
    ULONG port = 0, size = sizeof(port), kind = 0;
    if (CM_Get_DevNode_Registry_PropertyW(physicalDevice, CM_DRP_ADDRESS, &kind,
                                         &port, &size, 0) != CR_SUCCESS || !port)
        return {};
    DEVINST parent = 0;
    if (CM_Get_Parent(&parent, physicalDevice, 0) != CR_SUCCESS) return {};
    const std::wstring parentId = deviceId(parent).toStdWString();
    GUID hubGuid = {0xf18a0e88, 0xc30c, 0x11d0, {0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8}};
    ULONG chars = 0;
    if (CM_Get_Device_Interface_List_SizeW(&chars, &hubGuid,
            const_cast<wchar_t *>(parentId.c_str()), CM_GET_DEVICE_INTERFACE_LIST_PRESENT) != CR_SUCCESS ||
        chars < 2) return {};
    std::vector<wchar_t> paths(chars, L'\0');
    if (CM_Get_Device_Interface_ListW(&hubGuid, const_cast<wchar_t *>(parentId.c_str()),
            paths.data(), chars, CM_GET_DEVICE_INTERFACE_LIST_PRESENT) != CR_SUCCESS) return {};
    NativeHandle hub{CreateFileW(paths.data(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr)};
    if (hub.value == INVALID_HANDLE_VALUE) return {};
    const QByteArray device = hubDescriptor(hub.value, port, 1, 0, 0);
    if (device.size() < 18 || device.at(1) != 1) return {};
    const auto byte = [&](int offset) { return static_cast<unsigned char>(device.at(offset)); };
    const quint16 vendor = byte(8) | (byte(9) << 8);
    const quint16 product = byte(10) | (byte(11) << 8);
    if (vendor != 0x0483 || product != productId.toUShort(nullptr, 16) || byte(16) == 0) return {};
    const QByteArray languages = hubDescriptor(hub.value, port, 3, 0, 0);
    if (languages.size() < 4 || languages.at(1) != 3) return {};
    const USHORT language = static_cast<unsigned char>(languages.at(2)) |
                           (static_cast<unsigned char>(languages.at(3)) << 8);
    return ProbeDetector::stLinkSerialFromUsbDescriptor(
        hubDescriptor(hub.value, port, 3, byte(16), language));
}

static QString readStLinkUsbSerial(DEVINST physicalDevice, const QString &productId)
{
    // A faulty USB driver can delay cancellation indefinitely. Keep the request,
    // handles and buffers together on a worker while bounding the UI wait. Cap
    // outstanding workers so repeated refreshes cannot accumulate hung requests.
    static std::atomic<int> activeQueries{0};
    if (activeQueries.fetch_add(1) >= 2) {
        activeQueries.fetch_sub(1);
        return {};
    }
    std::packaged_task<QString()> query([physicalDevice, productId] {
        struct QueryGuard { ~QueryGuard() { activeQueries.fetch_sub(1); } } guard;
        return readStLinkUsbSerialDescriptor(physicalDevice, productId);
    });
    auto result = query.get_future();
    try {
        std::thread(std::move(query)).detach();
    } catch (const std::system_error &) {
        activeQueries.fetch_sub(1);
        return {};
    }
    if (result.wait_for(std::chrono::milliseconds(750)) != std::future_status::ready) return {};
    try { return result.get(); }
    catch (const std::exception &) { return {}; }
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
        DEVINST physicalUsbNode = 0;
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
                    physicalUsbNode = node;
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
        if (info.type == ProbeType::STLink && info.serial.isEmpty() && physicalUsbNode)
            info.serial = readStLinkUsbSerial(physicalUsbNode, info.productId);
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
