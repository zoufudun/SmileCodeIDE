/****************************************************************************
** Meta object code from reading C++ file 'serialportplot.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../serialportplot.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'serialportplot.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SerialSession_t {
    QByteArrayData data[16];
    char stringdata0[206];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SerialSession_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SerialSession_t qt_meta_stringdata_SerialSession = {
    {
QT_MOC_LITERAL(0, 0, 13), // "SerialSession"
QT_MOC_LITERAL(1, 14, 12), // "refreshPorts"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 10), // "checkPorts"
QT_MOC_LITERAL(4, 39, 13), // "openClosePort"
QT_MOC_LITERAL(5, 53, 11), // "onPortError"
QT_MOC_LITERAL(6, 65, 28), // "QSerialPort::SerialPortError"
QT_MOC_LITERAL(7, 94, 5), // "error"
QT_MOC_LITERAL(8, 100, 11), // "onReadyRead"
QT_MOC_LITERAL(9, 112, 8), // "sendData"
QT_MOC_LITERAL(10, 121, 16), // "clearReceiveArea"
QT_MOC_LITERAL(11, 138, 14), // "toggleAutoSend"
QT_MOC_LITERAL(12, 153, 7), // "checked"
QT_MOC_LITERAL(13, 161, 17), // "onAutoSendTimeout"
QT_MOC_LITERAL(14, 179, 15), // "onTxModeChanged"
QT_MOC_LITERAL(15, 195, 10) // "hexChecked"

    },
    "SerialSession\0refreshPorts\0\0checkPorts\0"
    "openClosePort\0onPortError\0"
    "QSerialPort::SerialPortError\0error\0"
    "onReadyRead\0sendData\0clearReceiveArea\0"
    "toggleAutoSend\0checked\0onAutoSendTimeout\0"
    "onTxModeChanged\0hexChecked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SerialSession[] = {

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
       1,    0,   64,    2, 0x08 /* Private */,
       3,    0,   65,    2, 0x08 /* Private */,
       4,    0,   66,    2, 0x08 /* Private */,
       5,    1,   67,    2, 0x08 /* Private */,
       8,    0,   70,    2, 0x08 /* Private */,
       9,    0,   71,    2, 0x08 /* Private */,
      10,    0,   72,    2, 0x08 /* Private */,
      11,    1,   73,    2, 0x08 /* Private */,
      13,    0,   76,    2, 0x08 /* Private */,
      14,    1,   77,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   12,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   15,

       0        // eod
};

void SerialSession::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SerialSession *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->refreshPorts(); break;
        case 1: _t->checkPorts(); break;
        case 2: _t->openClosePort(); break;
        case 3: _t->onPortError((*reinterpret_cast< QSerialPort::SerialPortError(*)>(_a[1]))); break;
        case 4: _t->onReadyRead(); break;
        case 5: _t->sendData(); break;
        case 6: _t->clearReceiveArea(); break;
        case 7: _t->toggleAutoSend((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->onAutoSendTimeout(); break;
        case 9: _t->onTxModeChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SerialSession::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_SerialSession.data,
    qt_meta_data_SerialSession,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SerialSession::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SerialSession::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SerialSession.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SerialSession::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
struct qt_meta_stringdata_SerialPortPlot_t {
    QByteArrayData data[19];
    char stringdata0[256];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SerialPortPlot_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SerialPortPlot_t qt_meta_stringdata_SerialPortPlot = {
    {
QT_MOC_LITERAL(0, 0, 14), // "SerialPortPlot"
QT_MOC_LITERAL(1, 15, 12), // "themeChanged"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 9), // "themeName"
QT_MOC_LITERAL(4, 39, 22), // "requestSplitHorizontal"
QT_MOC_LITERAL(5, 62, 15), // "SerialPortPlot*"
QT_MOC_LITERAL(6, 78, 4), // "plot"
QT_MOC_LITERAL(7, 83, 20), // "requestSplitVertical"
QT_MOC_LITERAL(8, 104, 17), // "requestCloseSplit"
QT_MOC_LITERAL(9, 122, 10), // "toggleDock"
QT_MOC_LITERAL(10, 133, 8), // "dockType"
QT_MOC_LITERAL(11, 142, 7), // "checked"
QT_MOC_LITERAL(12, 150, 13), // "addNewSession"
QT_MOC_LITERAL(13, 164, 18), // "onTabDoubleClicked"
QT_MOC_LITERAL(14, 183, 5), // "index"
QT_MOC_LITERAL(15, 189, 19), // "onTabCloseRequested"
QT_MOC_LITERAL(16, 209, 17), // "onSplitHorizontal"
QT_MOC_LITERAL(17, 227, 15), // "onSplitVertical"
QT_MOC_LITERAL(18, 243, 12) // "onCloseSplit"

    },
    "SerialPortPlot\0themeChanged\0\0themeName\0"
    "requestSplitHorizontal\0SerialPortPlot*\0"
    "plot\0requestSplitVertical\0requestCloseSplit\0"
    "toggleDock\0dockType\0checked\0addNewSession\0"
    "onTabDoubleClicked\0index\0onTabCloseRequested\0"
    "onSplitHorizontal\0onSplitVertical\0"
    "onCloseSplit"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SerialPortPlot[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   69,    2, 0x06 /* Public */,
       4,    1,   72,    2, 0x06 /* Public */,
       7,    1,   75,    2, 0x06 /* Public */,
       8,    1,   78,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    2,   81,    2, 0x0a /* Public */,
      12,    0,   86,    2, 0x08 /* Private */,
      13,    1,   87,    2, 0x08 /* Private */,
      15,    1,   90,    2, 0x08 /* Private */,
      16,    0,   93,    2, 0x08 /* Private */,
      17,    0,   94,    2, 0x08 /* Private */,
      18,    0,   95,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, 0x80000000 | 5,    6,

 // slots: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::Bool,   10,   11,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   14,
    QMetaType::Void, QMetaType::Int,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void SerialPortPlot::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SerialPortPlot *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->themeChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->requestSplitHorizontal((*reinterpret_cast< SerialPortPlot*(*)>(_a[1]))); break;
        case 2: _t->requestSplitVertical((*reinterpret_cast< SerialPortPlot*(*)>(_a[1]))); break;
        case 3: _t->requestCloseSplit((*reinterpret_cast< SerialPortPlot*(*)>(_a[1]))); break;
        case 4: _t->toggleDock((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 5: _t->addNewSession(); break;
        case 6: _t->onTabDoubleClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->onTabCloseRequested((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->onSplitHorizontal(); break;
        case 9: _t->onSplitVertical(); break;
        case 10: _t->onCloseSplit(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< SerialPortPlot* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< SerialPortPlot* >(); break;
            }
            break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< SerialPortPlot* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SerialPortPlot::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialPortPlot::themeChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SerialPortPlot::*)(SerialPortPlot * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialPortPlot::requestSplitHorizontal)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SerialPortPlot::*)(SerialPortPlot * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialPortPlot::requestSplitVertical)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SerialPortPlot::*)(SerialPortPlot * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SerialPortPlot::requestCloseSplit)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SerialPortPlot::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_SerialPortPlot.data,
    qt_meta_data_SerialPortPlot,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SerialPortPlot::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SerialPortPlot::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SerialPortPlot.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SerialPortPlot::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void SerialPortPlot::themeChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SerialPortPlot::requestSplitHorizontal(SerialPortPlot * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SerialPortPlot::requestSplitVertical(SerialPortPlot * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SerialPortPlot::requestCloseSplit(SerialPortPlot * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
struct qt_meta_stringdata_SerialPortContainer_t {
    QByteArrayData data[9];
    char stringdata0[130];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SerialPortContainer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SerialPortContainer_t qt_meta_stringdata_SerialPortContainer = {
    {
QT_MOC_LITERAL(0, 0, 19), // "SerialPortContainer"
QT_MOC_LITERAL(1, 20, 21), // "handleSplitHorizontal"
QT_MOC_LITERAL(2, 42, 0), // ""
QT_MOC_LITERAL(3, 43, 19), // "handleSplitVertical"
QT_MOC_LITERAL(4, 63, 16), // "handleCloseSplit"
QT_MOC_LITERAL(5, 80, 15), // "SerialPortPlot*"
QT_MOC_LITERAL(6, 96, 4), // "plot"
QT_MOC_LITERAL(7, 101, 18), // "handleThemeChanged"
QT_MOC_LITERAL(8, 120, 9) // "themeName"

    },
    "SerialPortContainer\0handleSplitHorizontal\0"
    "\0handleSplitVertical\0handleCloseSplit\0"
    "SerialPortPlot*\0plot\0handleThemeChanged\0"
    "themeName"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SerialPortContainer[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   34,    2, 0x08 /* Private */,
       3,    0,   35,    2, 0x08 /* Private */,
       4,    1,   36,    2, 0x08 /* Private */,
       7,    1,   39,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, QMetaType::QString,    8,

       0        // eod
};

void SerialPortContainer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SerialPortContainer *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->handleSplitHorizontal(); break;
        case 1: _t->handleSplitVertical(); break;
        case 2: _t->handleCloseSplit((*reinterpret_cast< SerialPortPlot*(*)>(_a[1]))); break;
        case 3: _t->handleThemeChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< SerialPortPlot* >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SerialPortContainer::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_SerialPortContainer.data,
    qt_meta_data_SerialPortContainer,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SerialPortContainer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SerialPortContainer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SerialPortContainer.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SerialPortContainer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
