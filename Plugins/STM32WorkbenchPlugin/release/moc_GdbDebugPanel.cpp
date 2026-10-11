/****************************************************************************
** Meta object code from reading C++ file 'GdbDebugPanel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../UI/GdbDebugPanel.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'GdbDebugPanel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN13GdbDebugPanelE_t {};
} // unnamed namespace

template <> constexpr inline auto GdbDebugPanel::qt_create_metaobjectdata<qt_meta_tag_ZN13GdbDebugPanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "GdbDebugPanel",
        "jumpToSourceLocation",
        "",
        "file",
        "line",
        "onTargetRunning",
        "onTargetStopped",
        "func",
        "reason",
        "onStackTraceReceived",
        "QList<GdbStackFrame>",
        "frames",
        "onLocalsReceived",
        "QList<GdbVariable>",
        "variables",
        "onRegistersReceived",
        "QList<GdbRegister>",
        "registers",
        "onBreakpointChanged",
        "GdbBreakpoint",
        "bkpt",
        "added",
        "onConsoleOutput",
        "text",
        "onEvaluateExpression",
        "onSendConsoleCommand",
        "onStackItemDoubleClicked",
        "QTableWidgetItem*",
        "item",
        "onBreakpointItemDoubleClicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'jumpToSourceLocation'
        QtMocHelpers::SignalData<void(const QString &, int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::Int, 4 },
        }}),
        // Slot 'onTargetRunning'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTargetStopped'
        QtMocHelpers::SlotData<void(const QString &, int, const QString &, const QString &)>(6, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::Int, 4 }, { QMetaType::QString, 7 }, { QMetaType::QString, 8 },
        }}),
        // Slot 'onStackTraceReceived'
        QtMocHelpers::SlotData<void(const QVector<GdbStackFrame> &)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Slot 'onLocalsReceived'
        QtMocHelpers::SlotData<void(const QVector<GdbVariable> &)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 13, 14 },
        }}),
        // Slot 'onRegistersReceived'
        QtMocHelpers::SlotData<void(const QVector<GdbRegister> &)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 16, 17 },
        }}),
        // Slot 'onBreakpointChanged'
        QtMocHelpers::SlotData<void(const GdbBreakpoint &, bool)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 19, 20 }, { QMetaType::Bool, 21 },
        }}),
        // Slot 'onConsoleOutput'
        QtMocHelpers::SlotData<void(const QString &)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 23 },
        }}),
        // Slot 'onEvaluateExpression'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSendConsoleCommand'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStackItemDoubleClicked'
        QtMocHelpers::SlotData<void(QTableWidgetItem *)>(26, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 27, 28 },
        }}),
        // Slot 'onBreakpointItemDoubleClicked'
        QtMocHelpers::SlotData<void(QTableWidgetItem *)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 27, 28 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<GdbDebugPanel, qt_meta_tag_ZN13GdbDebugPanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject GdbDebugPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13GdbDebugPanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13GdbDebugPanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13GdbDebugPanelE_t>.metaTypes,
    nullptr
} };

void GdbDebugPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GdbDebugPanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->jumpToSourceLocation((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 1: _t->onTargetRunning(); break;
        case 2: _t->onTargetStopped((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[4]))); break;
        case 3: _t->onStackTraceReceived((*reinterpret_cast<std::add_pointer_t<QList<GdbStackFrame>>>(_a[1]))); break;
        case 4: _t->onLocalsReceived((*reinterpret_cast<std::add_pointer_t<QList<GdbVariable>>>(_a[1]))); break;
        case 5: _t->onRegistersReceived((*reinterpret_cast<std::add_pointer_t<QList<GdbRegister>>>(_a[1]))); break;
        case 6: _t->onBreakpointChanged((*reinterpret_cast<std::add_pointer_t<GdbBreakpoint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 7: _t->onConsoleOutput((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->onEvaluateExpression(); break;
        case 9: _t->onSendConsoleCommand(); break;
        case 10: _t->onStackItemDoubleClicked((*reinterpret_cast<std::add_pointer_t<QTableWidgetItem*>>(_a[1]))); break;
        case 11: _t->onBreakpointItemDoubleClicked((*reinterpret_cast<std::add_pointer_t<QTableWidgetItem*>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (GdbDebugPanel::*)(const QString & , int )>(_a, &GdbDebugPanel::jumpToSourceLocation, 0))
            return;
    }
}

const QMetaObject *GdbDebugPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GdbDebugPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13GdbDebugPanelE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int GdbDebugPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
void GdbDebugPanel::jumpToSourceLocation(const QString & _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}
QT_WARNING_POP
