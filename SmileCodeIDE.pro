TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    qscintilla \
    PhudonTools \
    Plugins

qscintilla.file = $$PWD/QScintilla/src/qscintilla.pro

PhudonTools.file = $$PWD/PhudonTools/PhudonTools.pro
PhudonTools.depends = qscintilla

Plugins.file = $$PWD/Plugins/Plugins.pro
Plugins.depends = PhudonTools
