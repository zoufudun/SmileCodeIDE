/****************************************************************************
** Meta object code from reading C++ file 'BuildManager.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Core/BuildManager.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BuildManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
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
struct qt_meta_tag_ZN12BuildManagerE_t {};
} // unnamed namespace

template <> constexpr inline auto BuildManager::qt_create_metaobjectdata<qt_meta_tag_ZN12BuildManagerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "BuildManager",
        "buildSessionStarted",
        "",
        "action",
        "buildLogChanged",
        "buildStarted",
        "buildOutput",
        "text",
        "isError",
        "diagnosticFound",
        "BuildDiagnostic",
        "diag",
        "memoryUsageUpdated",
        "flashUsed",
        "flashTotal",
        "ramUsed",
        "ramTotal",
        "buildFinished",
        "success",
        "exitCode",
        "durationMs",
        "legacyLinkerMigrationRequired",
        "linkerPath",
        "onProcessReadyReadStd",
        "onProcessReadyReadErr",
        "onProcessFinished",
        "QProcess::ExitStatus",
        "exitStatus"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'buildSessionStarted'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'buildLogChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'buildStarted'
        QtMocHelpers::SignalData<void(const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'buildOutput'
        QtMocHelpers::SignalData<void(const QString &, bool)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 }, { QMetaType::Bool, 8 },
        }}),
        // Signal 'diagnosticFound'
        QtMocHelpers::SignalData<void(const BuildDiagnostic &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Signal 'memoryUsageUpdated'
        QtMocHelpers::SignalData<void(quint32, quint32, quint32, quint32)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 13 }, { QMetaType::UInt, 14 }, { QMetaType::UInt, 15 }, { QMetaType::UInt, 16 },
        }}),
        // Signal 'buildFinished'
        QtMocHelpers::SignalData<void(bool, int, int)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 18 }, { QMetaType::Int, 19 }, { QMetaType::Int, 20 },
        }}),
        // Signal 'legacyLinkerMigrationRequired'
        QtMocHelpers::SignalData<void(const QString &)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 22 },
        }}),
        // Slot 'onProcessReadyReadStd'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onProcessReadyReadErr'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onProcessFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 19 }, { 0x80000000 | 26, 27 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<BuildManager, qt_meta_tag_ZN12BuildManagerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject BuildManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12BuildManagerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12BuildManagerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12BuildManagerE_t>.metaTypes,
    nullptr
} };

void BuildManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<BuildManager *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->buildSessionStarted((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->buildLogChanged(); break;
        case 2: _t->buildStarted((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->buildOutput((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 4: _t->diagnosticFound((*reinterpret_cast<std::add_pointer_t<BuildDiagnostic>>(_a[1]))); break;
        case 5: _t->memoryUsageUpdated((*reinterpret_cast<std::add_pointer_t<quint32>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<quint32>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<quint32>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<quint32>>(_a[4]))); break;
        case 6: _t->buildFinished((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 7: _t->legacyLinkerMigrationRequired((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->onProcessReadyReadStd(); break;
        case 9: _t->onProcessReadyReadErr(); break;
        case 10: _t->onProcessFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (BuildManager::*)(const QString & )>(_a, &BuildManager::buildSessionStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (BuildManager::*)()>(_a, &BuildManager::buildLogChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (BuildManager::*)(const QString & )>(_a, &BuildManager::buildStarted, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (BuildManager::*)(const QString & , bool )>(_a, &BuildManager::buildOutput, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (BuildManager::*)(const BuildDiagnostic & )>(_a, &BuildManager::diagnosticFound, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (BuildManager::*)(quint32 , quint32 , quint32 , quint32 )>(_a, &BuildManager::memoryUsageUpdated, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (BuildManager::*)(bool , int , int )>(_a, &BuildManager::buildFinished, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (BuildManager::*)(const QString & )>(_a, &BuildManager::legacyLinkerMigrationRequired, 7))
            return;
    }
}

const QMetaObject *BuildManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BuildManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12BuildManagerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int BuildManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void BuildManager::buildSessionStarted(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void BuildManager::buildLogChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void BuildManager::buildStarted(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void BuildManager::buildOutput(const QString & _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void BuildManager::diagnosticFound(const BuildDiagnostic & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void BuildManager::memoryUsageUpdated(quint32 _t1, quint32 _t2, quint32 _t3, quint32 _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 6
void BuildManager::buildFinished(bool _t1, int _t2, int _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2, _t3);
}

// SIGNAL 7
void BuildManager::legacyLinkerMigrationRequired(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}
QT_WARNING_POP
