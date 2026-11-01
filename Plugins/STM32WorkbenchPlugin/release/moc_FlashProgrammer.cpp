/****************************************************************************
** Meta object code from reading C++ file 'FlashProgrammer.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../Core/FlashProgrammer.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FlashProgrammer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN15FlashProgrammerE_t {};
} // unnamed namespace

template <> constexpr inline auto FlashProgrammer::qt_create_metaobjectdata<qt_meta_tag_ZN15FlashProgrammerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "FlashProgrammer",
        "flashSessionStarted",
        "",
        "firmwarePath",
        "FlashOptions",
        "options",
        "flashStarted",
        "driverName",
        "flashOutput",
        "text",
        "isError",
        "flashProgress",
        "percentage",
        "statusText",
        "flashFinished",
        "success",
        "message",
        "onProcessReadyReadStd",
        "onProcessReadyReadErr",
        "onProcessFinished",
        "exitCode",
        "QProcess::ExitStatus",
        "exitStatus"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'flashSessionStarted'
        QtMocHelpers::SignalData<void(const QString &, const FlashOptions &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { 0x80000000 | 4, 5 },
        }}),
        // Signal 'flashStarted'
        QtMocHelpers::SignalData<void(const QString &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 },
        }}),
        // Signal 'flashOutput'
        QtMocHelpers::SignalData<void(const QString &, bool)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 9 }, { QMetaType::Bool, 10 },
        }}),
        // Signal 'flashProgress'
        QtMocHelpers::SignalData<void(int, const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 12 }, { QMetaType::QString, 13 },
        }}),
        // Signal 'flashFinished'
        QtMocHelpers::SignalData<void(bool, const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 15 }, { QMetaType::QString, 16 },
        }}),
        // Slot 'onProcessReadyReadStd'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onProcessReadyReadErr'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onProcessFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 20 }, { 0x80000000 | 21, 22 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<FlashProgrammer, qt_meta_tag_ZN15FlashProgrammerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject FlashProgrammer::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15FlashProgrammerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15FlashProgrammerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15FlashProgrammerE_t>.metaTypes,
    nullptr
} };

void FlashProgrammer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<FlashProgrammer *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->flashSessionStarted((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<FlashOptions>>(_a[2]))); break;
        case 1: _t->flashStarted((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->flashOutput((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 3: _t->flashProgress((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 4: _t->flashFinished((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 5: _t->onProcessReadyReadStd(); break;
        case 6: _t->onProcessReadyReadErr(); break;
        case 7: _t->onProcessFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (FlashProgrammer::*)(const QString & , const FlashOptions & )>(_a, &FlashProgrammer::flashSessionStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (FlashProgrammer::*)(const QString & )>(_a, &FlashProgrammer::flashStarted, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (FlashProgrammer::*)(const QString & , bool )>(_a, &FlashProgrammer::flashOutput, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (FlashProgrammer::*)(int , const QString & )>(_a, &FlashProgrammer::flashProgress, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (FlashProgrammer::*)(bool , const QString & )>(_a, &FlashProgrammer::flashFinished, 4))
            return;
    }
}

const QMetaObject *FlashProgrammer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FlashProgrammer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15FlashProgrammerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int FlashProgrammer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void FlashProgrammer::flashSessionStarted(const QString & _t1, const FlashOptions & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void FlashProgrammer::flashStarted(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void FlashProgrammer::flashOutput(const QString & _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void FlashProgrammer::flashProgress(int _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void FlashProgrammer::flashFinished(bool _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}
QT_WARNING_POP
