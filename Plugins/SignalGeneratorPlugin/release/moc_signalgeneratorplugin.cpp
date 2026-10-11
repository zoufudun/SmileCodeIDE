/****************************************************************************
** Meta object code from reading C++ file 'signalgeneratorplugin.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../signalgeneratorplugin.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'signalgeneratorplugin.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN21SignalGeneratorPluginE_t {};
} // unnamed namespace

template <> constexpr inline auto SignalGeneratorPlugin::qt_create_metaobjectdata<qt_meta_tag_ZN21SignalGeneratorPluginE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SignalGeneratorPlugin"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SignalGeneratorPlugin, qt_meta_tag_ZN21SignalGeneratorPluginE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SignalGeneratorPlugin::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21SignalGeneratorPluginE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21SignalGeneratorPluginE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN21SignalGeneratorPluginE_t>.metaTypes,
    nullptr
} };

void SignalGeneratorPlugin::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SignalGeneratorPlugin *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *SignalGeneratorPlugin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SignalGeneratorPlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21SignalGeneratorPluginE_t>.strings))
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

#ifdef QT_MOC_EXPORT_PLUGIN_V2
static constexpr unsigned char qt_pluginMetaDataV2_SignalGeneratorPlugin[] = {
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
QT_MOC_EXPORT_PLUGIN_V2(SignalGeneratorPlugin, SignalGeneratorPlugin, qt_pluginMetaDataV2_SignalGeneratorPlugin)
#else
QT_PLUGIN_METADATA_SECTION
Q_CONSTINIT static constexpr unsigned char qt_pluginMetaData_SignalGeneratorPlugin[] = {
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
#endif  // QT_MOC_EXPORT_PLUGIN_V2

QT_WARNING_POP
