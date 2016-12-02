/*
 * tilesetmodel.cpp
 * Copyright 2008-2009, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009, Edward Hutchins <eah1@yahoo.com>
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

#include "tilesetmodel.h"

#include "map.h"
#include "tile.h"
#include "tiled.h"
#include "tileset.h"

#include <QMimeData>

using namespace Tiled;
using namespace Tiled::Internal;

TilesetModel::TilesetModel(Tileset *tileset, QObject *parent):
    QAbstractListModel(parent),
    mTileset(tileset)
{
    refreshTileIds();
}

int TilesetModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    const int tileCount = mTileIds.size();
    const int columns = columnCount();

    int rows = 1;
    if (columns > 0) {
        rows = tileCount / columns;
        if (tileCount % columns > 0)
            ++rows;
    }

    return rows;
}

int TilesetModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    if (mTileset->columnCount())
        return mTileset->columnCount();
    // TODO: Non-table tilesets should use a different model.
    // For now use an arbitrary number of columns.
    return 5;
}

QVariant TilesetModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DecorationRole) {
        if (Tile *tile = tileAt(index))
            return tile->image();
    } else if (role == TerrainRole) {
        if (Tile *tile = tileAt(index))
            return tile->terrain();
    }

    return QVariant();
}

QVariant TilesetModel::headerData(int /* section */,
                                  Qt::Orientation /* orientation */,
                                  int role) const
{
    if (role == Qt::SizeHintRole)
        return QSize(1, 1);
    return QVariant();
}

Qt::ItemFlags TilesetModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);

    if (index.isValid())
        defaultFlags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

    return defaultFlags;
}

QStringList TilesetModel::mimeTypes() const
{
    QStringList types;
    types << QLatin1String(TILES_MIMETYPE);
    return types;
}

QMimeData *TilesetModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData;
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    for (const QModelIndex &index : indexes) {
        if (index.isValid())
            stream << tileIdAt(index);
    }

    mimeData->setData(QLatin1String(TILES_MIMETYPE), encodedData);
    return mimeData;
}

Tile *TilesetModel::tileAt(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    const int tileIndex = index.column() + index.row() * columnCount();

    if (tileIndex < mTileIds.size()) {
        const int tileId = mTileIds.at(tileIndex);
        return mTileset->findTile(tileId);
    }

    return nullptr;
}

int TilesetModel::tileIdAt(const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());
    const int tileIndex = index.column() + index.row() * columnCount();
    return mTileIds.at(tileIndex);
}

QModelIndex TilesetModel::tileIndex(const Tile *tile) const
{
    Q_ASSERT(tile->tileset() == mTileset);

    const int columnCount = TilesetModel::columnCount();
    const int tileIndex = mTileIds.indexOf(tile->id());
    Q_ASSERT(tileIndex != -1);

    const int row = tileIndex / columnCount;
    const int column = tileIndex % columnCount;

    return index(row, column);
}

void TilesetModel::setTileset(Tileset *tileset)
{
    if (mTileset == tileset)
        return;

    beginResetModel();

    mTileset = tileset;
    refreshTileIds();

    endResetModel();
}

void TilesetModel::tilesetChanged()
{
    beginResetModel();
    refreshTileIds();
    endResetModel();
}

void TilesetModel::resetModel()
{
    beginResetModel();
    endResetModel();
}

void TilesetModel::tilesChanged(const QList<Tile *> &tiles)
{
    if (tiles.first()->tileset() != mTileset)
        return;

    QModelIndex topLeft;
    QModelIndex bottomRight;

    for (const Tile *tile : tiles) {
        const QModelIndex i = tileIndex(tile);

        if (!topLeft.isValid()) {
            topLeft = i;
            bottomRight = i;
            continue;
        }

        if (i.row() < topLeft.row() || i.column() < topLeft.column())
            topLeft = index(qMin(topLeft.row(), i.row()),
                            qMin(topLeft.column(), i.column()));

        if (i.row() > bottomRight.row() || i.column() > bottomRight.column())
            bottomRight = index(qMax(bottomRight.row(), i.row()),
                                qMax(bottomRight.column(), i.column()));
    }

    if (topLeft.isValid())
        emit dataChanged(topLeft, bottomRight);
}

void TilesetModel::tileChanged(Tile *tile)
{
    if (tile->tileset() != mTileset)
        return;

    const QModelIndex i = tileIndex(tile);
    emit dataChanged(i, i);
}

void TilesetModel::refreshTileIds()
{
    mTileIds.clear();

    for (Tile *tile : mTileset->orderedTiles())
        mTileIds.append(tile->id());
}

Qt::DropActions TilesetModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

bool TilesetModel::removeRows(int row, int count, const QModelIndex &parent)
{
    (void)row;
    (void)count;
    (void)parent;
    return true;
}
