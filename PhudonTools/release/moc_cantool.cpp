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
    QByteArrayData data[16];
    char stringdata0[244];
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
QT_MOC_LITERAL(3, 24, 22), // "onStatusMonitorClicked"
QT_MOC_LITERAL(4, 47, 18), // "onNewViewTriggered"
QT_MOC_LITERAL(5, 66, 8), // "QAction*"
QT_MOC_LITERAL(6, 75, 6), // "action"
QT_MOC_LITERAL(7, 82, 19), // "onSendDataTriggered"
QT_MOC_LITERAL(8, 102, 20), // "onChannelUtilization"
QT_MOC_LITERAL(9, 123, 27), // "onAdvancedFeaturesTriggered"
QT_MOC_LITERAL(10, 151, 16), // "onToolsTriggered"
QT_MOC_LITERAL(11, 168, 23), // "onSettingsHelpTriggered"
QT_MOC_LITERAL(12, 192, 14), // "onCanConnected"
QT_MOC_LITERAL(13, 207, 17), // "onCanDisconnected"
QT_MOC_LITERAL(14, 225, 10), // "onCanError"
QT_MOC_LITERAL(15, 236, 7) // "message"

    },
    "CANTool\0onDeviceManage\0\0onStatusMonitorClicked\0"
    "onNewViewTriggered\0QAction*\0action\0"
    "onSendDataTriggered\0onChannelUtilization\0"
    "onAdvancedFeaturesTriggered\0"
    "onToolsTriggered\0onSettingsHelpTriggered\0"
    "onCanConnected\0onCanDisconnected\0"
    "onCanError\0message"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CANTool[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   69,    2, 0x08 /* Private */,
       3,    0,   70,    2, 0x08 /* Private */,
       4,    1,   71,    2, 0x08 /* Private */,
       7,    1,   74,    2, 0x08 /* Private */,
       8,    0,   77,    2, 0x08 /* Private */,
       9,    1,   78,    2, 0x08 /* Private */,
      10,    1,   81,    2, 0x08 /* Private */,
      11,    1,   84,    2, 0x08 /* Private */,
      12,    0,   87,    2, 0x08 /* Private */,
      13,    0,   88,    2, 0x08 /* Private */,
      14,    1,   89,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   15,

       0        // eod
};

void CANTool::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CANTool *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onDeviceManage(); break;
        case 1: _t->onStatusMonitorClicked(); break;
        case 2: _t->onNewViewTriggered((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 3: _t->onSendDataTriggered((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 4: _t->onChannelUtilization(); break;
        case 5: _t->onAdvancedFeaturesTriggered((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 6: _t->onToolsTriggered((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 7: _t->onSettingsHelpTriggered((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        case 8: _t->onCanConnected(); break;
        case 9: _t->onCanDisconnected(); break;
        case 10: _t->onCanError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
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
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 11;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
