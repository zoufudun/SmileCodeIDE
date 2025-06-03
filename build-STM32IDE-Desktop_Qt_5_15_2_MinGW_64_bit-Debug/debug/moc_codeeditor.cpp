/****************************************************************************
** Meta object code from reading C++ file 'codeeditor.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../STM32IDE/codeeditor.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'codeeditor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CodeEditor_t {
    QByteArrayData data[22];
    char stringdata0[312];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CodeEditor_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CodeEditor_t qt_meta_stringdata_CodeEditor = {
    {
QT_MOC_LITERAL(0, 0, 10), // "CodeEditor"
QT_MOC_LITERAL(1, 11, 16), // "newFileRequested"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 17), // "openFileRequested"
QT_MOC_LITERAL(4, 47, 17), // "saveFileRequested"
QT_MOC_LITERAL(5, 65, 14), // "buildRequested"
QT_MOC_LITERAL(6, 80, 14), // "cleanRequested"
QT_MOC_LITERAL(7, 95, 14), // "debugRequested"
QT_MOC_LITERAL(8, 110, 12), // "runRequested"
QT_MOC_LITERAL(9, 123, 13), // "stopRequested"
QT_MOC_LITERAL(10, 137, 22), // "serialMonitorRequested"
QT_MOC_LITERAL(11, 160, 17), // "settingsRequested"
QT_MOC_LITERAL(12, 178, 13), // "helpRequested"
QT_MOC_LITERAL(13, 192, 14), // "aboutRequested"
QT_MOC_LITERAL(14, 207, 18), // "updateVariableList"
QT_MOC_LITERAL(15, 226, 25), // "isValidFunctionDefinition"
QT_MOC_LITERAL(16, 252, 4), // "line"
QT_MOC_LITERAL(17, 257, 7), // "lineNum"
QT_MOC_LITERAL(18, 265, 8), // "allLines"
QT_MOC_LITERAL(19, 274, 15), // "onEditorChanged"
QT_MOC_LITERAL(20, 290, 14), // "QsciScintilla*"
QT_MOC_LITERAL(21, 305, 6) // "editor"

    },
    "CodeEditor\0newFileRequested\0\0"
    "openFileRequested\0saveFileRequested\0"
    "buildRequested\0cleanRequested\0"
    "debugRequested\0runRequested\0stopRequested\0"
    "serialMonitorRequested\0settingsRequested\0"
    "helpRequested\0aboutRequested\0"
    "updateVariableList\0isValidFunctionDefinition\0"
    "line\0lineNum\0allLines\0onEditorChanged\0"
    "QsciScintilla*\0editor"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CodeEditor[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   89,    2, 0x06 /* Public */,
       3,    0,   90,    2, 0x06 /* Public */,
       4,    0,   91,    2, 0x06 /* Public */,
       5,    0,   92,    2, 0x06 /* Public */,
       6,    0,   93,    2, 0x06 /* Public */,
       7,    0,   94,    2, 0x06 /* Public */,
       8,    0,   95,    2, 0x06 /* Public */,
       9,    0,   96,    2, 0x06 /* Public */,
      10,    0,   97,    2, 0x06 /* Public */,
      11,    0,   98,    2, 0x06 /* Public */,
      12,    0,   99,    2, 0x06 /* Public */,
      13,    0,  100,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      14,    0,  101,    2, 0x0a /* Public */,
      15,    3,  102,    2, 0x0a /* Public */,
      19,    1,  109,    2, 0x08 /* Private */,

 // signals: parameters
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

 // slots: parameters
    QMetaType::Void,
    QMetaType::Bool, QMetaType::QString, QMetaType::Int, QMetaType::QStringList,   16,   17,   18,
    QMetaType::Void, 0x80000000 | 20,   21,

       0        // eod
};

void CodeEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CodeEditor *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->newFileRequested(); break;
        case 1: _t->openFileRequested(); break;
        case 2: _t->saveFileRequested(); break;
        case 3: _t->buildRequested(); break;
        case 4: _t->cleanRequested(); break;
        case 5: _t->debugRequested(); break;
        case 6: _t->runRequested(); break;
        case 7: _t->stopRequested(); break;
        case 8: _t->serialMonitorRequested(); break;
        case 9: _t->settingsRequested(); break;
        case 10: _t->helpRequested(); break;
        case 11: _t->aboutRequested(); break;
        case 12: _t->updateVariableList(); break;
        case 13: { bool _r = _t->isValidFunctionDefinition((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QStringList(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 14: _t->onEditorChanged((*reinterpret_cast< QsciScintilla*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 14:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QsciScintilla* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::newFileRequested)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::openFileRequested)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::saveFileRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::buildRequested)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::cleanRequested)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::debugRequested)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::runRequested)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::stopRequested)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::serialMonitorRequested)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::settingsRequested)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::helpRequested)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::aboutRequested)) {
                *result = 11;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CodeEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CodeEditor.data,
    qt_meta_data_CodeEditor,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CodeEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CodeEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CodeEditor.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int CodeEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void CodeEditor::newFileRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void CodeEditor::openFileRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void CodeEditor::saveFileRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void CodeEditor::buildRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void CodeEditor::cleanRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void CodeEditor::debugRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void CodeEditor::runRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void CodeEditor::stopRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void CodeEditor::serialMonitorRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void CodeEditor::settingsRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void CodeEditor::helpRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void CodeEditor::aboutRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
