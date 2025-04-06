/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../STM32IDE/mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_MainWindow_t {
    uint offsetsAndSizes[108];
    char stringdata0[11];
    char stringdata1[12];
    char stringdata2[1];
    char stringdata3[11];
    char stringdata4[12];
    char stringdata5[13];
    char stringdata6[13];
    char stringdata7[13];
    char stringdata8[11];
    char stringdata9[10];
    char stringdata10[14];
    char stringdata11[9];
    char stringdata12[9];
    char stringdata13[8];
    char stringdata14[14];
    char stringdata15[19];
    char stringdata16[14];
    char stringdata17[13];
    char stringdata18[16];
    char stringdata19[9];
    char stringdata20[21];
    char stringdata21[11];
    char stringdata22[18];
    char stringdata23[5];
    char stringdata24[18];
    char stringdata25[6];
    char stringdata26[17];
    char stringdata27[12];
    char stringdata28[11];
    char stringdata29[16];
    char stringdata30[17];
    char stringdata31[13];
    char stringdata32[14];
    char stringdata33[16];
    char stringdata34[21];
    char stringdata35[16];
    char stringdata36[14];
    char stringdata37[22];
    char stringdata38[19];
    char stringdata39[18];
    char stringdata40[12];
    char stringdata41[19];
    char stringdata42[16];
    char stringdata43[13];
    char stringdata44[15];
    char stringdata45[17];
    char stringdata46[23];
    char stringdata47[22];
    char stringdata48[11];
    char stringdata49[10];
    char stringdata50[9];
    char stringdata51[12];
    char stringdata52[16];
    char stringdata53[11];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_MainWindow_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
        QT_MOC_LITERAL(0, 10),  // "MainWindow"
        QT_MOC_LITERAL(11, 11),  // "openProject"
        QT_MOC_LITERAL(23, 0),  // ""
        QT_MOC_LITERAL(24, 10),  // "newProject"
        QT_MOC_LITERAL(35, 11),  // "saveProject"
        QT_MOC_LITERAL(47, 12),  // "buildProject"
        QT_MOC_LITERAL(60, 12),  // "cleanProject"
        QT_MOC_LITERAL(73, 12),  // "flashProject"
        QT_MOC_LITERAL(86, 10),  // "startDebug"
        QT_MOC_LITERAL(97, 9),  // "stopDebug"
        QT_MOC_LITERAL(107, 13),  // "continueDebug"
        QT_MOC_LITERAL(121, 8),  // "stepOver"
        QT_MOC_LITERAL(130, 8),  // "stepInto"
        QT_MOC_LITERAL(139, 7),  // "stepOut"
        QT_MOC_LITERAL(147, 13),  // "setBreakpoint"
        QT_MOC_LITERAL(161, 18),  // "configureToolchain"
        QT_MOC_LITERAL(180, 13),  // "processOutput"
        QT_MOC_LITERAL(194, 12),  // "processError"
        QT_MOC_LITERAL(207, 15),  // "processFinished"
        QT_MOC_LITERAL(223, 8),  // "exitCode"
        QT_MOC_LITERAL(232, 20),  // "QProcess::ExitStatus"
        QT_MOC_LITERAL(253, 10),  // "exitStatus"
        QT_MOC_LITERAL(264, 17),  // "updateProjectTree"
        QT_MOC_LITERAL(282, 4),  // "path"
        QT_MOC_LITERAL(287, 17),  // "downloaderChanged"
        QT_MOC_LITERAL(305, 5),  // "index"
        QT_MOC_LITERAL(311, 16),  // "toggleFullScreen"
        QT_MOC_LITERAL(328, 11),  // "changeTheme"
        QT_MOC_LITERAL(340, 10),  // "themeIndex"
        QT_MOC_LITERAL(351, 15),  // "showAboutDialog"
        QT_MOC_LITERAL(367, 16),  // "updateStatusInfo"
        QT_MOC_LITERAL(384, 12),  // "setDarkTheme"
        QT_MOC_LITERAL(397, 13),  // "setLightTheme"
        QT_MOC_LITERAL(411, 15),  // "setOneDarkTheme"
        QT_MOC_LITERAL(427, 20),  // "setAtomMaterialTheme"
        QT_MOC_LITERAL(448, 15),  // "setAtomOneTheme"
        QT_MOC_LITERAL(464, 13),  // "setGerryTheme"
        QT_MOC_LITERAL(478, 21),  // "setMaterialIconsTheme"
        QT_MOC_LITERAL(500, 18),  // "setGithubDarkTheme"
        QT_MOC_LITERAL(519, 17),  // "setXcodeDarkTheme"
        QT_MOC_LITERAL(537, 11),  // "setVueTheme"
        QT_MOC_LITERAL(549, 18),  // "setMonokaiProTheme"
        QT_MOC_LITERAL(568, 15),  // "setDraculaTheme"
        QT_MOC_LITERAL(584, 12),  // "setNordTheme"
        QT_MOC_LITERAL(597, 14),  // "setNoctisTheme"
        QT_MOC_LITERAL(612, 16),  // "setNightOwlTheme"
        QT_MOC_LITERAL(629, 22),  // "setSolarizedLightTheme"
        QT_MOC_LITERAL(652, 21),  // "setMaterialLightTheme"
        QT_MOC_LITERAL(674, 10),  // "applyTheme"
        QT_MOC_LITERAL(685, 9),  // "themeName"
        QT_MOC_LITERAL(695, 8),  // "openFile"
        QT_MOC_LITERAL(704, 11),  // "QModelIndex"
        QT_MOC_LITERAL(716, 15),  // "saveCurrentFile"
        QT_MOC_LITERAL(732, 10)   // "saveFileAs"
    },
    "MainWindow",
    "openProject",
    "",
    "newProject",
    "saveProject",
    "buildProject",
    "cleanProject",
    "flashProject",
    "startDebug",
    "stopDebug",
    "continueDebug",
    "stepOver",
    "stepInto",
    "stepOut",
    "setBreakpoint",
    "configureToolchain",
    "processOutput",
    "processError",
    "processFinished",
    "exitCode",
    "QProcess::ExitStatus",
    "exitStatus",
    "updateProjectTree",
    "path",
    "downloaderChanged",
    "index",
    "toggleFullScreen",
    "changeTheme",
    "themeIndex",
    "showAboutDialog",
    "updateStatusInfo",
    "setDarkTheme",
    "setLightTheme",
    "setOneDarkTheme",
    "setAtomMaterialTheme",
    "setAtomOneTheme",
    "setGerryTheme",
    "setMaterialIconsTheme",
    "setGithubDarkTheme",
    "setXcodeDarkTheme",
    "setVueTheme",
    "setMonokaiProTheme",
    "setDraculaTheme",
    "setNordTheme",
    "setNoctisTheme",
    "setNightOwlTheme",
    "setSolarizedLightTheme",
    "setMaterialLightTheme",
    "applyTheme",
    "themeName",
    "openFile",
    "QModelIndex",
    "saveCurrentFile",
    "saveFileAs"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_MainWindow[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      44,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  278,    2, 0x08,    1 /* Private */,
       3,    0,  279,    2, 0x08,    2 /* Private */,
       4,    0,  280,    2, 0x08,    3 /* Private */,
       5,    0,  281,    2, 0x08,    4 /* Private */,
       6,    0,  282,    2, 0x08,    5 /* Private */,
       7,    0,  283,    2, 0x08,    6 /* Private */,
       8,    0,  284,    2, 0x08,    7 /* Private */,
       9,    0,  285,    2, 0x08,    8 /* Private */,
      10,    0,  286,    2, 0x08,    9 /* Private */,
      11,    0,  287,    2, 0x08,   10 /* Private */,
      12,    0,  288,    2, 0x08,   11 /* Private */,
      13,    0,  289,    2, 0x08,   12 /* Private */,
      14,    0,  290,    2, 0x08,   13 /* Private */,
      15,    0,  291,    2, 0x08,   14 /* Private */,
      16,    0,  292,    2, 0x08,   15 /* Private */,
      17,    0,  293,    2, 0x08,   16 /* Private */,
      18,    2,  294,    2, 0x08,   17 /* Private */,
      22,    1,  299,    2, 0x08,   20 /* Private */,
      24,    1,  302,    2, 0x08,   22 /* Private */,
      26,    0,  305,    2, 0x08,   24 /* Private */,
      27,    1,  306,    2, 0x08,   25 /* Private */,
      29,    0,  309,    2, 0x08,   27 /* Private */,
      30,    0,  310,    2, 0x08,   28 /* Private */,
      31,    0,  311,    2, 0x08,   29 /* Private */,
      32,    0,  312,    2, 0x08,   30 /* Private */,
      33,    0,  313,    2, 0x08,   31 /* Private */,
      34,    0,  314,    2, 0x08,   32 /* Private */,
      35,    0,  315,    2, 0x08,   33 /* Private */,
      36,    0,  316,    2, 0x08,   34 /* Private */,
      37,    0,  317,    2, 0x08,   35 /* Private */,
      38,    0,  318,    2, 0x08,   36 /* Private */,
      39,    0,  319,    2, 0x08,   37 /* Private */,
      40,    0,  320,    2, 0x08,   38 /* Private */,
      41,    0,  321,    2, 0x08,   39 /* Private */,
      42,    0,  322,    2, 0x08,   40 /* Private */,
      43,    0,  323,    2, 0x08,   41 /* Private */,
      44,    0,  324,    2, 0x08,   42 /* Private */,
      45,    0,  325,    2, 0x08,   43 /* Private */,
      46,    0,  326,    2, 0x08,   44 /* Private */,
      47,    0,  327,    2, 0x08,   45 /* Private */,
      48,    1,  328,    2, 0x08,   46 /* Private */,
      50,    1,  331,    2, 0x08,   48 /* Private */,
      52,    0,  334,    2, 0x08,   50 /* Private */,
      53,    0,  335,    2, 0x08,   51 /* Private */,

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
    QMetaType::Void, QMetaType::QString,   49,
    QMetaType::Void, 0x80000000 | 51,   25,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.offsetsAndSizes,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_MainWindow_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'openProject'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'newProject'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveProject'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'buildProject'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cleanProject'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'flashProject'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startDebug'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopDebug'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'continueDebug'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stepOver'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stepInto'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stepOut'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setBreakpoint'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'configureToolchain'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'processOutput'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'processError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'processFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ExitStatus, std::false_type>,
        // method 'updateProjectTree'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'downloaderChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'toggleFullScreen'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'changeTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'showAboutDialog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateStatusInfo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setDarkTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setLightTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setOneDarkTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setAtomMaterialTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setAtomOneTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setGerryTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setMaterialIconsTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setGithubDarkTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setXcodeDarkTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setVueTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setMonokaiProTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setDraculaTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setNordTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setNoctisTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setNightOwlTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setSolarizedLightTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setMaterialLightTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'applyTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'openFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>,
        // method 'saveCurrentFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveFileAs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
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
        case 16: _t->processFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 17: _t->updateProjectTree((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 18: _t->downloaderChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 19: _t->toggleFullScreen(); break;
        case 20: _t->changeTheme((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 21: _t->showAboutDialog(); break;
        case 22: _t->updateStatusInfo(); break;
        case 23: _t->setDarkTheme(); break;
        case 24: _t->setLightTheme(); break;
        case 25: _t->setOneDarkTheme(); break;
        case 26: _t->setAtomMaterialTheme(); break;
        case 27: _t->setAtomOneTheme(); break;
        case 28: _t->setGerryTheme(); break;
        case 29: _t->setMaterialIconsTheme(); break;
        case 30: _t->setGithubDarkTheme(); break;
        case 31: _t->setXcodeDarkTheme(); break;
        case 32: _t->setVueTheme(); break;
        case 33: _t->setMonokaiProTheme(); break;
        case 34: _t->setDraculaTheme(); break;
        case 35: _t->setNordTheme(); break;
        case 36: _t->setNoctisTheme(); break;
        case 37: _t->setNightOwlTheme(); break;
        case 38: _t->setSolarizedLightTheme(); break;
        case 39: _t->setMaterialLightTheme(); break;
        case 40: _t->applyTheme((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 41: _t->openFile((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 42: _t->saveCurrentFile(); break;
        case 43: _t->saveFileAs(); break;
        default: ;
        }
    }
}

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
        if (_id < 44)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 44;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 44)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 44;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
