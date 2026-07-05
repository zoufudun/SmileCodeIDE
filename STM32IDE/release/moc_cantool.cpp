/****************************************************************************
** Meta object code from reading C++ file 'cantool.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../cantool.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'cantool.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CANTool_t {
    QByteArrayData data[37];
    char stringdata0[385];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CANTool_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CANTool_t qt_meta_stringdata_CANTool = {
    {
QT_MOC_LITERAL(0, 0, 7), // "CANTool"
QT_MOC_LITERAL(1, 8, 14), // "onDeviceManage"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 13), // "onSendClicked"
QT_MOC_LITERAL(4, 38, 14), // "onClearReceive"
QT_MOC_LITERAL(5, 53, 14), // "onCanConnected"
QT_MOC_LITERAL(6, 68, 17), // "onCanDisconnected"
QT_MOC_LITERAL(7, 86, 10), // "onCanError"
QT_MOC_LITERAL(8, 97, 7), // "message"
QT_MOC_LITERAL(9, 105, 15), // "onFrameReceived"
QT_MOC_LITERAL(10, 121, 8), // "CanFrame"
QT_MOC_LITERAL(11, 130, 5), // "frame"
QT_MOC_LITERAL(12, 136, 11), // "onFrameSent"
QT_MOC_LITERAL(13, 148, 9), // "onNmtSend"
QT_MOC_LITERAL(14, 158, 9), // "onSdoRead"
QT_MOC_LITERAL(15, 168, 10), // "onSdoWrite"
QT_MOC_LITERAL(16, 179, 10), // "onSyncSend"
QT_MOC_LITERAL(17, 190, 11), // "onHeartbeat"
QT_MOC_LITERAL(18, 202, 6), // "nodeId"
QT_MOC_LITERAL(19, 209, 8), // "NmtState"
QT_MOC_LITERAL(20, 218, 5), // "state"
QT_MOC_LITERAL(21, 224, 6), // "onEmcy"
QT_MOC_LITERAL(22, 231, 9), // "errorCode"
QT_MOC_LITERAL(23, 241, 13), // "errorRegister"
QT_MOC_LITERAL(24, 255, 12), // "manufacturer"
QT_MOC_LITERAL(25, 268, 5), // "onPdo"
QT_MOC_LITERAL(26, 274, 9), // "pdoNumber"
QT_MOC_LITERAL(27, 284, 6), // "isTpdo"
QT_MOC_LITERAL(28, 291, 4), // "data"
QT_MOC_LITERAL(29, 296, 17), // "onSdoReadFinished"
QT_MOC_LITERAL(30, 314, 7), // "success"
QT_MOC_LITERAL(31, 322, 5), // "index"
QT_MOC_LITERAL(32, 328, 8), // "subIndex"
QT_MOC_LITERAL(33, 337, 5), // "value"
QT_MOC_LITERAL(34, 343, 9), // "abortCode"
QT_MOC_LITERAL(35, 353, 18), // "onSdoWriteFinished"
QT_MOC_LITERAL(36, 372, 12) // "onCanOpenLog"

    },
    "CANTool\0onDeviceManage\0\0onSendClicked\0"
    "onClearReceive\0onCanConnected\0"
    "onCanDisconnected\0onCanError\0message\0"
    "onFrameReceived\0CanFrame\0frame\0"
    "onFrameSent\0onNmtSend\0onSdoRead\0"
    "onSdoWrite\0onSyncSend\0onHeartbeat\0"
    "nodeId\0NmtState\0state\0onEmcy\0errorCode\0"
    "errorRegister\0manufacturer\0onPdo\0"
    "pdoNumber\0isTpdo\0data\0onSdoReadFinished\0"
    "success\0index\0subIndex\0value\0abortCode\0"
    "onSdoWriteFinished\0onCanOpenLog"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CANTool[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  104,    2, 0x08 /* Private */,
       3,    0,  105,    2, 0x08 /* Private */,
       4,    0,  106,    2, 0x08 /* Private */,
       5,    0,  107,    2, 0x08 /* Private */,
       6,    0,  108,    2, 0x08 /* Private */,
       7,    1,  109,    2, 0x08 /* Private */,
       9,    1,  112,    2, 0x08 /* Private */,
      12,    1,  115,    2, 0x08 /* Private */,
      13,    0,  118,    2, 0x08 /* Private */,
      14,    0,  119,    2, 0x08 /* Private */,
      15,    0,  120,    2, 0x08 /* Private */,
      16,    0,  121,    2, 0x08 /* Private */,
      17,    2,  122,    2, 0x08 /* Private */,
      21,    4,  127,    2, 0x08 /* Private */,
      25,    4,  136,    2, 0x08 /* Private */,
      29,    5,  145,    2, 0x08 /* Private */,
      35,    4,  156,    2, 0x08 /* Private */,
      36,    1,  165,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::UChar, 0x80000000 | 19,   18,   20,
    QMetaType::Void, QMetaType::UChar, QMetaType::UShort, QMetaType::UChar, QMetaType::QByteArray,   18,   22,   23,   24,
    QMetaType::Void, QMetaType::UChar, QMetaType::Int, QMetaType::Bool, QMetaType::QByteArray,   18,   26,   27,   28,
    QMetaType::Void, QMetaType::Bool, QMetaType::UShort, QMetaType::UChar, QMetaType::UInt, QMetaType::UInt,   30,   31,   32,   33,   34,
    QMetaType::Void, QMetaType::Bool, QMetaType::UShort, QMetaType::UChar, QMetaType::UInt,   30,   31,   32,   34,
    QMetaType::Void, QMetaType::QString,    8,

       0        // eod
};

void CANTool::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CANTool *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onDeviceManage(); break;
        case 1: _t->onSendClicked(); break;
        case 2: _t->onClearReceive(); break;
        case 3: _t->onCanConnected(); break;
        case 4: _t->onCanDisconnected(); break;
        case 5: _t->onCanError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->onFrameReceived((*reinterpret_cast< const CanFrame(*)>(_a[1]))); break;
        case 7: _t->onFrameSent((*reinterpret_cast< const CanFrame(*)>(_a[1]))); break;
        case 8: _t->onNmtSend(); break;
        case 9: _t->onSdoRead(); break;
        case 10: _t->onSdoWrite(); break;
        case 11: _t->onSyncSend(); break;
        case 12: _t->onHeartbeat((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< NmtState(*)>(_a[2]))); break;
        case 13: _t->onEmcy((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< const QByteArray(*)>(_a[4]))); break;
        case 14: _t->onPdo((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])),(*reinterpret_cast< const QByteArray(*)>(_a[4]))); break;
        case 15: _t->onSdoReadFinished((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< quint32(*)>(_a[4])),(*reinterpret_cast< quint32(*)>(_a[5]))); break;
        case 16: _t->onSdoWriteFinished((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< quint32(*)>(_a[4]))); break;
        case 17: _t->onCanOpenLog((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CANTool::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_CANTool.data,
    qt_meta_data_CANTool,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CANTool::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CANTool::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CANTool.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int CANTool::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 18;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
