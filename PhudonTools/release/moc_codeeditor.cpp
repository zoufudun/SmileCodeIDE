/****************************************************************************
** Meta object code from reading C++ file 'codeeditor.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../codeeditor.h"
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
    QByteArrayData data[31];
    char stringdata0[460];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CodeEditor_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CodeEditor_t qt_meta_stringdata_CodeEditor = {
    {
QT_MOC_LITERAL(0, 0, 10), // "CodeEditor"
QT_MOC_LITERAL(1, 11, 10), // "fileOpened"
QT_MOC_LITERAL(2, 22, 0), // ""
QT_MOC_LITERAL(3, 23, 8), // "filePath"
QT_MOC_LITERAL(4, 32, 9), // "fileSaved"
QT_MOC_LITERAL(5, 42, 16), // "newFileRequested"
QT_MOC_LITERAL(6, 59, 17), // "openFileRequested"
QT_MOC_LITERAL(7, 77, 17), // "saveFileRequested"
QT_MOC_LITERAL(8, 95, 14), // "buildRequested"
QT_MOC_LITERAL(9, 110, 14), // "cleanRequested"
QT_MOC_LITERAL(10, 125, 14), // "debugRequested"
QT_MOC_LITERAL(11, 140, 12), // "runRequested"
QT_MOC_LITERAL(12, 153, 13), // "stopRequested"
QT_MOC_LITERAL(13, 167, 22), // "serialMonitorRequested"
QT_MOC_LITERAL(14, 190, 17), // "settingsRequested"
QT_MOC_LITERAL(15, 208, 13), // "helpRequested"
QT_MOC_LITERAL(16, 222, 14), // "aboutRequested"
QT_MOC_LITERAL(17, 237, 18), // "updateVariableList"
QT_MOC_LITERAL(18, 256, 25), // "isValidFunctionDefinition"
QT_MOC_LITERAL(19, 282, 4), // "line"
QT_MOC_LITERAL(20, 287, 7), // "lineNum"
QT_MOC_LITERAL(21, 295, 8), // "allLines"
QT_MOC_LITERAL(22, 304, 19), // "isLikelyConstructor"
QT_MOC_LITERAL(23, 324, 12), // "functionName"
QT_MOC_LITERAL(24, 337, 15), // "onEditorChanged"
QT_MOC_LITERAL(25, 353, 14), // "QsciScintilla*"
QT_MOC_LITERAL(26, 368, 6), // "editor"
QT_MOC_LITERAL(27, 375, 26), // "highlightCurrentLineNumber"
QT_MOC_LITERAL(28, 402, 24), // "onOccurrenceTimerTimeout"
QT_MOC_LITERAL(29, 427, 28), // "onCustomContextMenuRequested"
QT_MOC_LITERAL(30, 456, 3) // "pos"

    },
    "CodeEditor\0fileOpened\0\0filePath\0"
    "fileSaved\0newFileRequested\0openFileRequested\0"
    "saveFileRequested\0buildRequested\0"
    "cleanRequested\0debugRequested\0"
    "runRequested\0stopRequested\0"
    "serialMonitorRequested\0settingsRequested\0"
    "helpRequested\0aboutRequested\0"
    "updateVariableList\0isValidFunctionDefinition\0"
    "line\0lineNum\0allLines\0isLikelyConstructor\0"
    "functionName\0onEditorChanged\0"
    "QsciScintilla*\0editor\0highlightCurrentLineNumber\0"
    "onOccurrenceTimerTimeout\0"
    "onCustomContextMenuRequested\0pos"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CodeEditor[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      21,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      14,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  119,    2, 0x06 /* Public */,
       4,    1,  122,    2, 0x06 /* Public */,
       5,    0,  125,    2, 0x06 /* Public */,
       6,    0,  126,    2, 0x06 /* Public */,
       7,    0,  127,    2, 0x06 /* Public */,
       8,    0,  128,    2, 0x06 /* Public */,
       9,    0,  129,    2, 0x06 /* Public */,
      10,    0,  130,    2, 0x06 /* Public */,
      11,    0,  131,    2, 0x06 /* Public */,
      12,    0,  132,    2, 0x06 /* Public */,
      13,    0,  133,    2, 0x06 /* Public */,
      14,    0,  134,    2, 0x06 /* Public */,
      15,    0,  135,    2, 0x06 /* Public */,
      16,    0,  136,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      17,    0,  137,    2, 0x0a /* Public */,
      18,    3,  138,    2, 0x0a /* Public */,
      22,    2,  145,    2, 0x0a /* Public */,
      24,    1,  150,    2, 0x08 /* Private */,
      27,    0,  153,    2, 0x08 /* Private */,
      28,    0,  154,    2, 0x08 /* Private */,
      29,    1,  155,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
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
    QMetaType::Bool, QMetaType::QString, QMetaType::Int, QMetaType::QStringList,   19,   20,   21,
    QMetaType::Bool, QMetaType::QString, QMetaType::QString,   23,   19,
    QMetaType::Void, 0x80000000 | 25,   26,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,   30,

       0        // eod
};

void CodeEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CodeEditor *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->fileOpened((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->fileSaved((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->newFileRequested(); break;
        case 3: _t->openFileRequested(); break;
        case 4: _t->saveFileRequested(); break;
        case 5: _t->buildRequested(); break;
        case 6: _t->cleanRequested(); break;
        case 7: _t->debugRequested(); break;
        case 8: _t->runRequested(); break;
        case 9: _t->stopRequested(); break;
        case 10: _t->serialMonitorRequested(); break;
        case 11: _t->settingsRequested(); break;
        case 12: _t->helpRequested(); break;
        case 13: _t->aboutRequested(); break;
        case 14: _t->updateVariableList(); break;
        case 15: { bool _r = _t->isValidFunctionDefinition((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QStringList(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 16: { bool _r = _t->isLikelyConstructor((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 17: _t->onEditorChanged((*reinterpret_cast< QsciScintilla*(*)>(_a[1]))); break;
        case 18: _t->highlightCurrentLineNumber(); break;
        case 19: _t->onOccurrenceTimerTimeout(); break;
        case 20: _t->onCustomContextMenuRequested((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 17:
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
            using _t = void (CodeEditor::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::fileOpened)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::fileSaved)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::newFileRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::openFileRequested)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::saveFileRequested)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::buildRequested)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::cleanRequested)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::debugRequested)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::runRequested)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::stopRequested)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::serialMonitorRequested)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::settingsRequested)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::helpRequested)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (CodeEditor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CodeEditor::aboutRequested)) {
                *result = 13;
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
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    return _id;
}

// SIGNAL 0
void CodeEditor::fileOpened(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CodeEditor::fileSaved(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CodeEditor::newFileRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void CodeEditor::openFileRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void CodeEditor::saveFileRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void CodeEditor::buildRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void CodeEditor::cleanRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void CodeEditor::debugRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void CodeEditor::runRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void CodeEditor::stopRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void CodeEditor::serialMonitorRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void CodeEditor::settingsRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void CodeEditor::helpRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void CodeEditor::aboutRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
