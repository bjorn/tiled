/*
 * changetileorder.h
 * Copyright 2012, Ryan Gumbs <githubcontrib666@gmail.com>
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

#ifndef CHANGETILEORDER_H
#define CHANGETILEORDER_H

#include <QMap>
#include <QUndoCommand>

#include "undocommands.h"
#include "tilesetview.h"

namespace Tiled {

class Tile;
class Tileset;

namespace Internal {

class MapDocument;

class ChangeTileOrder : public QUndoCommand
{
public:
    struct Change {
        Change(unsigned from, unsigned to)
            : from(from), to(to)
        {}

        unsigned from;
        unsigned to;
    };

    typedef QMap<int, Tile*> Changes;

    /**
     * Constructs an empty command that changes no terrain. When merged into
     * a previous terrain change command, it prevents that command from merging
     * with future commands.
     */
    ChangeTileOrder(TilesetView *view);

    void undo() override { swap(); }
    void redo() override { swap(); }

    int id() const override { return Cmd_ChangeTileOrder; }

private:
    void initText();
    void swap();

    QMap <int, Tile*> mPrevOrder;
    TilesetView *mView;
    bool mMergeable = false;
};

} // namespace Internal
} // namespace Tiled

#endif // CHANGETILEORDER_H
