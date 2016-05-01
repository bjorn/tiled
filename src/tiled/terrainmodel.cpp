/*
 * terrainmodel.cpp
 * Copyright 2008-2012, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009, Edward Hutchins <eah1@yahoo.com>
 * Copyright 2012, Manu Evans <turkeyman@gmail.com>
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

#include "terrainmodel.h"

#include "containerhelpers.h"
#include "map.h"
#include "mapdocument.h"
#include "renameterrain.h"
#include "terrain.h"
#include "tileset.h"
#include "tile.h"

#include <QApplication>
#include <QFont>
#include <QPalette>

using namespace Tiled;
using namespace Tiled::Internal;

TerrainModel::TerrainModel(MapDocument *mapDocument,
                           QObject *parent):
    QAbstractItemModel(parent),
    mMapDocument(mapDocument)
{
    connect(mapDocument, SIGNAL(tilesetAboutToBeAdded(int)),
            this, SLOT(tilesetAboutToBeAdded(int)));
    connect(mapDocument, SIGNAL(tilesetAdded(int,Tileset*)),
            this, SLOT(tilesetAdded()));
    connect(mapDocument, SIGNAL(tilesetAboutToBeRemoved(int)),
            this, SLOT(tilesetAboutToBeRemoved(int)));
    connect(mapDocument, SIGNAL(tilesetRemoved(Tileset*)),
            this, SLOT(tilesetRemoved()));

    // todo: re-introduce this signal somehow
    connect(mapDocument, SIGNAL(tilesetNameChanged(Tileset*)),
            this, SLOT(tilesetNameChanged(Tileset*)));

    // todo: Re-apply filter to tileset items when terrains are removed from it
    // (it may be their first or last terrain, in which case the tileset item
    // needs to show/hide).
}

TerrainModel::~TerrainModel()
{
}

QModelIndex TerrainModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if (!parent.isValid())
        return createIndex(row, column);
    else if (Tileset *tileset = tilesetAt(parent))
        return createIndex(row, column, tileset);

    return QModelIndex();
}

QModelIndex TerrainModel::index(Tileset *tileset) const
{
    int row = indexOf(mMapDocument->map()->tilesets(), tileset);
    Q_ASSERT(row != -1);
    return createIndex(row, 0);
}

QModelIndex TerrainModel::index(Terrain *terrain) const
{
    Tileset *tileset = terrain->tileset();
    int row = tileset->terrains().indexOf(terrain);
    return createIndex(row, 0, tileset);
}

QModelIndex TerrainModel::parent(const QModelIndex &child) const
{
    if (Terrain *terrain = terrainAt(child))
        return index(terrain->tileset());

    return QModelIndex();
}

int TerrainModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return mMapDocument->map()->tilesetCount();
    else if (Tileset *tileset = tilesetAt(parent))
        return tileset->terrainCount();

    return 0;
}

int TerrainModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant TerrainModel::data(const QModelIndex &index, int role) const
{
    if (Terrain *terrain = terrainAt(index)) {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return terrain->name();
        case Qt::DecorationRole:
            if (Tile *imageTile = terrain->imageTile())
                return imageTile->image();
            break;
        case TerrainRole:
            return QVariant::fromValue(terrain);
        }
    } else if (Tileset *tileset = tilesetAt(index)) {
        switch (role) {
        case Qt::DisplayRole:
            return tileset->name();
        case Qt::SizeHintRole:
            return QSize(1, 32);
        case Qt::FontRole: {
            QFont font = QApplication::font();
            font.setBold(true);
            return font;
        }
        case Qt::BackgroundRole: {
            QColor bg = QApplication::palette().alternateBase().color();
            return bg;//.darker(103);
        }
        }
    }

    return QVariant();
}

Tileset *TerrainModel::tilesetAt(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;
    if (index.parent().isValid()) // tilesets don't have parents
        return nullptr;
    if (index.row() >= mMapDocument->map()->tilesetCount())
        return nullptr;

    return mMapDocument->map()->tilesetAt(index.row()).data();
}

Terrain *TerrainModel::terrainAt(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    if (Tileset *tileset = static_cast<Tileset*>(index.internalPointer()))
        return tileset->terrain(index.row());

    return nullptr;
}

void TerrainModel::tilesetAboutToBeAdded(int index)
{
    beginInsertRows(QModelIndex(), index, index);
}

void TerrainModel::tilesetAdded()
{
    endInsertRows();
}

void TerrainModel::tilesetAboutToBeRemoved(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
}

void TerrainModel::tilesetRemoved()
{
    endRemoveRows();
}

void TerrainModel::tilesetNameChanged(Tileset *tileset)
{
    const QModelIndex index = TerrainModel::index(tileset);
    emit dataChanged(index, index);
}
