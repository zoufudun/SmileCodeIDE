QT += core gui widgets testlib
CONFIG += testcase c++17 console
TARGET = SourceNavigatorTest
MOC_DIR = $$OUT_PWD/moc
INCLUDEPATH += $$MOC_DIR
SOURCES += SourceNavigatorTest.cpp ../UI/SourceNavigator.cpp
HEADERS += ../UI/SourceNavigator.h

# Optional integration checks reuse an already built host without compiling it again.
contains(CONFIG, host_navigation) {
    QT += printsupport serialbus serialport websockets network core5compat
    DEFINES += SOURCE_NAVIGATOR_HOST_TESTS
    INCLUDEPATH += ../../../PhudonTools ../../../QScintilla/src
    hostObjects = $$cat($$HOST_BUILD_DIR/release/object_script.PhudonTools.Release, lines)
    for(hostObject, hostObjects) {
        !equals(hostObject, release/main.o): LIBS += $$HOST_BUILD_DIR/$$hostObject
    }
    LIBS += $$HOST_BUILD_DIR/release/PhudonTools_resource_res.o
    LIBS += -L$$PWD/../../../QScintilla/src/release -lqscintilla2_qt6
}
