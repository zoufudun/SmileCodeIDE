/****************************************************************************
** Meta object code from reading C++ file 'templateplugin.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../templateplugin.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'templateplugin.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TemplatePlugin_t {
    QByteArrayData data[1];
    char stringdata0[15];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TemplatePlugin_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TemplatePlugin_t qt_meta_stringdata_TemplatePlugin = {
    {
QT_MOC_LITERAL(0, 0, 14) // "TemplatePlugin"

    },
    "TemplatePlugin"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TemplatePlugin[] = {

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

void TemplatePlugin::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject TemplatePlugin::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_TemplatePlugin.data,
    qt_meta_data_TemplatePlugin,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TemplatePlugin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TemplatePlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TemplatePlugin.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "IAppPlugin"))
        return static_cast< IAppPlugin*>(this);
    if (!strcmp(_clname, "com.smilecode.plugin.IAppPlugin/1.0"))
        return static_cast< IAppPlugin*>(this);
    return QObject::qt_metacast(_clname);
}

int TemplatePlugin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
    0x03,  0x6e,  'T',  'e',  'm',  'p',  'l',  'a', 
    't',  'e',  'P',  'l',  'u',  'g',  'i',  'n', 
    // "MetaData"
    0x04,  0xa5,  0x66,  'a',  'u',  't',  'h',  'o', 
    'r',  0x76,  uchar('\xe5'), uchar('\xbc'), uchar('\x80'), uchar('\xe5'), uchar('\x8f'), uchar('\x91'),
    uchar('\xe8'), uchar('\x80'), uchar('\x85'), uchar('\xe5'), uchar('\x90'), uchar('\x8d'), uchar('\xe7'), uchar('\xa7'),
    uchar('\xb0'), '/',  uchar('\xe5'), uchar('\x9b'), uchar('\xa2'), uchar('\xe9'), uchar('\x98'), uchar('\x9f'),
    0x68,  'c',  'a',  't',  'e',  'g',  'o',  'r', 
    'y',  0x6c,  uchar('\xe5'), uchar('\xae'), uchar('\x9e'), uchar('\xe7'), uchar('\x94'), uchar('\xa8'),
    uchar('\xe5'), uchar('\xb7'), uchar('\xa5'), uchar('\xe5'), uchar('\x85'), uchar('\xb7'), 0x62,  'i', 
    'd',  0x6f,  't',  'e',  'm',  'p',  'l',  'a', 
    't',  'e',  '_',  'p',  'l',  'u',  'g',  'i', 
    'n',  0x64,  'n',  'a',  'm',  'e',  0x78,  0x1b, 
    uchar('\xe8'), uchar('\x87'), uchar('\xaa'), uchar('\xe5'), uchar('\xae'), uchar('\x9a'), uchar('\xe4'), uchar('\xb9'),
    uchar('\x89'), uchar('\xe6'), uchar('\x89'), uchar('\xa9'), uchar('\xe5'), uchar('\xb1'), uchar('\x95'), uchar('\xe6'),
    uchar('\x8f'), uchar('\x92'), uchar('\xe4'), uchar('\xbb'), uchar('\xb6'), uchar('\xe6'), uchar('\xa8'), uchar('\xa1'),
    uchar('\xe6'), uchar('\x9d'), uchar('\xbf'), 0x67,  'v',  'e',  'r',  's', 
    'i',  'o',  'n',  0x65,  '1',  '.',  '0',  '.', 
    '0', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN(TemplatePlugin, TemplatePlugin)

QT_WARNING_POP
QT_END_MOC_NAMESPACE
