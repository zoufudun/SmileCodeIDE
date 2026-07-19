/****************************************************************************
** Meta object code from reading C++ file 'candevicedialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../candevicedialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'candevicedialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CanDeviceDialog_t {
    QByteArrayData data[12];
    char stringdata0[215];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CanDeviceDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CanDeviceDialog_t qt_meta_stringdata_CanDeviceDialog = {
    {
QT_MOC_LITERAL(0, 0, 15), // "CanDeviceDialog"
QT_MOC_LITERAL(1, 16, 19), // "onOpenDeviceClicked"
QT_MOC_LITERAL(2, 36, 0), // ""
QT_MOC_LITERAL(3, 37, 20), // "onCloseDeviceClicked"
QT_MOC_LITERAL(4, 58, 21), // "onStartChannelClicked"
QT_MOC_LITERAL(5, 80, 7), // "channel"
QT_MOC_LITERAL(6, 88, 20), // "onStopChannelClicked"
QT_MOC_LITERAL(7, 109, 23), // "onFilterSettingsClicked"
QT_MOC_LITERAL(8, 133, 18), // "onStartAllChannels"
QT_MOC_LITERAL(9, 152, 17), // "onStopAllChannels"
QT_MOC_LITERAL(10, 170, 23), // "onShowDeviceInfoClicked"
QT_MOC_LITERAL(11, 194, 20) // "onCloudDeviceClicked"

    },
    "CanDeviceDialog\0onOpenDeviceClicked\0"
    "\0onCloseDeviceClicked\0onStartChannelClicked\0"
    "channel\0onStopChannelClicked\0"
    "onFilterSettingsClicked\0onStartAllChannels\0"
    "onStopAllChannels\0onShowDeviceInfoClicked\0"
    "onCloudDeviceClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CanDeviceDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   59,    2, 0x08 /* Private */,
       3,    0,   60,    2, 0x08 /* Private */,
       4,    1,   61,    2, 0x08 /* Private */,
       6,    1,   64,    2, 0x08 /* Private */,
       7,    1,   67,    2, 0x08 /* Private */,
       8,    0,   70,    2, 0x08 /* Private */,
       9,    0,   71,    2, 0x08 /* Private */,
      10,    0,   72,    2, 0x08 /* Private */,
      11,    0,   73,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void CanDeviceDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CanDeviceDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onOpenDeviceClicked(); break;
        case 1: _t->onCloseDeviceClicked(); break;
        case 2: _t->onStartChannelClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->onStopChannelClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->onFilterSettingsClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->onStartAllChannels(); break;
        case 6: _t->onStopAllChannels(); break;
        case 7: _t->onShowDeviceInfoClicked(); break;
        case 8: _t->onCloudDeviceClicked(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CanDeviceDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_CanDeviceDialog.data,
    qt_meta_data_CanDeviceDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CanDeviceDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CanDeviceDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CanDeviceDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int CanDeviceDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
