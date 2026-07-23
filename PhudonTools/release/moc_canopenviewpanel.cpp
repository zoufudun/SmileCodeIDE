/****************************************************************************
** Meta object code from reading C++ file 'canopenviewpanel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../canopenviewpanel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'canopenviewpanel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CanOpenViewPanel_t {
    QByteArrayData data[30];
    char stringdata0[302];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CanOpenViewPanel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CanOpenViewPanel_t qt_meta_stringdata_CanOpenViewPanel = {
    {
QT_MOC_LITERAL(0, 0, 16), // "CanOpenViewPanel"
QT_MOC_LITERAL(1, 17, 14), // "closeRequested"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 17), // "CanOpenViewPanel*"
QT_MOC_LITERAL(4, 51, 5), // "panel"
QT_MOC_LITERAL(5, 57, 9), // "onNmtSend"
QT_MOC_LITERAL(6, 67, 9), // "onSdoRead"
QT_MOC_LITERAL(7, 77, 10), // "onSdoWrite"
QT_MOC_LITERAL(8, 88, 10), // "onSyncSend"
QT_MOC_LITERAL(9, 99, 11), // "onHeartbeat"
QT_MOC_LITERAL(10, 111, 6), // "nodeId"
QT_MOC_LITERAL(11, 118, 8), // "NmtState"
QT_MOC_LITERAL(12, 127, 5), // "state"
QT_MOC_LITERAL(13, 133, 6), // "onEmcy"
QT_MOC_LITERAL(14, 140, 9), // "errorCode"
QT_MOC_LITERAL(15, 150, 13), // "errorRegister"
QT_MOC_LITERAL(16, 164, 12), // "manufacturer"
QT_MOC_LITERAL(17, 177, 5), // "onPdo"
QT_MOC_LITERAL(18, 183, 9), // "pdoNumber"
QT_MOC_LITERAL(19, 193, 6), // "isTpdo"
QT_MOC_LITERAL(20, 200, 4), // "data"
QT_MOC_LITERAL(21, 205, 17), // "onSdoReadFinished"
QT_MOC_LITERAL(22, 223, 7), // "success"
QT_MOC_LITERAL(23, 231, 5), // "index"
QT_MOC_LITERAL(24, 237, 8), // "subIndex"
QT_MOC_LITERAL(25, 246, 5), // "value"
QT_MOC_LITERAL(26, 252, 9), // "abortCode"
QT_MOC_LITERAL(27, 262, 18), // "onSdoWriteFinished"
QT_MOC_LITERAL(28, 281, 12), // "onCanOpenLog"
QT_MOC_LITERAL(29, 294, 7) // "message"

    },
    "CanOpenViewPanel\0closeRequested\0\0"
    "CanOpenViewPanel*\0panel\0onNmtSend\0"
    "onSdoRead\0onSdoWrite\0onSyncSend\0"
    "onHeartbeat\0nodeId\0NmtState\0state\0"
    "onEmcy\0errorCode\0errorRegister\0"
    "manufacturer\0onPdo\0pdoNumber\0isTpdo\0"
    "data\0onSdoReadFinished\0success\0index\0"
    "subIndex\0value\0abortCode\0onSdoWriteFinished\0"
    "onCanOpenLog\0message"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CanOpenViewPanel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   69,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       5,    0,   72,    2, 0x08 /* Private */,
       6,    0,   73,    2, 0x08 /* Private */,
       7,    0,   74,    2, 0x08 /* Private */,
       8,    0,   75,    2, 0x08 /* Private */,
       9,    2,   76,    2, 0x08 /* Private */,
      13,    4,   81,    2, 0x08 /* Private */,
      17,    4,   90,    2, 0x08 /* Private */,
      21,    5,   99,    2, 0x08 /* Private */,
      27,    4,  110,    2, 0x08 /* Private */,
      28,    1,  119,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::UChar, 0x80000000 | 11,   10,   12,
    QMetaType::Void, QMetaType::UChar, QMetaType::UShort, QMetaType::UChar, QMetaType::QByteArray,   10,   14,   15,   16,
    QMetaType::Void, QMetaType::UChar, QMetaType::Int, QMetaType::Bool, QMetaType::QByteArray,   10,   18,   19,   20,
    QMetaType::Void, QMetaType::Bool, QMetaType::UShort, QMetaType::UChar, QMetaType::UInt, QMetaType::UInt,   22,   23,   24,   25,   26,
    QMetaType::Void, QMetaType::Bool, QMetaType::UShort, QMetaType::UChar, QMetaType::UInt,   22,   23,   24,   26,
    QMetaType::Void, QMetaType::QString,   29,

       0        // eod
};

void CanOpenViewPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CanOpenViewPanel *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->closeRequested((*reinterpret_cast< CanOpenViewPanel*(*)>(_a[1]))); break;
        case 1: _t->onNmtSend(); break;
        case 2: _t->onSdoRead(); break;
        case 3: _t->onSdoWrite(); break;
        case 4: _t->onSyncSend(); break;
        case 5: _t->onHeartbeat((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< NmtState(*)>(_a[2]))); break;
        case 6: _t->onEmcy((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< const QByteArray(*)>(_a[4]))); break;
        case 7: _t->onPdo((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])),(*reinterpret_cast< const QByteArray(*)>(_a[4]))); break;
        case 8: _t->onSdoReadFinished((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< quint32(*)>(_a[4])),(*reinterpret_cast< quint32(*)>(_a[5]))); break;
        case 9: _t->onSdoWriteFinished((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< quint32(*)>(_a[4]))); break;
        case 10: _t->onCanOpenLog((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< CanOpenViewPanel* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CanOpenViewPanel::*)(CanOpenViewPanel * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanOpenViewPanel::closeRequested)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CanOpenViewPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CanOpenViewPanel.data,
    qt_meta_data_CanOpenViewPanel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CanOpenViewPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CanOpenViewPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CanOpenViewPanel.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int CanOpenViewPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void CanOpenViewPanel::closeRequested(CanOpenViewPanel * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
