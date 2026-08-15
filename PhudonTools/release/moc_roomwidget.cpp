/****************************************************************************
** Meta object code from reading C++ file 'roomwidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../roomwidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'roomwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_RoomWidget_t {
    QByteArrayData data[14];
    char stringdata0[162];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_RoomWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_RoomWidget_t qt_meta_stringdata_RoomWidget = {
    {
QT_MOC_LITERAL(0, 0, 10), // "RoomWidget"
QT_MOC_LITERAL(1, 11, 12), // "roomDragging"
QT_MOC_LITERAL(2, 24, 0), // ""
QT_MOC_LITERAL(3, 25, 2), // "id"
QT_MOC_LITERAL(4, 28, 11), // "currentGeom"
QT_MOC_LITERAL(5, 40, 16), // "roomDragFinished"
QT_MOC_LITERAL(6, 57, 9), // "roomMoved"
QT_MOC_LITERAL(7, 67, 7), // "newGeom"
QT_MOC_LITERAL(8, 75, 11), // "roomResized"
QT_MOC_LITERAL(9, 87, 11), // "roomClicked"
QT_MOC_LITERAL(10, 99, 19), // "roomRenameRequested"
QT_MOC_LITERAL(11, 119, 19), // "roomDeleteRequested"
QT_MOC_LITERAL(12, 139, 15), // "roomLockToggled"
QT_MOC_LITERAL(13, 155, 6) // "locked"

    },
    "RoomWidget\0roomDragging\0\0id\0currentGeom\0"
    "roomDragFinished\0roomMoved\0newGeom\0"
    "roomResized\0roomClicked\0roomRenameRequested\0"
    "roomDeleteRequested\0roomLockToggled\0"
    "locked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RoomWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   54,    2, 0x06 /* Public */,
       5,    1,   59,    2, 0x06 /* Public */,
       6,    2,   62,    2, 0x06 /* Public */,
       8,    2,   67,    2, 0x06 /* Public */,
       9,    1,   72,    2, 0x06 /* Public */,
      10,    1,   75,    2, 0x06 /* Public */,
      11,    1,   78,    2, 0x06 /* Public */,
      12,    2,   81,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QRect,    3,    4,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::QRect,    3,    7,
    QMetaType::Void, QMetaType::QString, QMetaType::QRect,    3,    7,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,    3,   13,

       0        // eod
};

void RoomWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<RoomWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->roomDragging((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QRect(*)>(_a[2]))); break;
        case 1: _t->roomDragFinished((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->roomMoved((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QRect(*)>(_a[2]))); break;
        case 3: _t->roomResized((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QRect(*)>(_a[2]))); break;
        case 4: _t->roomClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->roomRenameRequested((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->roomDeleteRequested((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->roomLockToggled((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (RoomWidget::*)(const QString & , const QRect & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RoomWidget::roomDragging)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (RoomWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RoomWidget::roomDragFinished)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (RoomWidget::*)(const QString & , const QRect & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RoomWidget::roomMoved)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (RoomWidget::*)(const QString & , const QRect & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RoomWidget::roomResized)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (RoomWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RoomWidget::roomClicked)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (RoomWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RoomWidget::roomRenameRequested)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (RoomWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RoomWidget::roomDeleteRequested)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (RoomWidget::*)(const QString & , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RoomWidget::roomLockToggled)) {
                *result = 7;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject RoomWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_RoomWidget.data,
    qt_meta_data_RoomWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *RoomWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RoomWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RoomWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int RoomWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void RoomWidget::roomDragging(const QString & _t1, const QRect & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void RoomWidget::roomDragFinished(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void RoomWidget::roomMoved(const QString & _t1, const QRect & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void RoomWidget::roomResized(const QString & _t1, const QRect & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void RoomWidget::roomClicked(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void RoomWidget::roomRenameRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void RoomWidget::roomDeleteRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void RoomWidget::roomLockToggled(const QString & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
