/*
 * ChangeTileOffset.h
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

#ifndef CHANGETILEOFFSET_H
#define CHANGETILEOFFSET_H

#include <QUndoCommand>

namespace Tiled {

class Tile;

namespace Internal {

class MapDocument;

class ChangeTileOffset : public QUndoCommand
{
public:
    ChangeTileOffset(MapDocument *mapDocument,
                          const QList<Tile*> &tiles,
                          QPoint offset);

    ChangeTileOffset(MapDocument *mapDocument,
                          const QList<Tile*> &tiles,
                          const QList<QPoint> &offsets,
                          QUndoCommand *parent = nullptr);

    void undo() override { swap(); }
    void redo() override { swap(); }

private:
    void swap();

    MapDocument *mMapDocument;
    QList<Tile*> mTiles;
    QList<QPoint> mOffsets;
};

} // namespace Internal
} // namespace Tiled

#endif // CHANGETILEOFFSET_H
