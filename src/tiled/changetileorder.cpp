/*
 * changetileorder.cpp
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

#include "changetileorder.h"

#include "mapdocument.h"
#include "tile.h"

#include <QCoreApplication>

namespace Tiled {
namespace Internal {

ChangeTileOrder::ChangeTileOrder(TilesetView *view)
    : mView(view)
{
    initText();
    mPrevOrder= view->tilesetModel()->tileset()->orderedTiles();
}

void ChangeTileOrder::swap()
{
    TilesetModel *mModel= mView->tilesetModel();

    if (!mModel)
        return;

    if (!mModel->tileset())
        return;

    QMap <int, Tile*> prevOrder= mModel->tileset()->orderedTiles();
    printf ("orderedTiles\n"); fflush(stdout);
    mModel->tileset()->setOrderedTileset(mPrevOrder);
    mModel->tilesetChanged();
    printf ("setOrderedTiles\n"); fflush(stdout);
    mPrevOrder= prevOrder;
  }

void ChangeTileOrder::initText()
{
    setText(QCoreApplication::translate("Undo Commands",
                                        "Change Tile Order"));
}

} // namespace Internal
} // namespace Tiled
