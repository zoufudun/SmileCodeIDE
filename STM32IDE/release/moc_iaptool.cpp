/****************************************************************************
** Meta object code from reading C++ file 'iaptool.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../iaptool.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'iaptool.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_IAPTool_t {
    QByteArrayData data[13];
    char stringdata0[187];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_IAPTool_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_IAPTool_t qt_meta_stringdata_IAPTool = {
    {
QT_MOC_LITERAL(0, 0, 7), // "IAPTool"
QT_MOC_LITERAL(1, 8, 17), // "onProtocolChanged"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 5), // "index"
QT_MOC_LITERAL(4, 33, 12), // "onBrowseFile"
QT_MOC_LITERAL(5, 46, 14), // "onStartUpgrade"
QT_MOC_LITERAL(6, 61, 16), // "onConnectClicked"
QT_MOC_LITERAL(7, 78, 18), // "refreshSerialPorts"
QT_MOC_LITERAL(8, 97, 20), // "onSerialDataReceived"
QT_MOC_LITERAL(9, 118, 17), // "onUdpDataReceived"
QT_MOC_LITERAL(10, 136, 14), // "onTcpConnected"
QT_MOC_LITERAL(11, 151, 17), // "onTcpDisconnected"
QT_MOC_LITERAL(12, 169, 17) // "onTcpDataReceived"

    },
    "IAPTool\0onProtocolChanged\0\0index\0"
    "onBrowseFile\0onStartUpgrade\0"
    "onConnectClicked\0refreshSerialPorts\0"
    "onSerialDataReceived\0onUdpDataReceived\0"
    "onTcpConnected\0onTcpDisconnected\0"
    "onTcpDataReceived"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_IAPTool[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   64,    2, 0x08 /* Private */,
       4,    0,   67,    2, 0x08 /* Private */,
       5,    0,   68,    2, 0x08 /* Private */,
       6,    0,   69,    2, 0x08 /* Private */,
       7,    0,   70,    2, 0x08 /* Private */,
       8,    0,   71,    2, 0x08 /* Private */,
       9,    0,   72,    2, 0x08 /* Private */,
      10,    0,   73,    2, 0x08 /* Private */,
      11,    0,   74,    2, 0x08 /* Private */,
      12,    0,   75,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void IAPTool::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<IAPTool *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onProtocolChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->onBrowseFile(); break;
        case 2: _t->onStartUpgrade(); break;
        case 3: _t->onConnectClicked(); break;
        case 4: _t->refreshSerialPorts(); break;
        case 5: _t->onSerialDataReceived(); break;
        case 6: _t->onUdpDataReceived(); break;
        case 7: _t->onTcpConnected(); break;
        case 8: _t->onTcpDisconnected(); break;
        case 9: _t->onTcpDataReceived(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject IAPTool::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_IAPTool.data,
    qt_meta_data_IAPTool,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *IAPTool::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *IAPTool::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_IAPTool.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int IAPTool::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 10;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
