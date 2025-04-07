/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../STM32IDE/mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[60];
    char stringdata0[818];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 11), // "openProject"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 10), // "newProject"
QT_MOC_LITERAL(4, 35, 11), // "saveProject"
QT_MOC_LITERAL(5, 47, 12), // "buildProject"
QT_MOC_LITERAL(6, 60, 12), // "cleanProject"
QT_MOC_LITERAL(7, 73, 12), // "flashProject"
QT_MOC_LITERAL(8, 86, 10), // "startDebug"
QT_MOC_LITERAL(9, 97, 9), // "stopDebug"
QT_MOC_LITERAL(10, 107, 13), // "continueDebug"
QT_MOC_LITERAL(11, 121, 8), // "stepOver"
QT_MOC_LITERAL(12, 130, 8), // "stepInto"
QT_MOC_LITERAL(13, 139, 7), // "stepOut"
QT_MOC_LITERAL(14, 147, 13), // "setBreakpoint"
QT_MOC_LITERAL(15, 161, 18), // "configureToolchain"
QT_MOC_LITERAL(16, 180, 13), // "processOutput"
QT_MOC_LITERAL(17, 194, 12), // "processError"
QT_MOC_LITERAL(18, 207, 15), // "processFinished"
QT_MOC_LITERAL(19, 223, 8), // "exitCode"
QT_MOC_LITERAL(20, 232, 20), // "QProcess::ExitStatus"
QT_MOC_LITERAL(21, 253, 10), // "exitStatus"
QT_MOC_LITERAL(22, 264, 17), // "updateProjectTree"
QT_MOC_LITERAL(23, 282, 4), // "path"
QT_MOC_LITERAL(24, 287, 17), // "downloaderChanged"
QT_MOC_LITERAL(25, 305, 5), // "index"
QT_MOC_LITERAL(26, 311, 16), // "toggleFullScreen"
QT_MOC_LITERAL(27, 328, 11), // "changeTheme"
QT_MOC_LITERAL(28, 340, 10), // "themeIndex"
QT_MOC_LITERAL(29, 351, 15), // "showAboutDialog"
QT_MOC_LITERAL(30, 367, 16), // "updateStatusInfo"
QT_MOC_LITERAL(31, 384, 19), // "onFileDoubleClicked"
QT_MOC_LITERAL(32, 404, 11), // "QModelIndex"
QT_MOC_LITERAL(33, 416, 15), // "showContextMenu"
QT_MOC_LITERAL(34, 432, 3), // "pos"
QT_MOC_LITERAL(35, 436, 12), // "setDarkTheme"
QT_MOC_LITERAL(36, 449, 13), // "setLightTheme"
QT_MOC_LITERAL(37, 463, 15), // "setOneDarkTheme"
QT_MOC_LITERAL(38, 479, 20), // "setAtomMaterialTheme"
QT_MOC_LITERAL(39, 500, 15), // "setAtomOneTheme"
QT_MOC_LITERAL(40, 516, 13), // "setGerryTheme"
QT_MOC_LITERAL(41, 530, 21), // "setMaterialIconsTheme"
QT_MOC_LITERAL(42, 552, 18), // "setGithubDarkTheme"
QT_MOC_LITERAL(43, 571, 17), // "setXcodeDarkTheme"
QT_MOC_LITERAL(44, 589, 11), // "setVueTheme"
QT_MOC_LITERAL(45, 601, 18), // "setMonokaiProTheme"
QT_MOC_LITERAL(46, 620, 15), // "setDraculaTheme"
QT_MOC_LITERAL(47, 636, 12), // "setNordTheme"
QT_MOC_LITERAL(48, 649, 14), // "setNoctisTheme"
QT_MOC_LITERAL(49, 664, 16), // "setNightOwlTheme"
QT_MOC_LITERAL(50, 681, 22), // "setSolarizedLightTheme"
QT_MOC_LITERAL(51, 704, 21), // "setMaterialLightTheme"
QT_MOC_LITERAL(52, 726, 10), // "applyTheme"
QT_MOC_LITERAL(53, 737, 9), // "themeName"
QT_MOC_LITERAL(54, 747, 8), // "openFile"
QT_MOC_LITERAL(55, 756, 15), // "saveCurrentFile"
QT_MOC_LITERAL(56, 772, 10), // "saveFileAs"
QT_MOC_LITERAL(57, 783, 17), // "executeGdbCommand"
QT_MOC_LITERAL(58, 801, 7), // "newFile"
QT_MOC_LITERAL(59, 809, 8) // "saveFile"

    },
    "MainWindow\0openProject\0\0newProject\0"
    "saveProject\0buildProject\0cleanProject\0"
    "flashProject\0startDebug\0stopDebug\0"
    "continueDebug\0stepOver\0stepInto\0stepOut\0"
    "setBreakpoint\0configureToolchain\0"
    "processOutput\0processError\0processFinished\0"
    "exitCode\0QProcess::ExitStatus\0exitStatus\0"
    "updateProjectTree\0path\0downloaderChanged\0"
    "index\0toggleFullScreen\0changeTheme\0"
    "themeIndex\0showAboutDialog\0updateStatusInfo\0"
    "onFileDoubleClicked\0QModelIndex\0"
    "showContextMenu\0pos\0setDarkTheme\0"
    "setLightTheme\0setOneDarkTheme\0"
    "setAtomMaterialTheme\0setAtomOneTheme\0"
    "setGerryTheme\0setMaterialIconsTheme\0"
    "setGithubDarkTheme\0setXcodeDarkTheme\0"
    "setVueTheme\0setMonokaiProTheme\0"
    "setDraculaTheme\0setNordTheme\0"
    "setNoctisTheme\0setNightOwlTheme\0"
    "setSolarizedLightTheme\0setMaterialLightTheme\0"
    "applyTheme\0themeName\0openFile\0"
    "saveCurrentFile\0saveFileAs\0executeGdbCommand\0"
    "newFile\0saveFile"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      49,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  259,    2, 0x08 /* Private */,
       3,    0,  260,    2, 0x08 /* Private */,
       4,    0,  261,    2, 0x08 /* Private */,
       5,    0,  262,    2, 0x08 /* Private */,
       6,    0,  263,    2, 0x08 /* Private */,
       7,    0,  264,    2, 0x08 /* Private */,
       8,    0,  265,    2, 0x08 /* Private */,
       9,    0,  266,    2, 0x08 /* Private */,
      10,    0,  267,    2, 0x08 /* Private */,
      11,    0,  268,    2, 0x08 /* Private */,
      12,    0,  269,    2, 0x08 /* Private */,
      13,    0,  270,    2, 0x08 /* Private */,
      14,    0,  271,    2, 0x08 /* Private */,
      15,    0,  272,    2, 0x08 /* Private */,
      16,    0,  273,    2, 0x08 /* Private */,
      17,    0,  274,    2, 0x08 /* Private */,
      18,    2,  275,    2, 0x08 /* Private */,
      22,    1,  280,    2, 0x08 /* Private */,
      24,    1,  283,    2, 0x08 /* Private */,
      26,    0,  286,    2, 0x08 /* Private */,
      27,    1,  287,    2, 0x08 /* Private */,
      29,    0,  290,    2, 0x08 /* Private */,
      30,    0,  291,    2, 0x08 /* Private */,
      31,    1,  292,    2, 0x08 /* Private */,
      33,    1,  295,    2, 0x08 /* Private */,
      35,    0,  298,    2, 0x08 /* Private */,
      36,    0,  299,    2, 0x08 /* Private */,
      37,    0,  300,    2, 0x08 /* Private */,
      38,    0,  301,    2, 0x08 /* Private */,
      39,    0,  302,    2, 0x08 /* Private */,
      40,    0,  303,    2, 0x08 /* Private */,
      41,    0,  304,    2, 0x08 /* Private */,
      42,    0,  305,    2, 0x08 /* Private */,
      43,    0,  306,    2, 0x08 /* Private */,
      44,    0,  307,    2, 0x08 /* Private */,
      45,    0,  308,    2, 0x08 /* Private */,
      46,    0,  309,    2, 0x08 /* Private */,
      47,    0,  310,    2, 0x08 /* Private */,
      48,    0,  311,    2, 0x08 /* Private */,
      49,    0,  312,    2, 0x08 /* Private */,
      50,    0,  313,    2, 0x08 /* Private */,
      51,    0,  314,    2, 0x08 /* Private */,
      52,    1,  315,    2, 0x08 /* Private */,
      54,    1,  318,    2, 0x08 /* Private */,
      55,    0,  321,    2, 0x08 /* Private */,
      56,    0,  322,    2, 0x08 /* Private */,
      57,    0,  323,    2, 0x08 /* Private */,
      58,    0,  324,    2, 0x08 /* Private */,
      59,    0,  325,    2, 0x08 /* Private */,

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
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 20,   19,   21,
    QMetaType::Void, QMetaType::QString,   23,
    QMetaType::Void, QMetaType::Int,   25,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   28,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 32,   25,
    QMetaType::Void, QMetaType::QPoint,   34,
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
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   53,
    QMetaType::Void, 0x80000000 | 32,   25,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->openProject(); break;
        case 1: _t->newProject(); break;
        case 2: _t->saveProject(); break;
        case 3: _t->buildProject(); break;
        case 4: _t->cleanProject(); break;
        case 5: _t->flashProject(); break;
        case 6: _t->startDebug(); break;
        case 7: _t->stopDebug(); break;
        case 8: _t->continueDebug(); break;
        case 9: _t->stepOver(); break;
        case 10: _t->stepInto(); break;
        case 11: _t->stepOut(); break;
        case 12: _t->setBreakpoint(); break;
        case 13: _t->configureToolchain(); break;
        case 14: _t->processOutput(); break;
        case 15: _t->processError(); break;
        case 16: _t->processFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QProcess::ExitStatus(*)>(_a[2]))); break;
        case 17: _t->updateProjectTree((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 18: _t->downloaderChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 19: _t->toggleFullScreen(); break;
        case 20: _t->changeTheme((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 21: _t->showAboutDialog(); break;
        case 22: _t->updateStatusInfo(); break;
        case 23: _t->onFileDoubleClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 24: _t->showContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 25: _t->setDarkTheme(); break;
        case 26: _t->setLightTheme(); break;
        case 27: _t->setOneDarkTheme(); break;
        case 28: _t->setAtomMaterialTheme(); break;
        case 29: _t->setAtomOneTheme(); break;
        case 30: _t->setGerryTheme(); break;
        case 31: _t->setMaterialIconsTheme(); break;
        case 32: _t->setGithubDarkTheme(); break;
        case 33: _t->setXcodeDarkTheme(); break;
        case 34: _t->setVueTheme(); break;
        case 35: _t->setMonokaiProTheme(); break;
        case 36: _t->setDraculaTheme(); break;
        case 37: _t->setNordTheme(); break;
        case 38: _t->setNoctisTheme(); break;
        case 39: _t->setNightOwlTheme(); break;
        case 40: _t->setSolarizedLightTheme(); break;
        case 41: _t->setMaterialLightTheme(); break;
        case 42: _t->applyTheme((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 43: _t->openFile((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 44: _t->saveCurrentFile(); break;
        case 45: _t->saveFileAs(); break;
        case 46: _t->executeGdbCommand(); break;
        case 47: _t->newFile(); break;
        case 48: _t->saveFile(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 49)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 49;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 49)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 49;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
