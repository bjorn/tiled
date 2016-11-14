include(../../tiled.pri)
include(../libtiled/libtiled.pri)
include(../qtpropertybrowser/src/qtpropertybrowser.pri)
include(../qtsingleapplication/src/qtsingleapplication.pri)

TEMPLATE = app
TARGET = tiled
target.path = $${PREFIX}/bin
INSTALLS += target
win32|!isEmpty(TILED_LINUX_ARCHIVE) {
    DESTDIR = ../..
} else {
    DESTDIR = ../../bin
}

QT += widgets

contains(QT_CONFIG, opengl):!macx: QT += opengl

DEFINES += TILED_VERSION=$${TILED_VERSION}

DEFINES += QT_NO_CAST_FROM_ASCII \
    QT_NO_CAST_TO_ASCII

!isEmpty(TILED_LINUX_ARCHIVE):DEFINES += TILED_LINUX_ARCHIVE

macx {
    QMAKE_LIBDIR += $$OUT_PWD/../../bin/Tiled.app/Contents/Frameworks
    LIBS += -framework Foundation
    DEFINES += QT_NO_OPENGL
    OBJECTIVE_SOURCES += macsupport.mm

    sparkle {
        SPARKLE_DIR = /Library/Frameworks

        !exists($${SPARKLE_DIR}/Sparkle.framework) {
            error("Sparkle.framework not found at $${SPARKLE_DIR}")
        }

        DEFINES += TILED_SPARKLE
        LIBS += -framework Sparkle -framework AppKit
        LIBS += -F$${SPARKLE_DIR}
        QMAKE_OBJECTIVE_CFLAGS += -F$${SPARKLE_DIR}
        OBJECTIVE_SOURCES += sparkleautoupdater.mm

        APP_RESOURCES.path = Contents/Resources
        APP_RESOURCES.files = \
            ../../dist/dsa_pub.pem \
            images/tmx-icon-mac.icns

        SPARKLE_FRAMEWORK.path = Contents/Frameworks
        SPARKLE_FRAMEWORK.files = $${SPARKLE_DIR}/Sparkle.framework

        QMAKE_BUNDLE_DATA += APP_RESOURCES SPARKLE_FRAMEWORK
    }
} else:win32 {
    LIBS += -L$$OUT_PWD/../../lib
} else {
    QMAKE_LIBDIR = $$OUT_PWD/../../lib $$QMAKE_LIBDIR
}

# Make sure the Tiled executable can find libtiled
!win32:!macx:!cygwin:contains(RPATH, yes) {
    QMAKE_RPATHDIR += \$\$ORIGIN/../lib

    # It is not possible to use ORIGIN in QMAKE_RPATHDIR, so a bit manually
    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$$join(QMAKE_RPATHDIR, ":")\'
    QMAKE_RPATHDIR =
}

SOURCES += aboutdialog.cpp \
    abstractobjecttool.cpp \
    abstracttiletool.cpp \
    abstracttool.cpp \
    addpropertydialog.cpp \
    addremovelayer.cpp \
    addremovemapobject.cpp \
    addremoveterrain.cpp \
    addremovetiles.cpp \
    addremovetileset.cpp \
    adjusttileindexes.cpp \
    automapper.cpp \
    automapperwrapper.cpp \
    automappingmanager.cpp \
    automappingutils.cpp  \
    autoupdater.cpp \
    brokenlinks.cpp \
    brushitem.cpp \
    bucketfilltool.cpp \
    changeimagelayerposition.cpp \
    changeimagelayerproperties.cpp \
    changelayer.cpp \
    changemapobject.cpp \
    changemapobjectsorder.cpp \
    changemapproperty.cpp \
    changeobjectgroupproperties.cpp \
    changepolygon.cpp \
    changeproperties.cpp \
    changetileanimation.cpp \
    changetileimagesource.cpp \
    changetileobjectgroup.cpp \
    changetileoffset.cpp \
    changetileprobability.cpp \
    changeselectedarea.cpp \
    changetileterrain.cpp \
    clipboardmanager.cpp \
    colorbutton.cpp \
    commandbutton.cpp \
    command.cpp \
    commanddatamodel.cpp \
    commanddialog.cpp \
    commandlineparser.cpp \
    consoledock.cpp \
    createellipseobjecttool.cpp \
    createmultipointobjecttool.cpp \
    createobjecttool.cpp \
    createpolygonobjecttool.cpp \
    createpolylineobjecttool.cpp \
    createrectangleobjecttool.cpp \
    createscalableobjecttool.cpp \
    createtileobjecttool.cpp \
    documentmanager.cpp \
    editpolygontool.cpp \
    editterraindialog.cpp \
    eraser.cpp \
    erasetiles.cpp \
    exportasimagedialog.cpp \
    fileedit.cpp \
    filesystemwatcher.cpp \
    flexiblescrollbar.cpp \
    flipmapobjects.cpp \
    geometry.cpp \
    imagelayeritem.cpp \
    languagemanager.cpp \
    layerdock.cpp \
    layermodel.cpp \
    layeroffsettool.cpp \
    magicwandtool.cpp \
    main.cpp \
    mainwindow.cpp \
    mapdocumentactionhandler.cpp \
    mapdocument.cpp \
    mapobjectitem.cpp \
    mapobjectmodel.cpp \
    mapscene.cpp \
    mapsdock.cpp \
    mapview.cpp \
    minimap.cpp \
    minimapdock.cpp \
    movelayer.cpp \
    movemapobject.cpp \
    movemapobjecttogroup.cpp \
    movetileset.cpp \
    newmapdialog.cpp \
    newtilesetdialog.cpp \
    objectgroupitem.cpp \
    objectsdock.cpp \
    objectselectionitem.cpp \
    objectselectiontool.cpp \
    objecttypes.cpp \
    objecttypeseditor.cpp \
    objecttypesmodel.cpp \
    offsetlayer.cpp \
    offsetmapdialog.cpp \
    painttilelayer.cpp \
    patreondialog.cpp \
    pluginlistmodel.cpp \
    preferences.cpp \
    preferencesdialog.cpp \
    propertiesdock.cpp \
    propertybrowser.cpp \
    raiselowerhelper.cpp \
    renamelayer.cpp \
    renameterrain.cpp \
    replacetileset.cpp \
    resizedialog.cpp \
    resizehelper.cpp \
    resizemap.cpp \
    resizemapobject.cpp \
    resizetilelayer.cpp \
    rotatemapobject.cpp \
    selectionrectangle.cpp \
    selectsametiletool.cpp \
    snaphelper.cpp \
    stampbrush.cpp \
    standardautoupdater.cpp \
    stylehelper.cpp \
    terrainbrush.cpp \
    terraindock.cpp \
    terrainmodel.cpp \
    terrainview.cpp \
    textpropertyedit.cpp \
    texteditordialog.cpp \
    thumbnailrenderer.cpp \
    tileanimationdriver.cpp \
    tileanimationeditor.cpp \
    tilecollisioneditor.cpp \
    tiledapplication.cpp \
    tiledproxystyle.cpp \
    tilelayeritem.cpp \
    tilepainter.cpp \
    tileselectionitem.cpp \
    tileselectiontool.cpp \
    tilesetchanges.cpp \
    tilesetdock.cpp \
    tilesetmanager.cpp \
    tilesetmodel.cpp \
    tilesetparametersedit.cpp \
    tilesetview.cpp \
    tilestamp.cpp \
    tilestampmanager.cpp \
    tilestampmodel.cpp \
    tilestampsdock.cpp \
    tmxmapformat.cpp \
    toolmanager.cpp \
    undodock.cpp \
    utils.cpp \
    varianteditorfactory.cpp \
    variantpropertymanager.cpp \
    zoomable.cpp \
    clickablelabel.cpp \
    imagecolorpickerwidget.cpp

HEADERS += aboutdialog.h \
    abstractobjecttool.h \
    abstracttiletool.h \
    abstracttool.h \
    addpropertydialog.h \
    addremovelayer.h \
    addremovemapobject.h \
    addremoveterrain.h \
    addremovetiles.h \
    addremovetileset.h \
    adjusttileindexes.h \
    automapper.h \
    automapperwrapper.h \
    automappingmanager.h \
    automappingutils.h \
    autoupdater.h \
    brokenlinks.h \
    brushitem.h \
    bucketfilltool.h \
    changeimagelayerposition.h \
    changeimagelayerproperties.h \
    changelayer.h \
    changemapobject.h \
    changemapobjectsorder.h \
    changemapproperty.h \
    changeobjectgroupproperties.h \
    changepolygon.h \
    changeproperties.h \
    changetileanimation.h \
    changetileimagesource.h \
    changetileobjectgroup.h \
    changetileoffset.h \
    changetileprobability.h \
    changeselectedarea.h \
    changetileterrain.h \
    clipboardmanager.h \
    colorbutton.h \
    containerhelpers.h \
    commandbutton.h \
    commanddatamodel.h \
    commanddialog.h \
    command.h \
    commandlineparser.h \
    consoledock.h \
    createellipseobjecttool.h \
    createmultipointobjecttool.h \
    createobjecttool.h \
    createpolygonobjecttool.h \
    createpolylineobjecttool.h \
    createrectangleobjecttool.h \
    createscalableobjecttool.h \
    createtileobjecttool.h \
    documentmanager.h \
    editpolygontool.h \
    editterraindialog.h \
    eraser.h \
    erasetiles.h \
    exportasimagedialog.h \
    fileedit.h \
    filesystemwatcher.h \
    flexiblescrollbar.h \
    flipmapobjects.h \
    geometry.h \
    imagelayeritem.h \
    languagemanager.h \
    layerdock.h \
    layermodel.h \
    layeroffsettool.h \
    macsupport.h \
    magicwandtool.h \
    mainwindow.h \
    mapdocumentactionhandler.h \
    mapdocument.h \
    mapobjectitem.h \
    mapobjectmodel.h \
    mapscene.h \
    mapsdock.h \
    mapview.h \
    minimap.h \
    minimapdock.h \
    movelayer.h \
    movemapobject.h \
    movemapobjecttogroup.h \
    movetileset.h \
    newmapdialog.h \
    newtilesetdialog.h \
    objectgroupitem.h \
    objectsdock.h \
    objectselectionitem.h \
    objectselectiontool.h \
    objecttypes.h \
    objecttypeseditor.h \
    objecttypesmodel.h \
    offsetlayer.h \
    offsetmapdialog.h \
    painttilelayer.h \
    patreondialog.h \
    pluginlistmodel.h \
    preferences.h \
    preferencesdialog.h \
    propertiesdock.h \
    propertybrowser.h \
    raiselowerhelper.h \
    randompicker.h \
    rangeset.h \
    renamelayer.h \
    renameterrain.h \
    replacetileset.h \
    resizedialog.h \
    resizehelper.h \
    resizemap.h \
    resizemapobject.h \
    resizetilelayer.h \
    rotatemapobject.h \
    selectionrectangle.h \
    selectsametiletool.h \
    snaphelper.h \
    sparkleautoupdater.h \
    stampbrush.h \
    standardautoupdater.h \
    stylehelper.h \
    terrainbrush.h \
    terraindock.h \
    terrainmodel.h \
    terrainview.h \
    texteditordialog.h \
    textpropertyedit.h \
    thumbnailrenderer.h \
    tileanimationdriver.h \
    tileanimationeditor.h \
    tilecollisioneditor.h \
    tiledapplication.h \
    tiledproxystyle.h \
    tilelayeritem.h \
    tilepainter.h \
    tileselectionitem.h \
    tileselectiontool.h \
    tilesetchanges.h \
    tilesetdock.h \
    tilesetmanager.h \
    tilesetmodel.h \
    tilesetparametersedit.h \
    tilesetview.h \
    tilestamp.h \
    tilestampmanager.h \
    tilestampmodel.h \
    tilestampsdock.h \
    tmxmapformat.h \
    toolmanager.h \
    undocommands.h \
    undodock.h \
    utils.h \
    varianteditorfactory.h \
    variantpropertymanager.h \
    zoomable.h \
    clickablelabel.h \
    imagecolorpickerwidget.h

FORMS += aboutdialog.ui \
    addpropertydialog.ui \
    commanddialog.ui \
    editterraindialog.ui \
    exportasimagedialog.ui \
    mainwindow.ui \
    newmapdialog.ui \
    newtilesetdialog.ui \
    objecttypeseditor.ui \
    offsetmapdialog.ui \
    patreondialog.ui \
    preferencesdialog.ui \
    resizedialog.ui \
    texteditordialog.ui \
    tileanimationeditor.ui \
    imagecolorpickerwidget.ui

icon32.path = $${PREFIX}/share/icons/hicolor/32x32/apps/
icon32.files += images/32x32/tiled.png
INSTALLS += icon32

icon16.path = $${PREFIX}/share/icons/hicolor/16x16/apps/
icon16.files += images/16x16/tiled.png
INSTALLS += icon16

iconscalable.path = $${PREFIX}/share/icons/hicolor/scalable/apps/
iconscalable.files += images/scalable/tiled.svg
INSTALLS += iconscalable

mimeicon16.path = $${PREFIX}/share/icons/hicolor/16x16/mimetypes/
mimeicon16.files += images/16x16/application-x-tiled.png
INSTALLS += mimeicon16

mimeicon32.path = $${PREFIX}/share/icons/hicolor/32x32/mimetypes/
mimeicon32.files += images/32x32/application-x-tiled.png
INSTALLS += mimeicon32

mimeiconscalable.path = $${PREFIX}/share/icons/hicolor/scalable/mimetypes/
mimeiconscalable.files += images/scalable/application-x-tiled.svg
INSTALLS += mimeiconscalable

mimeinfofile.path = $${PREFIX}/share/mime/packages/
mimeinfofile.files += ../../mime/tiled.xml
INSTALLS += mimeinfofile

thumbnailgenerator.path = $${PREFIX}/share/thumbnailers/
thumbnailgenerator.files += ../../mime/tiled.thumbnailer
INSTALLS += thumbnailgenerator

desktopfile.path = $${PREFIX}/share/applications/
desktopfile.files += ../../tiled.desktop
INSTALLS += desktopfile

manpage.path = $${PREFIX}/share/man/man1/
manpage.files += ../../man/tiled.1
INSTALLS += manpage

RESOURCES += tiled.qrc
macx {
    TARGET = Tiled
    QMAKE_INFO_PLIST = Info.plist
    ICON = images/tiled-icon-mac.icns
}
win32 {
    RC_FILE = tiled.rc.in
    PRECOMPILED_HEADER = pch.h
}
win32:INCLUDEPATH += .
contains(CONFIG, static) {
    DEFINES += STATIC_BUILD
    QTPLUGIN += qgif \
        qjpeg \
        qtiff
}
