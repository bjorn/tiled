/*
 * GMX Tiled Plugin
 * Copyright 2016, Jones Blunt <mrjonesblunt@gmail.com>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gmxplugin.h"

#include "map.h"
#include "tile.h"
#include "tilelayer.h"
#include "mapobject.h"
#include "objectgroup.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamWriter>

using namespace Tiled;
using namespace Gmx;

GmxPlugin::GmxPlugin()
{
}

bool GmxPlugin::write(const Map *map, const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        mError = tr("Could not open file for writing.");
        return false;
    }

    QXmlStreamWriter stream(&file);

    stream.setAutoFormatting(true);
    stream.setAutoFormattingIndent(2);

    stream.writeComment("This Document is generated by GameMaker, if you edit it by hand then you do so at your own risk!");

    stream.writeStartElement("room");
    stream.writeStartElement("caption");
    stream.writeEndElement();

    stream.writeTextElement("width", QString::number(map->tileWidth() * map->width()));

    stream.writeTextElement("height", QString::number(map->tileHeight() * map->height()));

    stream.writeTextElement("vsnap", QString::number(map->tileHeight()));
    stream.writeTextElement("hsnap", QString::number(map->tileWidth()));

    stream.writeTextElement("isometric", QString::number((map->orientation() == Map::Orientation::Isometric ? 1 : 0)));

    stream.writeStartElement("instances");

    uint objectId = 0u;
    foreach (const Layer *layer, map->layers()) {

        if (layer->layerType() != Layer::ObjectGroupType) continue;
        const ObjectGroup *objectLayer = static_cast<const ObjectGroup*>(layer);

        foreach(const MapObject *object, objectLayer->objects()) {
            stream.writeStartElement("instance");

            stream.writeAttribute("objName", object->name());
            stream.writeAttribute("x", QString::number((int)(object->x())));
            stream.writeAttribute("y", QString::number((int)(object->y() - object->height())));

            stream.writeAttribute("id", QString::number(++objectId));

            stream.writeAttribute("locked", QString::number(0));
            stream.writeAttribute("scaleX", QString::number(1));
            stream.writeAttribute("scaleY", QString::number(1));

            stream.writeEndElement();
        }

        if (file.error() != QFile::NoError) {
            mError = file.errorString();
            return false;
        }

    }

    stream.writeEndElement();


    stream.writeStartElement("tiles");



    uint tileId = 0u;
    int currentLayer = map->layers().size();

    foreach (const Layer *layer, map->layers()) {

        if (layer->layerType() != Layer::TileLayerType) continue;

        const TileLayer *tileLayer = static_cast<const TileLayer*>(layer);
        for (int y = 0; y < tileLayer->height(); ++y) {
            for (int x = 0; x < tileLayer->width(); ++x) {
                const Cell &cell = tileLayer->cellAt(x, y);
                const Tile *tile = cell.tile;

                if(tile) {
                    stream.writeStartElement("tile");

                    stream.writeAttribute("bgName", tile->tileset()->name());
                    stream.writeAttribute("x", QString::number((int)(x * map->tileWidth())));
                    stream.writeAttribute("y", QString::number((int)(y * map->tileHeight())));
                    stream.writeAttribute("w", QString::number(map->tileWidth()));
                    stream.writeAttribute("h", QString::number(map->tileHeight()));

                    stream.writeAttribute("xo", QString::number(tile->id() % tile->tileset()->columnCount() * tile->tileset()->tileWidth()));

                    stream.writeAttribute("yo", QString::number((int)(tile->id() / tile->tileset()->columnCount()) * tile->tileset()->tileWidth()));

                    stream.writeAttribute("depth", QString::number(layer->hasProperty(QLatin1String("Depth")) ? layer->property(QLatin1String("Depth")).toInt() : currentLayer));
                    stream.writeAttribute("id", QString::number(++tileId));
                    stream.writeAttribute("scaleX", QString::number(1));
                    stream.writeAttribute("scaleY", QString::number(1));

                    stream.writeEndElement();
                }
            }
        }

        currentLayer--;

        if (file.error() != QFile::NoError) {
            mError = file.errorString();
            return false;
        }
    }

    stream.writeEndElement();
    stream.writeEndDocument();

    file.close();
    return true;
}

QString GmxPlugin::errorString() const
{
    return mError;
}

QStringList GmxPlugin::outputFiles(const Tiled::Map *map, const QString &fileName) const
{
    QStringList result;

    QFileInfo fileInfo(fileName);
    const QString base = fileInfo.completeBaseName();
    const QString path = fileInfo.path();

    const QString completeName = base + QLatin1String(".room.gmx");
    const QString completePath = QDir(path).filePath(completeName);

    result.append(completePath);

    if (result.size() == 1)
        result[0] = fileName;

    return result;
}

QString GmxPlugin::nameFilter() const
{
    return tr("GameMaker room files (*.room.gmx)");
}
