# Check the Qt version. If QT_VERSION is not set, it is probably Qt 3.
isEmpty(QT_VERSION) {
    error("QT_VERSION not defined. Tiled does not work with Qt 3.")
}

include(tiled.pri)

!minQtVersion(5, 1, 0) {
    message("Cannot build Tiled with Qt version $${QT_VERSION}")
    error("Use at least Qt 5.1.0.")
}

win* {
    message("Building Tiled for Windows using qmake is no longer supported")
    error("Use the tiled.qbs project file instead")
}

TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = src translations

INCLUDEPATH +=
INCLUDEPATH += C:\Program Files (x86)\GnuWin32\lib
INCLUDEPATH += C:\Program Files (x86)\GnuWin32\include


