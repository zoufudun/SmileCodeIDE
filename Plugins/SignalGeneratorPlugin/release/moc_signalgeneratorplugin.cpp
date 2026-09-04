/****************************************************************************
** Meta object code from reading C++ file 'signalgeneratorplugin.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../signalgeneratorplugin.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'signalgeneratorplugin.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SignalGeneratorPlugin_t {
    QByteArrayData data[1];
    char stringdata0[22];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SignalGeneratorPlugin_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SignalGeneratorPlugin_t qt_meta_stringdata_SignalGeneratorPlugin = {
    {
QT_MOC_LITERAL(0, 0, 21) // "SignalGeneratorPlugin"

    },
    "SignalGeneratorPlugin"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SignalGeneratorPlugin[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void SignalGeneratorPlugin::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject SignalGeneratorPlugin::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_SignalGeneratorPlugin.data,
    qt_meta_data_SignalGeneratorPlugin,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SignalGeneratorPlugin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SignalGeneratorPlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SignalGeneratorPlugin.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "IAppPlugin"))
        return static_cast< IAppPlugin*>(this);
    if (!strcmp(_clname, "com.smilecode.plugin.IAppPlugin/1.0"))
        return static_cast< IAppPlugin*>(this);
    return QObject::qt_metacast(_clname);
}

int SignalGeneratorPlugin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    return _id;
}

QT_PLUGIN_METADATA_SECTION
static constexpr unsigned char qt_pluginMetaData[] = {
    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', '!',
    // metadata version, Qt version, architectural requirements
    0, QT_VERSION_MAJOR, QT_VERSION_MINOR, qPluginArchRequirements(),
    0xbf, 
    // "IID"
    0x02,  0x78,  0x23,  'c',  'o',  'm',  '.',  's', 
    'm',  'i',  'l',  'e',  'c',  'o',  'd',  'e', 
    '.',  'p',  'l',  'u',  'g',  'i',  'n',  '.', 
    'I',  'A',  'p',  'p',  'P',  'l',  'u',  'g', 
    'i',  'n',  '/',  '1',  '.',  '0', 
    // "className"
    0x03,  0x75,  'S',  'i',  'g',  'n',  'a',  'l', 
    'G',  'e',  'n',  'e',  'r',  'a',  't',  'o', 
    'r',  'P',  'l',  'u',  'g',  'i',  'n', 
    // "MetaData"
    0x04,  0xa5,  0x66,  'a',  'u',  't',  'h',  'o', 
    'r',  0x6e,  'S',  'm',  'i',  'l',  'e',  'C', 
    'o',  'd',  'e',  ' ',  'T',  'e',  'a',  'm', 
    0x68,  'c',  'a',  't',  'e',  'g',  'o',  'r', 
    'y',  0x6f,  uchar('\xe6'), uchar('\xb5'), uchar('\x8b'), uchar('\xe9'), uchar('\x87'), uchar('\x8f'),
    uchar('\xe4'), uchar('\xb8'), uchar('\x8e'), uchar('\xe5'), uchar('\x88'), uchar('\x86'), uchar('\xe6'), uchar('\x9e'),
    uchar('\x90'), 0x62,  'i',  'd',  0x70,  's',  'i',  'g', 
    'n',  'a',  'l',  '_',  'g',  'e',  'n',  'e', 
    'r',  'a',  't',  'o',  'r',  0x64,  'n',  'a', 
    'm',  'e',  0x78,  0x1e,  uchar('\xe9'), uchar('\xab'), uchar('\x98'), uchar('\xe9'),
    uchar('\xa2'), uchar('\x91'), uchar('\xe4'), uchar('\xbf'), uchar('\xa1'), uchar('\xe5'), uchar('\x8f'), uchar('\xb7'),
    uchar('\xe5'), uchar('\x8f'), uchar('\x91'), uchar('\xe7'), uchar('\x94'), uchar('\x9f'), uchar('\xe4'), uchar('\xb8'),
    uchar('\x8e'), uchar('\xe6'), uchar('\xa8'), uchar('\xa1'), uchar('\xe6'), uchar('\x8b'), uchar('\x9f'), uchar('\xe5'),
    uchar('\x99'), uchar('\xa8'), 0x67,  'v',  'e',  'r',  's',  'i', 
    'o',  'n',  0x65,  '1',  '.',  '0',  '.',  '0', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN(SignalGeneratorPlugin, SignalGeneratorPlugin)

QT_WARNING_POP
QT_END_MOC_NAMESPACE
