/*
 * bucketfilltool.cpp
 * Copyright 2009-2010, Jeff Bland <jksb@member.fsf.org>
 * Copyright 2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2010, Jared Adams <jaxad0127@gmail.com>
 * Copyright 2011, Stefan Beller <stefanbeller@googlemail.com>
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

#include "bucketfilltool.h"

#include "addremovetileset.h"
#include "brushitem.h"
#include "filltiles.h"
#include "tilepainter.h"
#include "tile.h"
#include "mapscene.h"
#include "mapdocument.h"

#include <QApplication>

using namespace Tiled;
using namespace Tiled::Internal;

BucketFillTool::BucketFillTool(QObject *parent)
    : AbstractTileTool(tr("Bucket Fill Tool"),
                       QIcon(QLatin1String(
                               ":images/22x22/stock-tool-bucket-fill.png")),
                       QKeySequence(tr("F")),
                       parent)
    , mIsActive(false)
    , mLastShiftStatus(false)
    , mIsRandom(false)
    , mLastRandomStatus(false)
{
}

BucketFillTool::~BucketFillTool()
{
}

void BucketFillTool::activate(MapScene *scene)
{
    AbstractTileTool::activate(scene);

    mIsActive = true;
    tilePositionChanged(tilePosition());
}

void BucketFillTool::deactivate(MapScene *scene)
{
    AbstractTileTool::deactivate(scene);

    mFillRegion = QRegion();
    mIsActive = false;
}

void BucketFillTool::tilePositionChanged(const QPoint &tilePos)
{
    if (mStamp.isEmpty())
        return;

    bool shiftPressed = QApplication::keyboardModifiers() & Qt::ShiftModifier;
    bool fillRegionChanged = false;

    // Make sure that a tile layer is selected
    TileLayer *tileLayer = currentTileLayer();
    if (!tileLayer)
        return;

    // todo: When there are multiple variations, it would make sense to choose
    // random variations while filling. When the variations have different
    // sizes, probably the bounding box of all variations should be used.
    Map *variation = mStamp.randomVariation();
    TileLayer *stampLayer = static_cast<TileLayer*>(variation->layerAt(0));

    // Skip filling if the stamp is empty
    if (!stampLayer || stampLayer->isEmpty())
        return;

    TilePainter regionComputer(mapDocument(), tileLayer);
    // If the stamp is a single tile, ignore it when making the region
    if (stampLayer->width() == 1 && stampLayer->height() == 1 && !shiftPressed &&
            stampLayer->cellAt(0, 0) == regionComputer.cellAt(tilePos.x(),
                                                              tilePos.y()))
        return;

    // This clears the connections so we don't get callbacks
    clearConnections(mapDocument());

    // Optimization: we don't need to recalculate the fill area
    // if the new mouse position is still over the filled region
    // and the shift modifier hasn't changed.
    if (!mFillRegion.contains(tilePos) || shiftPressed != mLastShiftStatus || mIsRandom) {

        // Clear overlay to make way for a new one
        clearOverlay();

        // Cache information about how the fill region was created
        mLastShiftStatus = shiftPressed;

        // Get the new fill region
        if (!shiftPressed) {
            // If not holding shift, a region is generated from the current pos
            mFillRegion = regionComputer.computePaintableFillRegion(tilePos);
        } else {
            // If holding shift, the region is the selection bounds
            mFillRegion = mapDocument()->selectedArea();

            // Fill region is the whole map if there is no selection
            if (mFillRegion.isEmpty())
                mFillRegion = tileLayer->bounds();

            // The mouse needs to be in the region
            if (!mFillRegion.contains(tilePos))
                mFillRegion = QRegion();
        }
        fillRegionChanged = true;
    }

    // Ensure that a fill region was created before making an overlay layer
    if (mFillRegion.isEmpty())
        return;

    if (mLastRandomStatus != mIsRandom) {
        mLastRandomStatus = mIsRandom;
        fillRegionChanged = true;
    }

    if (!mFillOverlay) {
        // Create a new overlay region
        const QRect fillBounds = mFillRegion.boundingRect();
        mFillOverlay = SharedTileLayer(new TileLayer(QString(),
                                                     fillBounds.x(),
                                                     fillBounds.y(),
                                                     fillBounds.width(),
                                                     fillBounds.height()));
    }

    // Paint the new overlay
    if (!mIsRandom) {
        if (fillRegionChanged) {
            mMissingTilesets.clear();
            mapDocument()->unifyTilesets(variation, mMissingTilesets);

            TilePainter tilePainter(mapDocument(), mFillOverlay.data());
            tilePainter.drawStamp(stampLayer, mFillRegion);
        }
    } else {
        randomFill(mFillOverlay.data(), mFillRegion);
        fillRegionChanged = true;
    }

    if (fillRegionChanged) {
        // Update the brush item to draw the overlay
        brushItem()->setTileLayer(mFillOverlay);
    }
    // Create connections to know when the overlay should be cleared
    makeConnections();
}

void BucketFillTool::mousePressed(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || mFillRegion.isEmpty())
        return;
    if (!brushItem()->isVisible())
        return;

    FillTiles *fillTiles = new FillTiles(mapDocument(),
                                         currentTileLayer(),
                                         mFillRegion,
                                         brushItem()->tileLayer());

    if (!mMissingTilesets.isEmpty()) {
        for (const SharedTileset &tileset : mMissingTilesets)
            new AddTileset(mapDocument(), tileset, fillTiles);

        mMissingTilesets.clear();
    }

    QRegion fillRegion(mFillRegion);
    mapDocument()->undoStack()->push(fillTiles);
    mapDocument()->emitRegionEdited(fillRegion, currentTileLayer());
}

void BucketFillTool::mouseReleased(QGraphicsSceneMouseEvent *)
{
}

void BucketFillTool::modifiersChanged(Qt::KeyboardModifiers)
{
    // Don't need to recalculate fill region if there was no fill region
    if (!mFillOverlay)
        return;

    tilePositionChanged(tilePosition());
}

void BucketFillTool::languageChanged()
{
    setName(tr("Bucket Fill Tool"));
    setShortcut(QKeySequence(tr("F")));
}

void BucketFillTool::mapDocumentChanged(MapDocument *oldDocument,
                                        MapDocument *newDocument)
{
    AbstractTileTool::mapDocumentChanged(oldDocument, newDocument);

    clearConnections(oldDocument);

    // Reset things that are probably invalid now
    setStamp(TileStamp());
    clearOverlay();
}

void BucketFillTool::setStamp(const TileStamp &stamp)
{
    // Clear any overlay that we presently have with an old stamp
    clearOverlay();

    mStamp = stamp;

    if (mIsRandom)
        updateRandomList();

    if (mIsActive && brushItem()->isVisible())
        tilePositionChanged(tilePosition());
}

void BucketFillTool::clearOverlay()
{
    // Clear connections before clearing overlay so there is no
    // risk of getting a callback and causing an infinite loop
    clearConnections(mapDocument());

    brushItem()->clear();
    mFillOverlay.clear();

    mFillRegion = QRegion();
    brushItem()->setTileRegion(QRegion());
}

void BucketFillTool::makeConnections()
{
    if (!mapDocument())
        return;

    // Overlay may need to be cleared if a region changed
    connect(mapDocument(), SIGNAL(regionChanged(QRegion)),
            this, SLOT(clearOverlay()));

    // Overlay needs to be cleared if we switch to another layer
    connect(mapDocument(), SIGNAL(currentLayerIndexChanged(int)),
            this, SLOT(clearOverlay()));

    // Overlay needs be cleared if the selection changes, since
    // the overlay may be bound or may need to be bound to the selection
    connect(mapDocument(), SIGNAL(selectedAreaChanged(QRegion,QRegion)),
            this, SLOT(clearOverlay()));
}

void BucketFillTool::clearConnections(MapDocument *mapDocument)
{
    if (!mapDocument)
        return;

    disconnect(mapDocument, SIGNAL(regionChanged(QRegion)),
               this, SLOT(clearOverlay()));

    disconnect(mapDocument, SIGNAL(currentLayerIndexChanged(int)),
               this, SLOT(clearOverlay()));

    disconnect(mapDocument, SIGNAL(selectedAreaChanged(QRegion,QRegion)),
               this, SLOT(clearOverlay()));
}

void BucketFillTool::setRandom(bool value)
{
    if (mIsRandom == value)
        return;

    mIsRandom = value;

    if (mIsRandom)
        updateRandomList();
    else
        mRandomCellPicker.clear();

    // Don't need to recalculate fill region if there was no fill region
    if (!mFillOverlay)
        return;

    tilePositionChanged(tilePosition());
}

void BucketFillTool::randomFill(TileLayer *tileLayer, const QRegion &region) const
{
    if (region.isEmpty() || mRandomCellPicker.isEmpty())
        return;

    foreach (const QRect &rect, region.rects()) {
        for (int _x = rect.left(); _x <= rect.right(); ++_x) {
            for (int _y = rect.top(); _y <= rect.bottom(); ++_y) {

                // todo: take into account tile probability
                tileLayer->setCell(_x - tileLayer->x(),
                                   _y - tileLayer->y(),
                                   mRandomCellPicker.pick());
            }
        }
    }
}

void BucketFillTool::updateRandomList()
{
    mRandomCellPicker.clear();
    mMissingTilesets.clear();

    foreach (const TileStampVariation &variation, mStamp.variations()) {
        TileLayer *tileLayer = static_cast<TileLayer*>(variation.map->layerAt(0));
        mapDocument()->unifyTilesets(variation.map, mMissingTilesets);
        for (int x = 0; x < tileLayer->width(); x++) {
            for (int y = 0; y < tileLayer->height(); y++) {
                const Cell &cell = tileLayer->cellAt(x, y);
                if (!cell.isEmpty())
                    mRandomCellPicker.add(cell, cell.tile->probability());
            }
        }
    }
}
