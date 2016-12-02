/*
 * mapwriter.cpp
 * Copyright 2008-2014, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2010, Jeff Bland <jksb@member.fsf.org>
 * Copyright 2010, Dennis Honeyman <arcticuno@gmail.com>
 *
 * This file is part of libtiled.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mapwriter.h"

#include "compression.h"
#include "gidmapper.h"
#include "map.h"
#include "mapobject.h"
#include "imagelayer.h"
#include "objectgroup.h"
#include "tile.h"
#include "tilelayer.h"
#include "tileset.h"
#include "terrain.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QSaveFile>
#include <QXmlStreamWriter>

using namespace Tiled;
using namespace Tiled::Internal;

static QString colorToString(const QColor &color)
{
    if (color.alpha() != 255)
        return color.name(QColor::HexArgb);
    return color.name();
}

namespace Tiled {
namespace Internal {

class MapWriterPrivate
{
    Q_DECLARE_TR_FUNCTIONS(MapReader)

public:
    MapWriterPrivate();

    void writeMap(const Map *map, QIODevice *device,
                  const QString &path);

    void writeTileset(const Tileset &tileset, QIODevice *device,
                      const QString &path);

    bool openFile(QIODevice *file);

    QString mError;
    Map::LayerDataFormat mLayerDataFormat;
    bool mDtdEnabled;

private:
    void writeMap(QXmlStreamWriter &w, const Map &map);
    void writeTileset(QXmlStreamWriter &w, const Tileset &tileset,
                      unsigned firstGid);
    void writeTileLayer(QXmlStreamWriter &w, const TileLayer &tileLayer);
    void writeLayerAttributes(QXmlStreamWriter &w, const Layer &layer);
    void writeObjectGroup(QXmlStreamWriter &w, const ObjectGroup &objectGroup);
    void writeObject(QXmlStreamWriter &w, const MapObject &mapObject);
    void writeImageLayer(QXmlStreamWriter &w, const ImageLayer &imageLayer);
    void writeProperties(QXmlStreamWriter &w,
                         const Properties &properties);

    QDir mMapDir;     // The directory in which the map is being saved
    GidMapper mGidMapper;
    bool mUseAbsolutePaths;
};

} // namespace Internal
} // namespace Tiled


MapWriterPrivate::MapWriterPrivate()
    : mLayerDataFormat(Map::Base64Zlib)
    , mDtdEnabled(false)
    , mUseAbsolutePaths(false)
{
}

bool MapWriterPrivate::openFile(QIODevice *file)
{
    if (!file->open(QIODevice::WriteOnly | QIODevice::Text)) {
        mError = tr("Could not open file for writing.");
        return false;
    }

    return true;
}

static QXmlStreamWriter *createWriter(QIODevice *device)
{
    QXmlStreamWriter *writer = new QXmlStreamWriter(device);
    writer->setAutoFormatting(true);
    writer->setAutoFormattingIndent(1);
    return writer;
}

void MapWriterPrivate::writeMap(const Map *map, QIODevice *device,
                                const QString &path)
{
    mMapDir = QDir(path);
    mUseAbsolutePaths = path.isEmpty();
    mLayerDataFormat = map->layerDataFormat();

    QXmlStreamWriter *writer = createWriter(device);
    writer->writeStartDocument();

    if (mDtdEnabled) {
        writer->writeDTD(QLatin1String("<!DOCTYPE map SYSTEM \""
                                       "http://mapeditor.org/dtd/1.0/"
                                       "map.dtd\">"));
    }

    writeMap(*writer, *map);
    writer->writeEndDocument();
    delete writer;
}

void MapWriterPrivate::writeTileset(const Tileset &tileset, QIODevice *device,
                                    const QString &path)
{
    mMapDir = QDir(path);
    mUseAbsolutePaths = path.isEmpty();

    QXmlStreamWriter *writer = createWriter(device);
    writer->writeStartDocument();

    if (mDtdEnabled) {
        writer->writeDTD(QLatin1String("<!DOCTYPE tileset SYSTEM \""
                                       "http://mapeditor.org/dtd/1.0/"
                                       "map.dtd\">"));
    }

    writeTileset(*writer, tileset, 0);
    writer->writeEndDocument();
    delete writer;
}

void MapWriterPrivate::writeMap(QXmlStreamWriter &w, const Map &map)
{
    w.writeStartElement(QLatin1String("map"));

    const QString orientation = orientationToString(map.orientation());
    const QString renderOrder = renderOrderToString(map.renderOrder());

    w.writeAttribute(QLatin1String("version"), QLatin1String("1.0"));
    w.writeAttribute(QLatin1String("orientation"), orientation);
    w.writeAttribute(QLatin1String("renderorder"), renderOrder);
    w.writeAttribute(QLatin1String("width"), QString::number(map.width()));
    w.writeAttribute(QLatin1String("height"), QString::number(map.height()));
    w.writeAttribute(QLatin1String("tilewidth"),
                     QString::number(map.tileWidth()));
    w.writeAttribute(QLatin1String("tileheight"),
                     QString::number(map.tileHeight()));

    if (map.orientation() == Map::Hexagonal) {
        w.writeAttribute(QLatin1String("hexsidelength"),
                         QString::number(map.hexSideLength()));
    }

    if (map.orientation() == Map::Staggered || map.orientation() == Map::Hexagonal) {
        w.writeAttribute(QLatin1String("staggeraxis"),
                         staggerAxisToString(map.staggerAxis()));
        w.writeAttribute(QLatin1String("staggerindex"),
                         staggerIndexToString(map.staggerIndex()));
    }

    if (map.backgroundColor().isValid()) {
        w.writeAttribute(QLatin1String("backgroundcolor"),
                         colorToString(map.backgroundColor()));
    }

    w.writeAttribute(QLatin1String("nextobjectid"),
                     QString::number(map.nextObjectId()));

    writeProperties(w, map.properties());

    mGidMapper.clear();
    unsigned firstGid = 1;
    for (const SharedTileset &tileset : map.tilesets()) {
        writeTileset(w, *tileset, firstGid);
        mGidMapper.insert(firstGid, tileset.data());
        firstGid += tileset->nextTileId();
    }

    for (const Layer *layer : map.layers()) {
        const Layer::TypeFlag type = layer->layerType();
        if (type == Layer::TileLayerType)
            writeTileLayer(w, *static_cast<const TileLayer*>(layer));
        else if (type == Layer::ObjectGroupType)
            writeObjectGroup(w, *static_cast<const ObjectGroup*>(layer));
        else if (type == Layer::ImageLayerType)
            writeImageLayer(w, *static_cast<const ImageLayer*>(layer));
    }

    w.writeEndElement();
}

static QString makeTerrainAttribute(const Tile *tile)
{
    QString terrain;
    for (int i = 0; i < 4; ++i ) {
        if (i > 0)
            terrain += QLatin1String(",");
        int t = tile->cornerTerrainId(i);
        if (t > -1)
            terrain += QString::number(t);
    }
    return terrain;
}

void MapWriterPrivate::writeTileset(QXmlStreamWriter &w, const Tileset &tileset,
                                    unsigned firstGid)
{
    w.writeStartElement(QLatin1String("tileset"));
    if (firstGid > 0)
        w.writeAttribute(QLatin1String("firstgid"), QString::number(firstGid));

    const QString &fileName = tileset.fileName();
    if (!fileName.isEmpty()) {
        QString source = fileName;
        if (!mUseAbsolutePaths)
            source = mMapDir.relativeFilePath(source);
        w.writeAttribute(QLatin1String("source"), source);

        // Tileset is external, so no need to write any of the stuff below
        w.writeEndElement();
        return;
    }

    w.writeAttribute(QLatin1String("name"), tileset.name());
    w.writeAttribute(QLatin1String("tilewidth"),
                     QString::number(tileset.tileWidth()));
    w.writeAttribute(QLatin1String("tileheight"),
                     QString::number(tileset.tileHeight()));
    const int tileSpacing = tileset.tileSpacing();
    const int margin = tileset.margin();
    if (tileSpacing != 0)
        w.writeAttribute(QLatin1String("spacing"),
                         QString::number(tileSpacing));
    if (margin != 0)
        w.writeAttribute(QLatin1String("margin"), QString::number(margin));

    w.writeAttribute(QLatin1String("tilecount"),
                     QString::number(tileset.tileCount()));
    w.writeAttribute(QLatin1String("columns"),
                     QString::number(tileset.columnCount()));

    const QPoint offset = tileset.tileOffset();
    if (!offset.isNull()) {
        w.writeStartElement(QLatin1String("tileoffset"));
        w.writeAttribute(QLatin1String("x"), QString::number(offset.x()));
        w.writeAttribute(QLatin1String("y"), QString::number(offset.y()));
        w.writeEndElement();
    }

    // Write the tileset properties
    writeProperties(w, tileset.properties());

    // Write the image element
    const QString &imageSource = tileset.imageSource();
    if (!imageSource.isEmpty()) {
        w.writeStartElement(QLatin1String("image"));
        QString source = imageSource;
        if (!mUseAbsolutePaths)
            source = mMapDir.relativeFilePath(source);
        w.writeAttribute(QLatin1String("source"), source);

        const QColor transColor = tileset.transparentColor();
        if (transColor.isValid())
            w.writeAttribute(QLatin1String("trans"), transColor.name().mid(1));

        if (tileset.imageWidth() > 0)
            w.writeAttribute(QLatin1String("width"),
                             QString::number(tileset.imageWidth()));
        if (tileset.imageHeight() > 0)
            w.writeAttribute(QLatin1String("height"),
                             QString::number(tileset.imageHeight()));

        w.writeEndElement();
    }

    // Write the terrain types
    if (tileset.terrainCount() > 0) {
        w.writeStartElement(QLatin1String("terraintypes"));
        for (int i = 0; i < tileset.terrainCount(); ++i) {
            const Terrain *t = tileset.terrain(i);
            w.writeStartElement(QLatin1String("terrain"));

            w.writeAttribute(QLatin1String("name"), t->name());
            w.writeAttribute(QLatin1String("tile"), QString::number(t->imageTileId()));

            writeProperties(w, t->properties());

            w.writeEndElement();
        }
        w.writeEndElement();
    }

    // Write the properties for those tiles that have them
    for (const Tile *tile : tileset.tiles()) {
        const Properties properties = tile->properties();
        unsigned terrain = tile->terrain();
        float probability = tile->probability();
        ObjectGroup *objectGroup = tile->objectGroup();

        if (!properties.isEmpty() || terrain != 0xFFFFFFFF || probability != 1.f || imageSource.isEmpty() || objectGroup || tile->isAnimated()) {
            w.writeStartElement(QLatin1String("tile"));
            w.writeAttribute(QLatin1String("id"), QString::number(tile->id()));
            w.writeAttribute(QLatin1String("order"), QString::number(tileset.tileOrder(tile)));
                w.writeAttribute(QLatin1String("terrain"), makeTerrainAttribute(tile));
            if (probability != 1.f)
                w.writeAttribute(QLatin1String("probability"), QString::number(probability));
            if (!properties.isEmpty())
                writeProperties(w, properties);
            if (imageSource.isEmpty()) {
                w.writeStartElement(QLatin1String("image"));

                const QSize tileSize = tile->size();
                if (!tileSize.isNull()) {
                    w.writeAttribute(QLatin1String("width"),
                                     QString::number(tileSize.width()));
                    w.writeAttribute(QLatin1String("height"),
                                     QString::number(tileSize.height()));
                }

                if (tile->imageSource().isEmpty()) {
                    w.writeAttribute(QLatin1String("format"),
                                     QLatin1String("png"));

                    w.writeStartElement(QLatin1String("data"));
                    w.writeAttribute(QLatin1String("encoding"),
                                     QLatin1String("base64"));

                    QBuffer buffer;
                    tile->image().save(&buffer, "png");
                    w.writeCharacters(QString::fromLatin1(buffer.data().toBase64()));
                    w.writeEndElement(); // </data>
                } else {
                    QString source = tile->imageSource();
                    if (!mUseAbsolutePaths)
                        source = mMapDir.relativeFilePath(source);
                    w.writeAttribute(QLatin1String("source"), source);
                }

                w.writeEndElement(); // </image>
            }
            if (objectGroup)
                writeObjectGroup(w, *objectGroup);
            if (tile->isAnimated()) {
                const QVector<Frame> &frames = tile->frames();

                w.writeStartElement(QLatin1String("animation"));
                for (const Frame &frame : frames) {
                    w.writeStartElement(QLatin1String("frame"));
                    w.writeAttribute(QLatin1String("tileid"), QString::number(frame.tileId));
                    w.writeAttribute(QLatin1String("duration"), QString::number(frame.duration));
                    w.writeEndElement(); // </frame>
                }
                w.writeEndElement(); // </animation>
            }

            w.writeEndElement(); // </tile>
        }
    }

    w.writeEndElement();
}

void MapWriterPrivate::writeTileLayer(QXmlStreamWriter &w,
                                      const TileLayer &tileLayer)
{
    w.writeStartElement(QLatin1String("layer"));
    writeLayerAttributes(w, tileLayer);
    writeProperties(w, tileLayer.properties());

    QString encoding;
    QString compression;

    if (mLayerDataFormat == Map::Base64
            || mLayerDataFormat == Map::Base64Gzip
            || mLayerDataFormat == Map::Base64Zlib) {

        encoding = QLatin1String("base64");

        if (mLayerDataFormat == Map::Base64Gzip)
            compression = QLatin1String("gzip");
        else if (mLayerDataFormat == Map::Base64Zlib)
            compression = QLatin1String("zlib");

    } else if (mLayerDataFormat == Map::CSV)
        encoding = QLatin1String("csv");

    w.writeStartElement(QLatin1String("data"));
    if (!encoding.isEmpty())
        w.writeAttribute(QLatin1String("encoding"), encoding);
    if (!compression.isEmpty())
        w.writeAttribute(QLatin1String("compression"), compression);

    if (mLayerDataFormat == Map::XML) {
        for (int y = 0; y < tileLayer.height(); ++y) {
            for (int x = 0; x < tileLayer.width(); ++x) {
                const unsigned gid = mGidMapper.cellToGid(tileLayer.cellAt(x, y));
                w.writeStartElement(QLatin1String("tile"));
                w.writeAttribute(QLatin1String("gid"), QString::number(gid));
                w.writeEndElement();
            }
        }
    } else if (mLayerDataFormat == Map::CSV) {
        QString tileData;

        for (int y = 0; y < tileLayer.height(); ++y) {
            for (int x = 0; x < tileLayer.width(); ++x) {
                const unsigned gid = mGidMapper.cellToGid(tileLayer.cellAt(x, y));
                tileData.append(QString::number(gid));
                if (x != tileLayer.width() - 1
                    || y != tileLayer.height() - 1)
                    tileData.append(QLatin1String(","));
            }
            tileData.append(QLatin1String("\n"));
        }

        w.writeCharacters(QLatin1String("\n"));
        w.writeCharacters(tileData);
    } else {
        QByteArray tileData = mGidMapper.encodeLayerData(tileLayer,
                                                         mLayerDataFormat);

        w.writeCharacters(QLatin1String("\n   "));
        w.writeCharacters(QString::fromLatin1(tileData));
        w.writeCharacters(QLatin1String("\n  "));
    }

    w.writeEndElement(); // </data>
    w.writeEndElement(); // </layer>
}

void MapWriterPrivate::writeLayerAttributes(QXmlStreamWriter &w,
                                            const Layer &layer)
{
    if (!layer.name().isEmpty())
        w.writeAttribute(QLatin1String("name"), layer.name());

    const int x = layer.x();
    const int y = layer.y();
    const qreal opacity = layer.opacity();
    if (x != 0)
        w.writeAttribute(QLatin1String("x"), QString::number(x));
    if (y != 0)
        w.writeAttribute(QLatin1String("y"), QString::number(y));

    if (layer.layerType() == Layer::TileLayerType) {
        w.writeAttribute(QLatin1String("width"),
                         QString::number(layer.width()));
        w.writeAttribute(QLatin1String("height"),
                         QString::number(layer.height()));
    }

    if (!layer.isVisible())
        w.writeAttribute(QLatin1String("visible"), QLatin1String("0"));
    if (opacity != qreal(1))
        w.writeAttribute(QLatin1String("opacity"), QString::number(opacity));

    const QPointF offset = layer.offset();
    if (!offset.isNull()) {
        w.writeAttribute(QLatin1String("offsetx"), QString::number(offset.x()));
        w.writeAttribute(QLatin1String("offsety"), QString::number(offset.y()));
    }
}

void MapWriterPrivate::writeObjectGroup(QXmlStreamWriter &w,
                                        const ObjectGroup &objectGroup)
{
    w.writeStartElement(QLatin1String("objectgroup"));

    if (objectGroup.color().isValid())
        w.writeAttribute(QLatin1String("color"),
                         colorToString(objectGroup.color()));

    if (objectGroup.drawOrder() != ObjectGroup::TopDownOrder) {
        w.writeAttribute(QLatin1String("draworder"),
                         drawOrderToString(objectGroup.drawOrder()));
    }

    writeLayerAttributes(w, objectGroup);
    writeProperties(w, objectGroup.properties());

    for (const MapObject *mapObject : objectGroup.objects())
        writeObject(w, *mapObject);

    w.writeEndElement();
}

void MapWriterPrivate::writeObject(QXmlStreamWriter &w,
                                   const MapObject &mapObject)
{
    w.writeStartElement(QLatin1String("object"));
    w.writeAttribute(QLatin1String("id"), QString::number(mapObject.id()));
    const QString &name = mapObject.name();
    const QString &type = mapObject.type();
    if (!name.isEmpty())
        w.writeAttribute(QLatin1String("name"), name);
    if (!type.isEmpty())
        w.writeAttribute(QLatin1String("type"), type);

    if (!mapObject.cell().isEmpty()) {
        const unsigned gid = mGidMapper.cellToGid(mapObject.cell());
        w.writeAttribute(QLatin1String("gid"), QString::number(gid));
    }

    const QPointF pos = mapObject.position();
    const QSizeF size = mapObject.size();

    w.writeAttribute(QLatin1String("x"), QString::number(pos.x()));
    w.writeAttribute(QLatin1String("y"), QString::number(pos.y()));

    if (size.width() != 0)
        w.writeAttribute(QLatin1String("width"), QString::number(size.width()));
    if (size.height() != 0)
        w.writeAttribute(QLatin1String("height"), QString::number(size.height()));

    const qreal rotation = mapObject.rotation();
    if (rotation != 0.0)
        w.writeAttribute(QLatin1String("rotation"), QString::number(rotation));

    if (!mapObject.isVisible())
        w.writeAttribute(QLatin1String("visible"), QLatin1String("0"));

    writeProperties(w, mapObject.properties());

    const QPolygonF &polygon = mapObject.polygon();
    if (!polygon.isEmpty()) {
        if (mapObject.shape() == MapObject::Polygon)
            w.writeStartElement(QLatin1String("polygon"));
        else
            w.writeStartElement(QLatin1String("polyline"));

        QString points;
        for (const QPointF &point : polygon) {
            points.append(QString::number(point.x()));
            points.append(QLatin1Char(','));
            points.append(QString::number(point.y()));
            points.append(QLatin1Char(' '));
        }
        points.chop(1);
        w.writeAttribute(QLatin1String("points"), points);
        w.writeEndElement();
    }

    if (mapObject.shape() == MapObject::Ellipse)
        w.writeEmptyElement(QLatin1String("ellipse"));

    w.writeEndElement();
}

void MapWriterPrivate::writeImageLayer(QXmlStreamWriter &w,
                                       const ImageLayer &imageLayer)
{
    w.writeStartElement(QLatin1String("imagelayer"));
    writeLayerAttributes(w, imageLayer);

    // Write the image element
    const QString &imageSource = imageLayer.imageSource();
    if (!imageSource.isEmpty()) {
        w.writeStartElement(QLatin1String("image"));
        QString source = imageSource;
        if (!mUseAbsolutePaths)
            source = mMapDir.relativeFilePath(source);
        w.writeAttribute(QLatin1String("source"), source);

        const QColor transColor = imageLayer.transparentColor();
        if (transColor.isValid())
            w.writeAttribute(QLatin1String("trans"), transColor.name().mid(1));

        const QSize imageSize = imageLayer.image().size();
        if (!imageSize.isNull()) {
            w.writeAttribute(QLatin1String("width"),
                             QString::number(imageSize.width()));
            w.writeAttribute(QLatin1String("height"),
                             QString::number(imageSize.height()));
        }

        w.writeEndElement();
    }

    writeProperties(w, imageLayer.properties());

    w.writeEndElement();
}

void MapWriterPrivate::writeProperties(QXmlStreamWriter &w,
                                       const Properties &properties)
{
    if (properties.isEmpty())
        return;

    w.writeStartElement(QLatin1String("properties"));

    Properties::const_iterator it = properties.constBegin();
    Properties::const_iterator it_end = properties.constEnd();
    for (; it != it_end; ++it) {
        w.writeStartElement(QLatin1String("property"));
        w.writeAttribute(QLatin1String("name"), it.key());

        int type = it.value().userType();
        QString typeName = typeToName(type);
        if (typeName != QLatin1String("string"))
            w.writeAttribute(QLatin1String("type"), typeName);

        QString value = toExportValue(it.value()).toString();

        if (type == filePathTypeId() && !mUseAbsolutePaths)
            value = mMapDir.relativeFilePath(value);

        if (value.contains(QLatin1Char('\n')))
            w.writeCharacters(value);
        else
            w.writeAttribute(QLatin1String("value"), value);

        w.writeEndElement();
    }

    w.writeEndElement();
}


MapWriter::MapWriter()
    : d(new MapWriterPrivate)
{
}

MapWriter::~MapWriter()
{
    delete d;
}

void MapWriter::writeMap(const Map *map, QIODevice *device,
                         const QString &path)
{
    d->writeMap(map, device, path);
}

bool MapWriter::writeMap(const Map *map, const QString &fileName)
{
    QSaveFile file(fileName);
    if (!d->openFile(&file))
        return false;

    writeMap(map, &file, QFileInfo(fileName).absolutePath());

    if (file.error() != QFile::NoError) {
        d->mError = file.errorString();
        return false;
    }

    if (!file.commit()) {
        d->mError = file.errorString();
        return false;
    }

    return true;
}

void MapWriter::writeTileset(const Tileset &tileset, QIODevice *device,
                             const QString &path)
{
    d->writeTileset(tileset, device, path);
}

bool MapWriter::writeTileset(const Tileset &tileset, const QString &fileName)
{
    QFile file(fileName);
    if (!d->openFile(&file))
        return false;

    writeTileset(tileset, &file, QFileInfo(fileName).absolutePath());

    if (file.error() != QFile::NoError) {
        d->mError = file.errorString();
        return false;
    }

    return true;
}

QString MapWriter::errorString() const
{
    return d->mError;
}

void MapWriter::setDtdEnabled(bool enabled)
{
    d->mDtdEnabled = enabled;
}

bool MapWriter::isDtdEnabled() const
{
    return d->mDtdEnabled;
}
