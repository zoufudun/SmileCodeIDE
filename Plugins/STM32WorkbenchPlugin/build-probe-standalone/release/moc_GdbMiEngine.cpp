/****************************************************************************
** Meta object code from reading C++ file 'GdbMiEngine.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../Core/GdbMiEngine.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'GdbMiEngine.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.12.0. It"
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
struct qt_meta_tag_ZN11GdbMiEngineE_t {};
} // unnamed namespace

template <> constexpr inline auto GdbMiEngine::qt_create_metaobjectdata<qt_meta_tag_ZN11GdbMiEngineE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "GdbMiEngine",
        "stateChanged",
        "",
        "GdbTargetState",
        "state",
        "consoleOutput",
        "text",
        "targetRunning",
        "targetStopped",
        "file",
        "line",
        "func",
        "reason",
        "breakpointChanged",
        "GdbBreakpoint",
        "bkpt",
        "added",
        "stackTraceReceived",
        "QList<GdbStackFrame>",
        "frames",
        "localsReceived",
        "QList<GdbVariable>",
        "variables",
        "registersReceived",
        "QList<GdbRegister>",
        "registers",
        "expressionEvaluated",
        "expr",
        "result",
        "sessionFinished",
        "onProcessReadyRead",
        "onProcessFinished",
        "exitCode",
        "QProcess::ExitStatus",
        "exitStatus"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'stateChanged'
        QtMocHelpers::SignalData<void(GdbTargetState)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'consoleOutput'
        QtMocHelpers::SignalData<void(const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Signal 'targetRunning'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'targetStopped'
        QtMocHelpers::SignalData<void(const QString &, int, const QString &, const QString &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 9 }, { QMetaType::Int, 10 }, { QMetaType::QString, 11 }, { QMetaType::QString, 12 },
        }}),
        // Signal 'breakpointChanged'
        QtMocHelpers::SignalData<void(const GdbBreakpoint &, bool)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 14, 15 }, { QMetaType::Bool, 16 },
        }}),
        // Signal 'stackTraceReceived'
        QtMocHelpers::SignalData<void(const QVector<GdbStackFrame> &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 18, 19 },
        }}),
        // Signal 'localsReceived'
        QtMocHelpers::SignalData<void(const QVector<GdbVariable> &)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 21, 22 },
        }}),
        // Signal 'registersReceived'
        QtMocHelpers::SignalData<void(const QVector<GdbRegister> &)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 24, 25 },
        }}),
        // Signal 'expressionEvaluated'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 27 }, { QMetaType::QString, 28 },
        }}),
        // Signal 'sessionFinished'
        QtMocHelpers::SignalData<void()>(29, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onProcessReadyRead'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onProcessFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(31, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 32 }, { 0x80000000 | 33, 34 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<GdbMiEngine, qt_meta_tag_ZN11GdbMiEngineE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject GdbMiEngine::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11GdbMiEngineE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11GdbMiEngineE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN11GdbMiEngineE_t>.metaTypes,
    nullptr
} };

void GdbMiEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GdbMiEngine *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->stateChanged((*reinterpret_cast<std::add_pointer_t<GdbTargetState>>(_a[1]))); break;
        case 1: _t->consoleOutput((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->targetRunning(); break;
        case 3: _t->targetStopped((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[4]))); break;
        case 4: _t->breakpointChanged((*reinterpret_cast<std::add_pointer_t<GdbBreakpoint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 5: _t->stackTraceReceived((*reinterpret_cast<std::add_pointer_t<QList<GdbStackFrame>>>(_a[1]))); break;
        case 6: _t->localsReceived((*reinterpret_cast<std::add_pointer_t<QList<GdbVariable>>>(_a[1]))); break;
        case 7: _t->registersReceived((*reinterpret_cast<std::add_pointer_t<QList<GdbRegister>>>(_a[1]))); break;
        case 8: _t->expressionEvaluated((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 9: _t->sessionFinished(); break;
        case 10: _t->onProcessReadyRead(); break;
        case 11: _t->onProcessFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (GdbMiEngine::*)(GdbTargetState )>(_a, &GdbMiEngine::stateChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (GdbMiEngine::*)(const QString & )>(_a, &GdbMiEngine::consoleOutput, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (GdbMiEngine::*)()>(_a, &GdbMiEngine::targetRunning, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (GdbMiEngine::*)(const QString & , int , const QString & , const QString & )>(_a, &GdbMiEngine::targetStopped, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (GdbMiEngine::*)(const GdbBreakpoint & , bool )>(_a, &GdbMiEngine::breakpointChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (GdbMiEngine::*)(const QVector<GdbStackFrame> & )>(_a, &GdbMiEngine::stackTraceReceived, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (GdbMiEngine::*)(const QVector<GdbVariable> & )>(_a, &GdbMiEngine::localsReceived, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (GdbMiEngine::*)(const QVector<GdbRegister> & )>(_a, &GdbMiEngine::registersReceived, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (GdbMiEngine::*)(const QString & , const QString & )>(_a, &GdbMiEngine::expressionEvaluated, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (GdbMiEngine::*)()>(_a, &GdbMiEngine::sessionFinished, 9))
            return;
    }
}

const QMetaObject *GdbMiEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GdbMiEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN11GdbMiEngineE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int GdbMiEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void GdbMiEngine::stateChanged(GdbTargetState _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void GdbMiEngine::consoleOutput(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void GdbMiEngine::targetRunning()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void GdbMiEngine::targetStopped(const QString & _t1, int _t2, const QString & _t3, const QString & _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 4
void GdbMiEngine::breakpointChanged(const GdbBreakpoint & _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void GdbMiEngine::stackTraceReceived(const QVector<GdbStackFrame> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void GdbMiEngine::localsReceived(const QVector<GdbVariable> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void GdbMiEngine::registersReceived(const QVector<GdbRegister> & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void GdbMiEngine::expressionEvaluated(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1, _t2);
}

// SIGNAL 9
void GdbMiEngine::sessionFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}
QT_WARNING_POP
