/****************************************************************************
** Meta object code from reading C++ file 'normalsenddialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../normalsenddialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'normalsenddialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_RowSender_t {
    QByteArrayData data[8];
    char stringdata0[69];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_RowSender_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_RowSender_t qt_meta_stringdata_RowSender = {
    {
QT_MOC_LITERAL(0, 0, 9), // "RowSender"
QT_MOC_LITERAL(1, 10, 13), // "statusChanged"
QT_MOC_LITERAL(2, 24, 0), // ""
QT_MOC_LITERAL(3, 25, 5), // "rowId"
QT_MOC_LITERAL(4, 31, 10), // "statusText"
QT_MOC_LITERAL(5, 42, 8), // "finished"
QT_MOC_LITERAL(6, 51, 7), // "success"
QT_MOC_LITERAL(7, 59, 9) // "onTimeout"

    },
    "RowSender\0statusChanged\0\0rowId\0"
    "statusText\0finished\0success\0onTimeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RowSender[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   29,    2, 0x06 /* Public */,
       5,    2,   34,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    0,   39,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Bool,    3,    6,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void RowSender::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<RowSender *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->statusChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->finished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 2: _t->onTimeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (RowSender::*)(int , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RowSender::statusChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (RowSender::*)(int , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RowSender::finished)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject RowSender::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_RowSender.data,
    qt_meta_data_RowSender,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *RowSender::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RowSender::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RowSender.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int RowSender::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void RowSender::statusChanged(int _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void RowSender::finished(int _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
struct qt_meta_stringdata_NormalSendPage_t {
    QByteArrayData data[23];
    char stringdata0[316];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NormalSendPage_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NormalSendPage_t qt_meta_stringdata_NormalSendPage = {
    {
QT_MOC_LITERAL(0, 0, 14), // "NormalSendPage"
QT_MOC_LITERAL(1, 15, 15), // "onImmediateSend"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 11), // "onAddToList"
QT_MOC_LITERAL(4, 44, 24), // "onImmediateSendTimerTick"
QT_MOC_LITERAL(5, 69, 11), // "onSelectAll"
QT_MOC_LITERAL(6, 81, 17), // "onInvertSelection"
QT_MOC_LITERAL(7, 99, 8), // "onMoveUp"
QT_MOC_LITERAL(8, 108, 10), // "onMoveDown"
QT_MOC_LITERAL(9, 119, 16), // "onDeleteSelected"
QT_MOC_LITERAL(10, 136, 11), // "onClearList"
QT_MOC_LITERAL(11, 148, 12), // "onImportList"
QT_MOC_LITERAL(12, 161, 12), // "onExportList"
QT_MOC_LITERAL(13, 174, 15), // "onListSendStart"
QT_MOC_LITERAL(14, 190, 14), // "onListSendStop"
QT_MOC_LITERAL(15, 205, 14), // "onListSendTick"
QT_MOC_LITERAL(16, 220, 19), // "onRowSenderFinished"
QT_MOC_LITERAL(17, 240, 5), // "rowId"
QT_MOC_LITERAL(18, 246, 7), // "success"
QT_MOC_LITERAL(19, 254, 17), // "onRowSenderStatus"
QT_MOC_LITERAL(20, 272, 10), // "statusText"
QT_MOC_LITERAL(21, 283, 16), // "onUIRefreshTimer"
QT_MOC_LITERAL(22, 300, 15) // "refreshChannels"

    },
    "NormalSendPage\0onImmediateSend\0\0"
    "onAddToList\0onImmediateSendTimerTick\0"
    "onSelectAll\0onInvertSelection\0onMoveUp\0"
    "onMoveDown\0onDeleteSelected\0onClearList\0"
    "onImportList\0onExportList\0onListSendStart\0"
    "onListSendStop\0onListSendTick\0"
    "onRowSenderFinished\0rowId\0success\0"
    "onRowSenderStatus\0statusText\0"
    "onUIRefreshTimer\0refreshChannels"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NormalSendPage[] = {

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
       7,    0,  109,    2, 0x08 /* Private */,
       8,    0,  110,    2, 0x08 /* Private */,
       9,    0,  111,    2, 0x08 /* Private */,
      10,    0,  112,    2, 0x08 /* Private */,
      11,    0,  113,    2, 0x08 /* Private */,
      12,    0,  114,    2, 0x08 /* Private */,
      13,    0,  115,    2, 0x08 /* Private */,
      14,    0,  116,    2, 0x08 /* Private */,
      15,    0,  117,    2, 0x08 /* Private */,
      16,    2,  118,    2, 0x08 /* Private */,
      19,    2,  123,    2, 0x08 /* Private */,
      21,    0,  128,    2, 0x08 /* Private */,
      22,    0,  129,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Bool,   17,   18,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,   17,   20,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void NormalSendPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NormalSendPage *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onImmediateSend(); break;
        case 1: _t->onAddToList(); break;
        case 2: _t->onImmediateSendTimerTick(); break;
        case 3: _t->onSelectAll(); break;
        case 4: _t->onInvertSelection(); break;
        case 5: _t->onMoveUp(); break;
        case 6: _t->onMoveDown(); break;
        case 7: _t->onDeleteSelected(); break;
        case 8: _t->onClearList(); break;
        case 9: _t->onImportList(); break;
        case 10: _t->onExportList(); break;
        case 11: _t->onListSendStart(); break;
        case 12: _t->onListSendStop(); break;
        case 13: _t->onListSendTick(); break;
        case 14: _t->onRowSenderFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 15: _t->onRowSenderStatus((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 16: _t->onUIRefreshTimer(); break;
        case 17: _t->refreshChannels(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject NormalSendPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_NormalSendPage.data,
    qt_meta_data_NormalSendPage,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *NormalSendPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NormalSendPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NormalSendPage.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int NormalSendPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
struct qt_meta_stringdata_NormalSendDialog_t {
    QByteArrayData data[5];
    char stringdata0[48];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NormalSendDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NormalSendDialog_t qt_meta_stringdata_NormalSendDialog = {
    {
QT_MOC_LITERAL(0, 0, 16), // "NormalSendDialog"
QT_MOC_LITERAL(1, 17, 12), // "onTabChanged"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 5), // "index"
QT_MOC_LITERAL(4, 37, 10) // "onCloseTab"

    },
    "NormalSendDialog\0onTabChanged\0\0index\0"
    "onCloseTab"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NormalSendDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   24,    2, 0x08 /* Private */,
       4,    1,   27,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,

       0        // eod
};

void NormalSendDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NormalSendDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onTabChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->onCloseTab((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject NormalSendDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_NormalSendDialog.data,
    qt_meta_data_NormalSendDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *NormalSendDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NormalSendDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NormalSendDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int NormalSendDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
