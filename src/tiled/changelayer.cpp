/*
 * changelayer.cpp
 * Copyright 2012-2013, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "changelayer.h"

#include "layer.h"
#include "layermodel.h"
#include "map.h"
#include "mapdocument.h"

#include <QCoreApplication>

namespace Tiled {
namespace Internal {

SetLayerVisible::SetLayerVisible(MapDocument *mapDocument,
                                 int layerIndex,
                                 bool visible)
    : mMapDocument(mapDocument)
    , mLayerIndex(layerIndex)
    , mVisible(visible)
{
    if (visible)
        setText(QCoreApplication::translate("Undo Commands",
                                            "Show Layer"));
    else
        setText(QCoreApplication::translate("Undo Commands",
                                            "Hide Layer"));
}

void SetLayerVisible::swap()
{
    const Layer *layer = mMapDocument->map()->layerAt(mLayerIndex);
    const bool previousVisible = layer->isVisible();
    mMapDocument->layerModel()->setLayerVisible(mLayerIndex, mVisible);
    mVisible = previousVisible;
}


SetLayerOpacity::SetLayerOpacity(MapDocument *mapDocument,
                                 int layerIndex,
                                 float opacity)
    : mMapDocument(mapDocument)
    , mLayerIndex(layerIndex)
    , mOldOpacity(mMapDocument->map()->layerAt(layerIndex)->opacity())
    , mNewOpacity(opacity)
{
    setText(QCoreApplication::translate("Undo Commands",
                                        "Change Layer Opacity"));
}

bool SetLayerOpacity::mergeWith(const QUndoCommand *other)
{
    const SetLayerOpacity *o = static_cast<const SetLayerOpacity*>(other);
    if (!(mMapDocument == o->mMapDocument &&
          mLayerIndex == o->mLayerIndex))
        return false;

    mNewOpacity = o->mNewOpacity;
    return true;
}

void SetLayerOpacity::setOpacity(float opacity)
{
    mMapDocument->layerModel()->setLayerOpacity(mLayerIndex, opacity);
}


SetLayerOffset::SetLayerOffset(MapDocument *mapDocument,
                               int layerIndex,
                               const QPointF &offset)
    : mMapDocument(mapDocument)
    , mLayerIndex(layerIndex)
    , mOldOffset(mMapDocument->map()->layerAt(layerIndex)->offset())
    , mNewOffset(offset)
{
    setText(QCoreApplication::translate("Undo Commands",
                                        "Change Layer Offset"));
}

void SetLayerOffset::setOffset(const QPointF &offset)
{
    mMapDocument->layerModel()->setLayerOffset(mLayerIndex, offset);
}

SetLayerTileSize::SetLayerTileSize(MapDocument *mapDocument,
                                   int layerIndex,
                                   const QSize &tileSize)
    : mMapDocument(mapDocument)
    , mLayerIndex(layerIndex)
    , mOldTileSize(mMapDocument->map()->layerAt(layerIndex)->tileSize())
    , mNewTileSize(tileSize)
{
    setText(QCoreApplication::translate("Undo Commands",
                                        "Change Layer TileSize"));
}

void SetLayerTileSize::setTileSize(const QSize &tileSize)
{
    mMapDocument->layerModel()->setLayerTileSize(mLayerIndex, tileSize);
}


} // namespace Internal
} // namespace Tiled
