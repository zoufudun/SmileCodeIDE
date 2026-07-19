/****************************************************************************
** Meta object code from reading C++ file 'devicemonitorpanel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../devicemonitorpanel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'devicemonitorpanel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DeviceMonitorPanel_t {
    QByteArrayData data[13];
    char stringdata0[166];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_DeviceMonitorPanel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_DeviceMonitorPanel_t qt_meta_stringdata_DeviceMonitorPanel = {
    {
QT_MOC_LITERAL(0, 0, 18), // "DeviceMonitorPanel"
QT_MOC_LITERAL(1, 19, 10), // "logMessage"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 3), // "msg"
QT_MOC_LITERAL(4, 35, 15), // "onFrameReceived"
QT_MOC_LITERAL(5, 51, 8), // "CanFrame"
QT_MOC_LITERAL(6, 60, 5), // "frame"
QT_MOC_LITERAL(7, 66, 15), // "onConfigClicked"
QT_MOC_LITERAL(8, 82, 15), // "onImportClicked"
QT_MOC_LITERAL(9, 98, 15), // "onExportClicked"
QT_MOC_LITERAL(10, 114, 14), // "onResetClicked"
QT_MOC_LITERAL(11, 129, 15), // "onNewConnection"
QT_MOC_LITERAL(12, 145, 20) // "onClientDisconnected"

    },
    "DeviceMonitorPanel\0logMessage\0\0msg\0"
    "onFrameReceived\0CanFrame\0frame\0"
    "onConfigClicked\0onImportClicked\0"
    "onExportClicked\0onResetClicked\0"
    "onNewConnection\0onClientDisconnected"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DeviceMonitorPanel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    1,   57,    2, 0x0a /* Public */,
       7,    0,   60,    2, 0x08 /* Private */,
       8,    0,   61,    2, 0x08 /* Private */,
       9,    0,   62,    2, 0x08 /* Private */,
      10,    0,   63,    2, 0x08 /* Private */,
      11,    0,   64,    2, 0x08 /* Private */,
      12,    0,   65,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void DeviceMonitorPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DeviceMonitorPanel *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->logMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->onFrameReceived((*reinterpret_cast< const CanFrame(*)>(_a[1]))); break;
        case 2: _t->onConfigClicked(); break;
        case 3: _t->onImportClicked(); break;
        case 4: _t->onExportClicked(); break;
        case 5: _t->onResetClicked(); break;
        case 6: _t->onNewConnection(); break;
        case 7: _t->onClientDisconnected(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DeviceMonitorPanel::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DeviceMonitorPanel::logMessage)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject DeviceMonitorPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_DeviceMonitorPanel.data,
    qt_meta_data_DeviceMonitorPanel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *DeviceMonitorPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DeviceMonitorPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DeviceMonitorPanel.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int DeviceMonitorPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void DeviceMonitorPanel::logMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
