/****************************************************************************
** Meta object code from reading C++ file 'canopenmaster.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../canopenmaster.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'canopenmaster.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CanOpenMaster_t {
    QByteArrayData data[27];
    char stringdata0[271];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CanOpenMaster_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CanOpenMaster_t qt_meta_stringdata_CanOpenMaster = {
    {
QT_MOC_LITERAL(0, 0, 13), // "CanOpenMaster"
QT_MOC_LITERAL(1, 14, 17), // "heartbeatReceived"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 6), // "nodeId"
QT_MOC_LITERAL(4, 40, 8), // "NmtState"
QT_MOC_LITERAL(5, 49, 5), // "state"
QT_MOC_LITERAL(6, 55, 12), // "emcyReceived"
QT_MOC_LITERAL(7, 68, 9), // "errorCode"
QT_MOC_LITERAL(8, 78, 13), // "errorRegister"
QT_MOC_LITERAL(9, 92, 12), // "manufacturer"
QT_MOC_LITERAL(10, 105, 11), // "pdoReceived"
QT_MOC_LITERAL(11, 117, 9), // "pdoNumber"
QT_MOC_LITERAL(12, 127, 6), // "isTpdo"
QT_MOC_LITERAL(13, 134, 4), // "data"
QT_MOC_LITERAL(14, 139, 15), // "sdoReadFinished"
QT_MOC_LITERAL(15, 155, 7), // "success"
QT_MOC_LITERAL(16, 163, 5), // "index"
QT_MOC_LITERAL(17, 169, 8), // "subIndex"
QT_MOC_LITERAL(18, 178, 5), // "value"
QT_MOC_LITERAL(19, 184, 9), // "abortCode"
QT_MOC_LITERAL(20, 194, 16), // "sdoWriteFinished"
QT_MOC_LITERAL(21, 211, 10), // "logMessage"
QT_MOC_LITERAL(22, 222, 7), // "message"
QT_MOC_LITERAL(23, 230, 12), // "processFrame"
QT_MOC_LITERAL(24, 243, 8), // "CanFrame"
QT_MOC_LITERAL(25, 252, 5), // "frame"
QT_MOC_LITERAL(26, 258, 12) // "onSdoTimeout"

    },
    "CanOpenMaster\0heartbeatReceived\0\0"
    "nodeId\0NmtState\0state\0emcyReceived\0"
    "errorCode\0errorRegister\0manufacturer\0"
    "pdoReceived\0pdoNumber\0isTpdo\0data\0"
    "sdoReadFinished\0success\0index\0subIndex\0"
    "value\0abortCode\0sdoWriteFinished\0"
    "logMessage\0message\0processFrame\0"
    "CanFrame\0frame\0onSdoTimeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CanOpenMaster[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   54,    2, 0x06 /* Public */,
       6,    4,   59,    2, 0x06 /* Public */,
      10,    4,   68,    2, 0x06 /* Public */,
      14,    5,   77,    2, 0x06 /* Public */,
      20,    4,   88,    2, 0x06 /* Public */,
      21,    1,   97,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      23,    1,  100,    2, 0x0a /* Public */,
      26,    0,  103,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::UChar, 0x80000000 | 4,    3,    5,
    QMetaType::Void, QMetaType::UChar, QMetaType::UShort, QMetaType::UChar, QMetaType::QByteArray,    3,    7,    8,    9,
    QMetaType::Void, QMetaType::UChar, QMetaType::Int, QMetaType::Bool, QMetaType::QByteArray,    3,   11,   12,   13,
    QMetaType::Void, QMetaType::Bool, QMetaType::UShort, QMetaType::UChar, QMetaType::UInt, QMetaType::UInt,   15,   16,   17,   18,   19,
    QMetaType::Void, QMetaType::Bool, QMetaType::UShort, QMetaType::UChar, QMetaType::UInt,   15,   16,   17,   19,
    QMetaType::Void, QMetaType::QString,   22,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 24,   25,
    QMetaType::Void,

       0        // eod
};

void CanOpenMaster::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CanOpenMaster *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->heartbeatReceived((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< NmtState(*)>(_a[2]))); break;
        case 1: _t->emcyReceived((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< const QByteArray(*)>(_a[4]))); break;
        case 2: _t->pdoReceived((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3])),(*reinterpret_cast< const QByteArray(*)>(_a[4]))); break;
        case 3: _t->sdoReadFinished((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< quint32(*)>(_a[4])),(*reinterpret_cast< quint32(*)>(_a[5]))); break;
        case 4: _t->sdoWriteFinished((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< quint8(*)>(_a[3])),(*reinterpret_cast< quint32(*)>(_a[4]))); break;
        case 5: _t->logMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->processFrame((*reinterpret_cast< const CanFrame(*)>(_a[1]))); break;
        case 7: _t->onSdoTimeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CanOpenMaster::*)(quint8 , NmtState );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanOpenMaster::heartbeatReceived)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CanOpenMaster::*)(quint8 , quint16 , quint8 , const QByteArray & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanOpenMaster::emcyReceived)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CanOpenMaster::*)(quint8 , int , bool , const QByteArray & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanOpenMaster::pdoReceived)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CanOpenMaster::*)(bool , quint16 , quint8 , quint32 , quint32 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanOpenMaster::sdoReadFinished)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CanOpenMaster::*)(bool , quint16 , quint8 , quint32 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanOpenMaster::sdoWriteFinished)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CanOpenMaster::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CanOpenMaster::logMessage)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CanOpenMaster::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CanOpenMaster.data,
    qt_meta_data_CanOpenMaster,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CanOpenMaster::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CanOpenMaster::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CanOpenMaster.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CanOpenMaster::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void CanOpenMaster::heartbeatReceived(quint8 _t1, NmtState _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CanOpenMaster::emcyReceived(quint8 _t1, quint16 _t2, quint8 _t3, const QByteArray & _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CanOpenMaster::pdoReceived(quint8 _t1, int _t2, bool _t3, const QByteArray & _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CanOpenMaster::sdoReadFinished(bool _t1, quint16 _t2, quint8 _t3, quint32 _t4, quint32 _t5)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t5))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void CanOpenMaster::sdoWriteFinished(bool _t1, quint16 _t2, quint8 _t3, quint32 _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void CanOpenMaster::logMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
