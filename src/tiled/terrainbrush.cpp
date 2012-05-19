/*
 * terrainbrush.cpp
 * Copyright 2009-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2010 Stefan Beller <stefanbeller@googlemail.com>
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

#include "terrainbrush.h"

#include "brushitem.h"
#include "map.h"
#include "mapdocument.h"
#include "mapscene.h"
#include "painttilelayer.h"
#include "tilelayer.h"
#include "tileset.h"
#include "tile.h"

#include <math.h>
#include <QVector>

using namespace Tiled;
using namespace Tiled::Internal;

TerrainBrush::TerrainBrush(QObject *parent)
    : AbstractTileTool(tr("Terrain Brush"),
                       QIcon(QLatin1String(
                               ":images/22x22/stock-tool-clone.png")),
                       QKeySequence(tr("T")),
                       parent)
    , mTerrain(NULL)
    , mPaintX(0), mPaintY(0)
    , mOffsetX(0), mOffsetY(0)
    , mBrushBehavior(Free)
    , mLineReferenceX(0)
    , mLineReferenceY(0)
{
}

TerrainBrush::~TerrainBrush()
{
}


/**
 * Returns the lists of points on a line from (x0,y0) to (x1,y1).
 *
 * This is an implementation of bresenhams line algorithm, initially copied
 * from http://en.wikipedia.org/wiki/Bresenham's_line_algorithm#Optimization
 * changed to C++ syntax.
 */
static QVector<QPoint> calculateLine(int x0, int y0, int x1, int y1)
{
    QVector<QPoint> ret;

    bool steep = qAbs(y1 - y0) > qAbs(x1 - x0);
    if (steep) {
        qSwap(x0, y0);
        qSwap(x1, y1);
    }
    if (x0 > x1) {
        qSwap(x0, x1);
        qSwap(y0, y1);
    }
    const int deltax = x1 - x0;
    const int deltay = qAbs(y1 - y0);
    int error = deltax / 2;
    int ystep;
    int y = y0;

    if (y0 < y1)
        ystep = 1;
    else
        ystep = -1;

    for (int x = x0; x < x1 + 1 ; x++) {
        if (steep)
            ret += QPoint(y, x);
        else
            ret += QPoint(x, y);
        error = error - deltay;
        if (error < 0) {
             y = y + ystep;
             error = error + deltax;
        }
    }

    return ret;
}

void TerrainBrush::tilePositionChanged(const QPoint &pos)
{

    switch (mBrushBehavior) {
    case Paint:
	{
	    int x = mPaintX;
	    int y = mPaintY;
        foreach (const QPoint &p, calculateLine(x, y, pos.x(), pos.y()))
		{
		    updateBrush(p);
            doPaint(true, p.x(), p.y());
		}
		// HACK: because the line may traverse in the reverse direction, updateBrush() leaves these at the line start point
	    mPaintX = pos.x();
	    mPaintY = pos.y();
        break;
	}
    case LineStartSet:
        configureBrush(calculateLine(mLineReferenceX, mLineReferenceY, pos.x(), pos.y()));
        break;
    case Line:
    case Free:
        updateBrush(pos);
        break;
    }
}

void TerrainBrush::mousePressed(QGraphicsSceneMouseEvent *event)
{
    if (!brushItem()->isVisible())
        return;

    if (event->button() == Qt::LeftButton) {
        switch (mBrushBehavior) {
        case Line:
            mLineReferenceX = mPaintX;
            mLineReferenceY = mPaintY;
            mBrushBehavior = LineStartSet;
            break;
        case LineStartSet:
            doPaint(false, 0, 0);
            mLineReferenceX = mPaintX;
            mLineReferenceY = mPaintY;
            break;
        case Paint:
            beginPaint();
            break;
        case Free:
            beginPaint();
            mBrushBehavior = Paint;
            break;
        }
    } else {
        if (event->button() == Qt::RightButton)
            capture();
    }
}

void TerrainBrush::mouseReleased(QGraphicsSceneMouseEvent *event)
{
    switch (mBrushBehavior) {
    case Paint:
        if (event->button() == Qt::LeftButton)
            mBrushBehavior = Free;
    default:
        // do nothing?
        break;
    }
}

void TerrainBrush::configureBrush(const QVector<QPoint> &list)
{
/*
    QRegion reg;
    QRegion stampRegion;

    stampRegion = mStamp->region();

    Map *map = mapDocument()->map();

    TileLayer *stamp = new TileLayer(QString(), 0, 0,
                                     map->width(), map->height());

    foreach (const QPoint p, list) {
        const QRegion update = stampRegion.translated(p.x() - mStampX,
                                                      p.y() - mStampY);
        if (!reg.intersects(update)) {
            reg += update;

            if (mIsRandom) {
                TileLayer *newStamp = getRandomTileLayer();
                stamp->merge(p, newStamp);
                delete newStamp;
            } else {
                stamp->merge(p, mStamp);
            }

        }
    }

    brushItem()->setTileLayer(stamp);
    delete stamp;
*/
}

void TerrainBrush::modifiersChanged(Qt::KeyboardModifiers modifiers)
{
    if (modifiers & Qt::ShiftModifier) {
        mBrushBehavior = Line;
    } else {
        mBrushBehavior = Free;
    }

    updateBrush(tilePosition());
}

void TerrainBrush::languageChanged()
{
    setName(tr("Terain Brush"));
    setShortcut(QKeySequence(tr("T")));
}

void TerrainBrush::mapDocumentChanged(MapDocument *oldDocument,
                                    MapDocument *newDocument)
{
    AbstractTileTool::mapDocumentChanged(oldDocument, newDocument);

    // Reset the brush, since it probably became invalid
    brushItem()->setTileRegion(QRegion());
    setTerrain(NULL);

	// hack
	Tileset *t = newDocument->map()->tilesets().at(0);
	setTerrain(t->terrainType(2));
}

void TerrainBrush::setTerrain(TerrainType *terrain)
{
    if (mTerrain == terrain)
        return;

    mTerrain = terrain;

	updateBrush(tilePosition());
}

void TerrainBrush::beginPaint()
{
    if (mBrushBehavior != Free)
        return;

    mBrushBehavior = Paint;
    doPaint(false, mPaintX, mPaintY);
}

void TerrainBrush::capture()
{
    TileLayer *tileLayer = currentTileLayer();
    Q_ASSERT(tileLayer);

	// TODO: we need to know which corner the mouse is closest to...

	const Cell &cell = tileLayer->cellAt(tilePosition());
	TerrainType *t = cell.tile->cornerTerrain(0);
    setTerrain(t);
}

void TerrainBrush::doPaint(bool mergeable, int whereX, int whereY)
{
    TileLayer *stamp = brushItem()->tileLayer();

    if (!stamp)
        return;

    // This method shouldn't be called when current layer is not a tile layer
    TileLayer *tileLayer = currentTileLayer();
    Q_ASSERT(tileLayer);

	whereX -= mOffsetX;
	whereY -= mOffsetY;

    if (!tileLayer->bounds().intersects(QRect(whereX, whereY, stamp->width(), stamp->height())))
        return;

    PaintTileLayer *paint = new PaintTileLayer(mapDocument(), tileLayer, whereX, whereY, stamp);
    paint->setMergeable(mergeable);
    mapDocument()->undoStack()->push(paint);
    mapDocument()->emitRegionEdited(brushItem()->tileRegion(), tileLayer);
}

static inline unsigned int makeTerrain(int t)
{
	t &= 0xFF;
	return t << 24 | t << 16 | t << 8 | t;
}

static inline unsigned int makeTerrain(int tl, int tr, int bl, int br)
{
	return (tl & 0xFF) << 24 | (tr & 0xFF) << 16 | (bl & 0xFF) << 8 | (br & 0xFF);
}

Tile *TerrainBrush::findBestTile(unsigned int terrain, unsigned int considerationMask)
{
	// if all quadrants are set to 'no terrain', then the 'empty' tile is the only choice we can deduce
	if (terrain == 0xFFFFFFFF)
		return NULL;

	QList<Tile*> matches;
	int confidence = 0;

	// TODO: should we scan each tileset? we really need to use gId's for terrains aswell as tiles i guess...
	Tileset *tileset = mapDocument()->map()->tilesets().at(0);

	int tileCount = tileset->tileCount();
	for (int i = 0; i < tileCount; ++i) {
		Tile *t = tileset->tileAt(i);
		if ((t->terrain() & considerationMask) == (terrain & considerationMask)) {
			// prefer tiles with the most possible matches to the requested terrain
			int matchingQuadrants = 0;
			if ((t->terrain() & 0xFF000000) == (terrain & 0xFF000000))
				++matchingQuadrants;
			if ((t->terrain() & 0xFF0000) == (terrain & 0xFF0000))
				++matchingQuadrants;
			if ((t->terrain() & 0xFF00) == (terrain & 0xFF00))
				++matchingQuadrants;
			if ((t->terrain() & 0xFF) == (terrain & 0xFF))
				++matchingQuadrants;

			if (matchingQuadrants >= confidence) {
				if (matchingQuadrants > confidence)
					matches.clear();
				confidence = matchingQuadrants;

				matches.push_back(t);
			}
		}
	}

	// choose a candidate at random, with consideration for terrain probability
	if (!matches.isEmpty())
		return matches[0];

	return NULL;
}

void TerrainBrush::updateBrush(const QPoint &cursorPos)
{
	// get the current tile layer (TODO: what if the current layer isn't a tile layer?)
    TileLayer *currentLayer = currentTileLayer();
    Q_ASSERT(currentLayer);

	int layerWidth = currentLayer->width();
	int layerHeight = currentLayer->height();
	int numTiles = layerWidth * layerHeight;

	if (!currentLayer->bounds().contains(cursorPos))
		return;

	// allocate a buffer to build the terrain tilemap (TODO: this could be retained per layer to save regular allocation)
	Tile **newTerrain = new Tile*[numTiles];

	// allocate a buffer of flags for each tile that may be considered (TODO: this could be retained per layer to save regular allocation)
	char *checked = new char[numTiles];
	memset(checked, 0, numTiles);

	// create a consideration list, and push the starting point
	QList<QPoint> transitionList;
	transitionList.push_back(cursorPos);
	bool first = true;

	QRect brushRect(cursorPos, cursorPos);

	// produce terrain with transitions using a simple, relative naive approach (considers each tile once, and doesn't allow re-consideration if selection was bad)
	while (!transitionList.isEmpty()) {
		// get the next point in the consideration list
		QPoint p = transitionList.front();
		transitionList.pop_front();
		int x = p.x(), y = p.y();

		// if we have already considered this point, skip to the next
		if (checked[y*layerWidth + x])
			continue;

		// get the relevant tiles
		const Tile *tile = currentLayer->cellAt(p).tile;
		Tile *paste = NULL;

		// find a tile that best suits this position
		if (first) {
			// the first tile is special, we will just paste the selected terrain and add the surroundings for consideration

			// TODO: if we're painting quadrants rather than full tiles, we need to set the appropriate mask
			paste = mTerrain ? findBestTile(makeTerrain(mTerrain->id()), 0xFFFFFFFF) : NULL;
			newTerrain[y*layerWidth + x] = paste;
			first = false;
		} else {
			// following tiles each need consideration against their surroundings
			unsigned int preferredTerrain = tile->terrain();
			unsigned int mask = 0;

			// depending which connections have been set, we update the preferred terrain of the tile accordingly
			if (y > 0 && checked[(y-1)*layerWidth + x]) {
				preferredTerrain = (newTerrain[(y-1)*layerWidth + x]->terrain() << 16) | (preferredTerrain & 0x0000FFFF);
				mask |= 0xFFFF0000;
			}
			if (y < layerHeight - 1 && checked[(y+1)*layerWidth + x]) {
				preferredTerrain = (newTerrain[(y+1)*layerWidth + x]->terrain() >> 16) | (preferredTerrain & 0xFFFF0000);
				mask |= 0x0000FFFF;
			}
			if (x > 0 && checked[y*layerWidth + x - 1]) {
				preferredTerrain = ((newTerrain[y*layerWidth + x - 1]->terrain() << 8) & 0xFF00FF00) | (preferredTerrain & 0x00FF00FF);
				mask |= 0xFF00FF00;
			}
			if (x < layerWidth - 1 && checked[y*layerWidth + x + 1]) {
				preferredTerrain = ((newTerrain[y*layerWidth + x + 1]->terrain() >> 8) & 0x00FF00FF) | (preferredTerrain & 0xFF00FF00);
				mask |= 0x00FF00FF;
			}

			paste = findBestTile(preferredTerrain, mask);
			newTerrain[y*layerWidth + x] = paste;
		}

		// update the brush rect and consideration map
		brushRect |= QRect(p, p);
		checked[y*layerWidth + x] = true;

		// consider surrounding tiles
		if (y > 0) {
			const Tile *above = currentLayer->cellAt(x, y - 1).tile;
			if (paste->topEdge() != above->bottomEdge() && !checked[(y-1)*layerWidth + x])
				transitionList.push_back(QPoint(x, y - 1));
		}
		if (y < layerHeight - 1) {
			const Tile *below = currentLayer->cellAt(x, y + 1).tile;
			if (paste->bottomEdge() != below->topEdge() && !checked[(y+1)*layerWidth + x])
				transitionList.push_back(QPoint(x, y + 1));
		}
		if (x > 0) {
			const Tile *left = currentLayer->cellAt(x - 1, y).tile;
			if (paste->leftEdge() != left->rightEdge() && !checked[y*layerWidth + x - 1])
				transitionList.push_back(QPoint(x - 1, y));
		}
		if (x < layerWidth - 1) {
			const Tile *right = currentLayer->cellAt(x + 1, y).tile;
			if (paste->rightEdge() != right->leftEdge() && !checked[y*layerWidth + x + 1])
				transitionList.push_back(QPoint(x + 1, y));
		}
	}

	// create a stamp for the terrain block
	TileLayer *stamp = new TileLayer(QString(), 0, 0, brushRect.width(), brushRect.height());

	for (int y = brushRect.top(); y <= brushRect.bottom(); ++y) {
		for (int x = brushRect.left(); x <= brushRect.right(); ++x) {
			if (!checked[y*layerWidth + x])
				continue;
			stamp->setCell(x - brushRect.left(), y - brushRect.top(), Cell(newTerrain[y*layerWidth + x]));
		}
	}

    brushItem()->setTileLayer(stamp);

	delete[] checked;
	delete[] newTerrain;

/*
    const QPoint tilePos = tilePosition();

	if (!brushItem()->tileLayer()) {
        brushItem()->setTileRegion(QRect(tilePos, QSize(1, 1)));
    }
*/

	brushItem()->setTileLayerPosition(QPoint(brushRect.left(), brushRect.top()));

	mPaintX = cursorPos.x();
	mPaintY = cursorPos.y();
	mOffsetX = cursorPos.x() - brushRect.left();
	mOffsetY = cursorPos.y() - brushRect.top();
}
