/****************************************************************************
** Meta object code from reading C++ file 'QCodeEditor.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../STM32IDE/QCodeEditor/include/internal/QCodeEditor.hpp"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QCodeEditor.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_QCodeEditor_t {
    uint offsetsAndSizes[20];
    char stringdata0[12];
    char stringdata1[17];
    char stringdata2[1];
    char stringdata3[2];
    char stringdata4[26];
    char stringdata5[21];
    char stringdata6[5];
    char stringdata7[21];
    char stringdata8[12];
    char stringdata9[19];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_QCodeEditor_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_QCodeEditor_t qt_meta_stringdata_QCodeEditor = {
    {
        QT_MOC_LITERAL(0, 11),  // "QCodeEditor"
        QT_MOC_LITERAL(12, 16),  // "insertCompletion"
        QT_MOC_LITERAL(29, 0),  // ""
        QT_MOC_LITERAL(30, 1),  // "s"
        QT_MOC_LITERAL(32, 25),  // "updateLineNumberAreaWidth"
        QT_MOC_LITERAL(58, 20),  // "updateLineNumberArea"
        QT_MOC_LITERAL(79, 4),  // "rect"
        QT_MOC_LITERAL(84, 20),  // "updateExtraSelection"
        QT_MOC_LITERAL(105, 11),  // "updateStyle"
        QT_MOC_LITERAL(117, 18)   // "onSelectionChanged"
    },
    "QCodeEditor",
    "insertCompletion",
    "",
    "s",
    "updateLineNumberAreaWidth",
    "updateLineNumberArea",
    "rect",
    "updateExtraSelection",
    "updateStyle",
    "onSelectionChanged"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_QCodeEditor[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x0a,    1 /* Public */,
       4,    1,   53,    2, 0x0a,    3 /* Public */,
       5,    1,   56,    2, 0x0a,    5 /* Public */,
       7,    0,   59,    2, 0x0a,    7 /* Public */,
       8,    0,   60,    2, 0x0a,    8 /* Public */,
       9,    0,   61,    2, 0x0a,    9 /* Public */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::QRect,    6,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject QCodeEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QTextEdit::staticMetaObject>(),
    qt_meta_stringdata_QCodeEditor.offsetsAndSizes,
    qt_meta_data_QCodeEditor,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_QCodeEditor_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<QCodeEditor, std::true_type>,
        // method 'insertCompletion'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'updateLineNumberAreaWidth'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'updateLineNumberArea'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QRect &, std::false_type>,
        // method 'updateExtraSelection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateStyle'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSelectionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void QCodeEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<QCodeEditor *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->insertCompletion((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->updateLineNumberAreaWidth((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->updateLineNumberArea((*reinterpret_cast< std::add_pointer_t<QRect>>(_a[1]))); break;
        case 3: _t->updateExtraSelection(); break;
        case 4: _t->updateStyle(); break;
        case 5: _t->onSelectionChanged(); break;
        default: ;
        }
    }
}

const QMetaObject *QCodeEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *QCodeEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_QCodeEditor.stringdata0))
        return static_cast<void*>(this);
    return QTextEdit::qt_metacast(_clname);
}

int QCodeEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTextEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
