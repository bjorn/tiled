/*
 * changetileoffset.cpp
 * Copyright 2015, Ryan Gumbs <githubcontrib666@gmail.com>
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

#include "changetileoffset.h"

#include "mapdocument.h"
#include "tile.h"

#include <QCoreApplication>

namespace Tiled {
namespace Internal {

ChangeTileOffset::ChangeTileOffset(MapDocument *mapDocument,
                                             const QList<Tile*>& tiles,
                                             QPoint offset)
    : mMapDocument(mapDocument)
    , mTiles(tiles)
{
    mOffsets.reserve(tiles.size());
    for (int i = 0; i < tiles.size(); ++ i)
        mOffsets.append(offset);

    setText(QCoreApplication::translate("Undo Commands",
                                        "Change Drawing Offset"));
}

ChangeTileOffset::ChangeTileOffset(MapDocument *mapDocument,
                                             const QList<Tile *> &tiles,
                                             const QList<QPoint> &offsets,
                                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , mMapDocument(mapDocument)
    , mTiles(tiles)
    , mOffsets(offsets)
{
    Q_ASSERT(mTiles.size() == mOffsets.size());
    setText(QCoreApplication::translate("Undo Commands",
                                        "Change Drawing Offset"));
}

void ChangeTileOffset::swap()
{
    for (int i = 0; i < mTiles.size(); ++ i) {
        Tile* tile = mTiles[i];
        QPoint offset = tile->offset();
        tile->setOffset(mOffsets[i]);
        mOffsets[i] = offset;
        mMapDocument->emitTileOffsetChanged(tile);
    }
}

} // namespace Internal
} // namespace Tiled

