/*
 * abstracttiletool.cpp
 * Copyright 2009-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "abstracttiletool.h"

#include "brushitem.h"
#include "map.h"
#include "mapdocument.h"
#include "maprenderer.h"
#include "mapscene.h"
#include "tile.h"
#include "tilelayer.h"
#include "tilestamp.h"
#include "invertyaxishelper.h"

#include <QtMath>

using namespace Tiled;

AbstractTileTool::AbstractTileTool(const QString &name,
                                   const QIcon &icon,
                                   const QKeySequence &shortcut,
                                   BrushItem *brushItem,
                                   QObject *parent)
    : AbstractTool(name, icon, shortcut, parent)
    , mTilePositionMethod(OnTiles)
    , mBrushItem(brushItem)
    , mBrushVisible(false)
{
    if (!mBrushItem)
        mBrushItem = new BrushItem;
    mBrushItem->setVisible(false);
    mBrushItem->setZValue(10000);
}

AbstractTileTool::~AbstractTileTool()
{
    delete mBrushItem;
}

void AbstractTileTool::activate(MapScene *scene)
{
    scene->addItem(mBrushItem);
}

void AbstractTileTool::deactivate(MapScene *scene)
{
    scene->removeItem(mBrushItem);
}

void AbstractTileTool::mouseEntered()
{
    setBrushVisible(true);
}

void AbstractTileTool::mouseLeft()
{
    setBrushVisible(false);
}

void AbstractTileTool::mouseMoved(const QPointF &pos, Qt::KeyboardModifiers)
{
    // Take into account the offset of the current layer
    QPointF offsetPos = pos;
    if (Layer *layer = currentLayer()) {
        offsetPos -= layer->totalOffset();
        mBrushItem->setLayerOffset(layer->totalOffset());
    }

    const MapRenderer *renderer = mapDocument()->renderer();
    const QPointF tilePosF = renderer->screenToTileCoords(offsetPos);
    QPoint tilePos;

    if (mTilePositionMethod == BetweenTiles)
        tilePos = tilePosF.toPoint();
    else
        tilePos = QPoint(qFloor(tilePosF.x()),
                         qFloor(tilePosF.y()));

    if (mTilePosition != tilePos) {
        mTilePosition = tilePos;

        tilePositionChanged(tilePos);
        updateStatusInfo();
    }
}

void AbstractTileTool::mapDocumentChanged(MapDocument *oldDocument,
                                          MapDocument *newDocument)
{
    Q_UNUSED(oldDocument)
    mBrushItem->setMapDocument(newDocument);
}

void AbstractTileTool::updateEnabledState()
{
    setEnabled(currentTileLayer() != nullptr);
    updateBrushVisibility();
}

void AbstractTileTool::updateStatusInfo()
{
    if (mBrushVisible) {
        Cell cell;

        if (const TileLayer *tileLayer = currentTileLayer()) {
            const QPoint pos = tilePosition() - tileLayer->position();
            cell = tileLayer->cellAt(pos);
        }

        QString tileIdString = cell.tileId() >= 0 ? QString::number(cell.tileId()) : tr("empty");

        QVarLengthArray<QChar, 3> flippedBits;
        if (cell.flippedHorizontally())
            flippedBits.append(QLatin1Char('H'));
        if (cell.flippedVertically())
            flippedBits.append(QLatin1Char('V'));
        if (cell.flippedAntiDiagonally())
            flippedBits.append(QLatin1Char('D'));

        if (!flippedBits.isEmpty()) {
            tileIdString.append(QLatin1Char(' '));
            tileIdString.append(flippedBits.first());
            for (int i = 1; i < flippedBits.size(); ++i) {
                tileIdString.append(QLatin1Char(','));
                tileIdString.append(flippedBits.at(i));
            }
        }

        setStatusInfo(QString(QLatin1String("%1, %2 [%3]"))
                      .arg(mTilePosition.x())
                      .arg(InvertYAxisHelper(mapDocument()).tileY(mTilePosition.y()))
                      .arg(tileIdString));
    } else {
        setStatusInfo(QString());
    }
}

TileLayer *AbstractTileTool::currentTileLayer() const
{
    if (mapDocument())
        if (auto currentLayer = mapDocument()->currentLayer())
            return currentLayer->asTileLayer();
    return nullptr;
}

void AbstractTileTool::updateBrushVisibility()
{
    // Show the tile brush only when at least one target layer is visible
    bool showBrush = false;
    if (mBrushVisible) {
        const auto layers = targetLayers();
        for (auto layer : layers) {
            if (!layer->isHidden()) {
                showBrush = true;
                break;
            }
        }
    }
    mBrushItem->setVisible(showBrush);
}

QList<Layer *> AbstractTileTool::targetLayers() const
{
    // By default, only a current tile layer is considered the target
    QList<Layer *> layers;
    if (Layer *layer = currentTileLayer())
        layers.append(layer);
    return layers;
}

/**
 * A helper method that returns the possible target layers of a given \a stamp.
 */
QList<Layer *> AbstractTileTool::targetLayersForStamp(const TileStamp &stamp) const
{
    QList<Layer*> layers;

    if (!mapDocument())
        return layers;

    const Map &map = *mapDocument()->map();

    for (const TileStampVariation &variation : stamp.variations()) {
        LayerIterator it(variation.map, Layer::TileLayerType);
        const Layer *firstLayer = it.next();
        const bool isMultiLayer = firstLayer && it.next();

        if (isMultiLayer && !firstLayer->name().isEmpty()) {
            for (Layer *layer : variation.map->tileLayers()) {
                TileLayer *target = static_cast<TileLayer*>(map.findLayer(layer->name(), Layer::TileLayerType));
                if (!layers.contains(target))
                    layers.append(target);
            }
        } else {
            if (TileLayer *tileLayer = currentTileLayer())
                if (!layers.contains(tileLayer))
                    layers.append(tileLayer);
        }
    }

    return layers;
}

void AbstractTileTool::setBrushVisible(bool visible)
{
    if (mBrushVisible == visible)
        return;

    mBrushVisible = visible;
    updateStatusInfo();
    updateBrushVisibility();
}
