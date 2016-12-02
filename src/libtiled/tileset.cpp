/*
 * tileset.cpp
 * Copyright 2008-2015, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009, Edward Hutchins <eah1@yahoo.com>
 *
 * This file is part of libtiled.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tileset.h"
#include "tile.h"
#include "terrain.h"

#include <QBitmap>

using namespace Tiled;

Tileset::Tileset(QString name, int tileWidth, int tileHeight,
                 int tileSpacing, int margin):
    Object(TilesetType),
    mName(std::move(name)),
    mTileWidth(tileWidth),
    mTileHeight(tileHeight),
    mTileSpacing(tileSpacing),
    mMargin(margin),
    mColumnCount(0),
    mExpectedColumnCount(0),
    mExpectedRowCount(0),
    mNextTileId(0),
    mTerrainDistancesDirty(false),
    mLoaded(true)
{
    Q_ASSERT(tileSpacing >= 0);
    Q_ASSERT(margin >= 0);
}

Tileset::~Tileset()
{
    qDeleteAll(mTiles);
    qDeleteAll(mTerrainTypes);
}

/**
 * Sets the tile size of this tileset. Affects how image is cut in loadImage.
 *
 * @warning Invalid for image collection tilesets!
 */
void Tileset::setTileSize(QSize tileSize)
{
    Q_ASSERT(!tileSize.isEmpty());
    mTileWidth = tileSize.width();
    mTileHeight = tileSize.height();
}

/**
 * Sets the space in pixels between tiles in the tileset. Affects how image is
 * cut in loadImage.
 */
void Tileset::setTileSpacing(int tileSpacing)
{
    Q_ASSERT(tileSpacing >= 0);
    mTileSpacing = tileSpacing;
}

/**
 * Sets the margin used by the tileset image. This is the number of pixels
 * at the top-left border of the image that is skipped when cutting out tiles.
 * Affects how image is cut in loadImage.
 */
void Tileset::setMargin(int margin)
{
    Q_ASSERT(margin >= 0);
    mMargin = margin;
}

/**
 * Returns the tile with the given ID, creating it when it does not exist yet.
 */
Tile *Tileset::findOrCreateTile(int id)
{
    if (Tile *tile = mTiles.value(id))
        return tile;

    mNextTileId = std::max(mNextTileId, id + 1);
    mTiles[id] = new Tile(id, this);
    setTileOrder(mTiles[id], id);
    return mTiles[id];
}

/**
 * Sets the tile display order in the TilesetDock.
 */
void Tileset::setTileOrder(Tile *tile, int index)
{
    // remove tile if it exists

    QMultiMap<int, Tile*>::iterator i = mOrderedTiles.begin();
    while (i != mOrderedTiles.end())
    {
        if (i.value()==tile)
        {
            mOrderedTiles.erase(i);
            break;
        }
        else
            i++;
    }

    i= mOrderedTiles.insert(index, tile);

    // if another tile has the same index then we need to rebuild
    if (mOrderedTiles.count(index) > 1)
        mNeedsReorder= true;
}

/**
 * Rebuilds the ordered tile list removing any duplicates.
 */
void Tileset::rebuildTileOrder()
{
    int index=0;
    QMultiMap <int, Tile*> newmap;
    QMultiMap<int, Tile*>::iterator i = mOrderedTiles.begin();

    while (i != mOrderedTiles.end())
    {
        newmap.insert(index, i.value());
        index++;
        i++;
    }

    mOrderedTiles= newmap;
    mNeedsReorder= false;
}

/**
 * Returns the order index for a tile.
 */
int Tileset::tileOrder(const Tile *tile) const
{
    QMultiMap<int, Tile*>::const_iterator i = mOrderedTiles.begin();
    while (i != mOrderedTiles.end())
    {
        if (i.value()==tile)
            return i.key();
        i++;
    }


    return -1;
}

/**
 * Sets an ordered tileset list for undo
 */
void Tileset::setOrderedTileset(const QMap<int, Tile*> order)
{
    mOrderedTiles= order;
    mOrderedTiles.detach();
}

/**
 * Retuns an ordered tile list for display in the TilesetDock.
 */
const QMap<int, Tile*> Tileset::orderedTiles()
{
    if (mNeedsReorder)
        rebuildTileOrder();

    QMap <int, Tile*> newmap;

    QMultiMap<int, Tile*>::iterator i = mOrderedTiles.begin();
    while (i != mOrderedTiles.end())
    {
        newmap[i.key()]= i.value();
        i++;
    }

    return newmap;
}

/**
 * Returns the number of tile rows in the tileset image.
 */
int Tileset::rowCount() const
{
    if (isCollection())
        return 1;

    return rowCountForHeight(mImageReference.size.height());
}

/**
 * Sets the transparent color. Pixels with this color will be masked out
 * when loadFromImage() is called.
 */
void Tileset::setTransparentColor(const QColor &c)
{
    mImageReference.transparentColor = c;
}

/**
 * Sets the image reference data for tileset image based tilesets.
 *
 * This function also sets the expected column count, which can be used later
 * for automatic adjustment of tile indexes in case the tileset image width has
 * changed.
 */
void Tileset::setImageReference(const ImageReference &reference)
{
    mImageReference = reference;
    mExpectedColumnCount = columnCountForWidth(mImageReference.size.width());
    mExpectedRowCount = rowCountForHeight(mImageReference.size.height());
}

/**
 * Load this tileset from the given tileset \a image. This will replace
 * existing tile images in this tileset with new ones. If the new image
 * contains more tiles than exist in the tileset new tiles will be
 * appended, if there are fewer tiles the excess images will be blanked.
 *
 * The tile width and height of this tileset must be higher than 0.
 *
 * @param image    the image to load the tiles from
 * @param fileName the file name of the image, which will be remembered
 *                 as the image source of this tileset.
 * @return <code>true</code> if loading was successful, otherwise
 *         returns <code>false</code>
 */
bool Tileset::loadFromImage(const QImage &image,
                            const QString &fileName)
{
    mImageReference.source = fileName;

    if (image.isNull()) {
        mImageReference.loaded = false;
        return false;
    }

    const QSize tileSize = this->tileSize();
    const int margin = this->margin();
    const int spacing = this->tileSpacing();

    Q_ASSERT(tileSize.width() > 0 && tileSize.height() > 0);

    const int stopWidth = image.width() - tileSize.width();
    const int stopHeight = image.height() - tileSize.height();

    int tileNum = 0;

    for (int y = margin; y <= stopHeight; y += tileSize.height() + spacing) {
        for (int x = margin; x <= stopWidth; x += tileSize.width() + spacing) {
            const QImage tileImage = image.copy(x, y, tileSize.width(), tileSize.height());
            QPixmap tilePixmap = QPixmap::fromImage(tileImage);
            const QColor &transparent = mImageReference.transparentColor;

            if (transparent.isValid()) {
                const QImage mask = tileImage.createMaskFromColor(transparent.rgb());
                tilePixmap.setMask(QBitmap::fromImage(mask));
            }

            auto it = mTiles.find(tileNum);
            if (it != mTiles.end())
                it.value()->setImage(tilePixmap);
            else
                mTiles.insert(tileNum, new Tile(tilePixmap, tileNum, this));

            ++tileNum;
        }
    }

    // Blank out any remaining tiles to avoid confusion (todo: could be more clear)
    for (Tile *tile : mTiles) {
        if (tile->id() >= tileNum) {
            QPixmap tilePixmap = QPixmap(tileSize);
            tilePixmap.fill();
            tile->setImage(tilePixmap);
        }
    }

    mNextTileId = std::max(mNextTileId, tileNum);

    mImageReference.size = image.size();
    mColumnCount = columnCountForWidth(mImageReference.size.width());
    mImageReference.loaded = true;

    return true;
}

/**
 * Tries to load the image this tileset is referring to.
 *
 * @return <code>true</code> if loading was successful, otherwise
 *         returns <code>false</code>
 */
bool Tileset::loadImage()
{
    return loadFromImage(mImageReference.create(), mImageReference.source);
}

/**
 * Returns whether the tiles in \a candidate use the same images as the ones
 * in \a subject. Note that \a candidate is allowed to have additional tiles
 * as well.
 */
static bool sameTileImages(const Tileset &subject, const Tileset &candidate)
{
    for (const Tile *subjectTile : subject.tiles()) {
        const Tile *replacementTile = candidate.findTile(subjectTile->id());
        if (!replacementTile)
            return false;
        if (subjectTile->imageSource() != replacementTile->imageSource())
            return false;
    }

    return true;
}

/**
 * This checks if there is a similar tileset in the given list.
 * It is needed for replacing this tileset by its similar copy.
 */
SharedTileset Tileset::findSimilarTileset(const QVector<SharedTileset> &tilesets) const
{
    for (const SharedTileset &candidate : tilesets) {
        Q_ASSERT(candidate != this);

        if (candidate->tileCount() != tileCount())
            continue;
        if (candidate->imageSource() != imageSource())
            continue;
        if (candidate->tileSize() != tileSize())
            continue;
        if (candidate->tileSpacing() != tileSpacing())
            continue;
        if (candidate->margin() != margin())
            continue;
        if (candidate->tileOffset() != tileOffset())
            continue;

        // For an image collection tileset, check the image sources
        if (isCollection())
            if (!sameTileImages(*this, *candidate))
                continue;

        return candidate;
    }

    return SharedTileset();
}

/**
 * Changes the source of the tileset image.
 *
 * Only takes affect when loadImage is called.
 */
void Tileset::setImageSource(const QString &imageSource)
{
    mImageReference.source = imageSource;
}

/**
 * Returns the column count that this tileset would have if the tileset
 * image would have the given \a width. This takes into account the tile
 * size, margin and spacing.
 */
int Tileset::columnCountForWidth(int width) const
{
    if (mTileWidth <= 0)
        return 0;
    return (width - mMargin + mTileSpacing) / (mTileWidth + mTileSpacing);
}

/**
 * Returns the row count that this tileset would have if the tileset
 * image would have the given \a width. This takes into account the tile
 * size, margin and spacing.
 */
int Tileset::rowCountForHeight(int height) const
{
    if (mTileHeight <= 0)
        return 0;
    return (height - mMargin + mTileSpacing) / (mTileHeight + mTileSpacing);
}

/**
 * Adds a new terrain type.
 *
 * @param name      the name of the terrain
 * @param imageTile the id of the tile that represents the terrain visually
 * @return the created Terrain instance
 */
Terrain *Tileset::addTerrain(const QString &name, int imageTileId)
{
    Terrain *terrain = new Terrain(terrainCount(), this, name,
                                   imageTileId);
    insertTerrain(terrainCount(), terrain);
    return terrain;
}

/**
 * Adds the \a terrain type at the given \a index.
 *
 * The terrain should already have this tileset associated with it.
 */
void Tileset::insertTerrain(int index, Terrain *terrain)
{
    Q_ASSERT(terrain->tileset() == this);

    mTerrainTypes.insert(index, terrain);

    // Reassign terrain IDs
    for (int terrainId = index; terrainId < mTerrainTypes.size(); ++terrainId)
        mTerrainTypes.at(terrainId)->mId = terrainId;

    // Adjust tile terrain references
    for (Tile *tile : mTiles) {
        for (int corner = 0; corner < 4; ++corner) {
            const int terrainId = tile->cornerTerrainId(corner);
            if (terrainId >= index)
                tile->setCornerTerrainId(corner, terrainId + 1);
        }
    }

    mTerrainDistancesDirty = true;
}

/**
 * Removes the terrain type at the given \a index and returns it. The
 * caller becomes responsible for the lifetime of the terrain type.
 *
 * This will cause the terrain ids of subsequent terrains to shift up to
 * fill the space and the terrain information of all tiles in this tileset
 * will be updated accordingly.
 */
Terrain *Tileset::takeTerrainAt(int index)
{
    Terrain *terrain = mTerrainTypes.takeAt(index);

    // Reassign terrain IDs
    for (int terrainId = index; terrainId < mTerrainTypes.size(); ++terrainId)
        mTerrainTypes.at(terrainId)->mId = terrainId;

    // Clear and adjust tile terrain references
    for (Tile *tile : mTiles) {
        for (int corner = 0; corner < 4; ++corner) {
            const int terrainId = tile->cornerTerrainId(corner);
            if (terrainId == index)
                tile->setCornerTerrainId(corner, 0xFF);
            else if (terrainId > index)
                tile->setCornerTerrainId(corner, terrainId - 1);
        }
    }

    mTerrainDistancesDirty = true;

    return terrain;
}

/**
 * Returns the transition penalty(/distance) between 2 terrains. -1 if no
 * transition is possible.
 */
int Tileset::terrainTransitionPenalty(int terrainType0, int terrainType1) const
{
    if (mTerrainDistancesDirty) {
        const_cast<Tileset*>(this)->recalculateTerrainDistances();
        const_cast<Tileset*>(this)->mTerrainDistancesDirty = false;
    }

    terrainType0 = terrainType0 == 255 ? -1 : terrainType0;
    terrainType1 = terrainType1 == 255 ? -1 : terrainType1;

    // Do some magic, since we don't have a transition array for no-terrain
    if (terrainType0 == -1 && terrainType1 == -1)
        return 0;
    if (terrainType0 == -1)
        return mTerrainTypes.at(terrainType1)->transitionDistance(terrainType0);
    return mTerrainTypes.at(terrainType0)->transitionDistance(terrainType1);
}

/**
 * Calculates the transition distance matrix for all terrain types.
 */
void Tileset::recalculateTerrainDistances()
{
    // some fancy macros which can search for a value in each byte of a word simultaneously
    #define hasZeroByte(dword) (((dword) - 0x01010101UL) & ~(dword) & 0x80808080UL)
    #define hasByteEqualTo(dword, value) (hasZeroByte((dword) ^ (~0UL/255 * (value))))

    // Terrain distances are the number of transitions required before one terrain may meet another
    // Terrains that have no transition path have a distance of -1

    for (int i = 0; i < terrainCount(); ++i) {
        Terrain *type = terrain(i);
        QVector<int> distance(terrainCount() + 1, -1);

        // Check all tiles for transitions to other terrain types
        for (const Tile *tile : mTiles) {
            if (!hasByteEqualTo(tile->terrain(), i))
                continue;

            // This tile has transitions, add the transitions as neightbours (distance 1)
            int tl = tile->cornerTerrainId(0);
            int tr = tile->cornerTerrainId(1);
            int bl = tile->cornerTerrainId(2);
            int br = tile->cornerTerrainId(3);

            // Terrain on diagonally opposite corners are not actually a neighbour
            if (tl == i || br == i) {
                distance[tr + 1] = 1;
                distance[bl + 1] = 1;
            }
            if (tr == i || bl == i) {
                distance[tl + 1] = 1;
                distance[br + 1] = 1;
            }

            // terrain has at least one tile of its own type
            distance[i + 1] = 0;
        }

        type->setTransitionDistances(distance);
    }

    // Calculate indirect transition distances
    bool bNewConnections;
    do {
        bNewConnections = false;

        // For each combination of terrain types
        for (int i = 0; i < terrainCount(); ++i) {
            Terrain *t0 = terrain(i);
            for (int j = 0; j < terrainCount(); ++j) {
                if (i == j)
                    continue;
                Terrain *t1 = terrain(j);

                // Scan through each terrain type, and see if we have any in common
                for (int t = -1; t < terrainCount(); ++t) {
                    int d0 = t0->transitionDistance(t);
                    int d1 = t1->transitionDistance(t);
                    if (d0 == -1 || d1 == -1)
                        continue;

                    // We have cound a common connection
                    int d = t0->transitionDistance(j);
                    Q_ASSERT(t1->transitionDistance(i) == d);

                    // If the new path is shorter, record the new distance
                    if (d == -1 || d0 + d1 < d) {
                        d = d0 + d1;
                        t0->setTransitionDistance(j, d);
                        t1->setTransitionDistance(i, d);

                        // We're making progress, flag for another iteration...
                        bNewConnections = true;
                    }
                }
            }
        }

        // Repeat while we are still making new connections (could take a
        // number of iterations for distant terrain types to connect)
    } while (bNewConnections);
}

/**
 * Adds a new tile to the end of the tileset.
 */
Tile *Tileset::addTile(const QPixmap &image, const QString &source)
{
    Tile *newTile = new Tile(takeNextTileId(), this);
    newTile->setImage(image);
    newTile->setImageSource(source);

    mTiles.insert(newTile->id(), newTile);
    if (mTileHeight < image.height())
        mTileHeight = image.height();
    if (mTileWidth < image.width())
        mTileWidth = image.width();
    return newTile;
}

/**
 * Adds the given list of tiles to this tileset.
 *
 * The tiles should already have unique tile IDs associated with them!
 */
void Tileset::addTiles(const QList<Tile *> &tiles)
{
    for (Tile *tile : tiles) {
        Q_ASSERT(!mTiles.contains(tile->id()));
        mTiles.insert(tile->id(), tile);
    }

    updateTileSize();
}

/**
 * Removes the given list of tiles from this tileset.
 *
 * @warning The tiles are not deleted!
 */
void Tileset::removeTiles(const QList<Tile *> &tiles)
{
    for (Tile *tile : tiles) {
        Q_ASSERT(mTiles.contains(tile->id()));
        mTiles.remove(tile->id());
    }

    updateTileSize();
}

/**
 * Removes the given tile from this set and deletes it.
 */
void Tileset::deleteTile(int id)
{
    delete mTiles.take(id);
}

/**
 * Sets the \a image to be used for the tile with the given \a id.
 *
 * This function makes sure the tile width and tile height properties of the
 * tileset reflect the maximum size. It is only expected to be used for
 * image collection tilesets.
 */
void Tileset::setTileImage(Tile *tile,
                           const QPixmap &image,
                           const QString &source)
{
    Q_ASSERT(isCollection());
    Q_ASSERT(mTiles.value(tile->id()) == tile);

    const QSize previousImageSize = tile->image().size();
    const QSize newImageSize = image.size();

    tile->setImage(image);
    tile->setImageSource(source);

    if (previousImageSize != newImageSize) {
        // Update our max. tile size
        if (previousImageSize.height() == mTileHeight ||
                previousImageSize.width() == mTileWidth) {
            // This used to be the max image; we have to recompute
            updateTileSize();
        } else {
            // Check if we have a new maximum
            if (mTileHeight < newImageSize.height())
                mTileHeight = newImageSize.height();
            if (mTileWidth < newImageSize.width())
                mTileWidth = newImageSize.width();
        }
    }
}

/**
 * Sets tile size to the maximum size.
 */
void Tileset::updateTileSize()
{
    int maxWidth = 0;
    int maxHeight = 0;
    for (Tile *tile : mTiles) {
        const QSize size = tile->size();
        if (maxWidth < size.width())
            maxWidth = size.width();
        if (maxHeight < size.height())
            maxHeight = size.height();
    }
    mTileWidth = maxWidth;
    mTileHeight = maxHeight;
}
