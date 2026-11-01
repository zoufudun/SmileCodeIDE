/****************************************************************************
** Meta object code from reading C++ file 'templateplugin.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../templateplugin.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'templateplugin.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14TemplatePluginE_t {};
} // unnamed namespace

template <> constexpr inline auto TemplatePlugin::qt_create_metaobjectdata<qt_meta_tag_ZN14TemplatePluginE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TemplatePlugin"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TemplatePlugin, qt_meta_tag_ZN14TemplatePluginE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TemplatePlugin::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14TemplatePluginE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14TemplatePluginE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14TemplatePluginE_t>.metaTypes,
    nullptr
} };

void TemplatePlugin::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TemplatePlugin *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *TemplatePlugin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TemplatePlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14TemplatePluginE_t>.strings))
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

#ifdef QT_MOC_EXPORT_PLUGIN_V2
static constexpr unsigned char qt_pluginMetaDataV2_TemplatePlugin[] = {
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
QT_MOC_EXPORT_PLUGIN_V2(TemplatePlugin, TemplatePlugin, qt_pluginMetaDataV2_TemplatePlugin)
#else
QT_PLUGIN_METADATA_SECTION
Q_CONSTINIT static constexpr unsigned char qt_pluginMetaData_TemplatePlugin[] = {
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
#endif  // QT_MOC_EXPORT_PLUGIN_V2

QT_WARNING_POP
