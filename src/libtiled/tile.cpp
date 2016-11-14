/*
 * tile.cpp
 * Copyright 2012-2014, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "tile.h"

#include "objectgroup.h"
#include "tileset.h"

using namespace Tiled;

Tile::Tile(int id, Tileset *tileset):
    Object(TileType),
    mId(id),
    mTileset(tileset),
    mTerrain(-1),
    mProbability(1.f),
    mObjectGroup(nullptr),
    mCurrentFrameIndex(0),
    mUnusedTime(0)
{}

Tile::Tile(const QPixmap &image, int id, Tileset *tileset):
    Object(TileType),
    mId(id),
    mTileset(tileset),
    mImage(image),
    mTerrain(-1),
    mProbability(1.f),
    mObjectGroup(nullptr),
    mCurrentFrameIndex(0),
    mUnusedTime(0)
{}

Tile::~Tile()
{
    delete mObjectGroup;
}

/**
 * Returns the tileset that this tile is part of as a shared pointer.
 */
QSharedPointer<Tileset> Tile::sharedTileset() const
{
    return mTileset->sharedPointer();
}

/**
 * Returns the image for rendering this tile, taking into account tile
 * animations.
 */
const QPixmap &Tile::currentFrameImage() const
{
    if (isAnimated()) {
        const Frame &frame = mFrames.at(mCurrentFrameIndex);
        return mTileset->findTile(frame.tileId)->image();
    } else {
        return mImage;
    }
}

/**
 * Returns the drawing offset of the tile (in pixels).
 */
QPoint Tile::totalOffset() const
{
    return mTileset->tileOffset() + mOffset;
}

/**
 * Returns the Terrain of a given corner.
 */
Terrain *Tile::terrainAtCorner(int corner) const
{
    return mTileset->terrain(cornerTerrainId(corner));
}

/**
 * Set the terrain for each corner of the tile.
 */
void Tile::setTerrain(unsigned terrain)
{
    if (mTerrain == terrain)
        return;

    mTerrain = terrain;
    mTileset->markTerrainDistancesDirty();
}

/**
 * Sets \a objectGroup to be the group of objects associated with this tile.
 * The Tile takes ownership over the ObjectGroup and it can't also be part of
 * a map.
 */
void Tile::setObjectGroup(ObjectGroup *objectGroup)
{
    Q_ASSERT(!objectGroup || !objectGroup->map());

    if (mObjectGroup == objectGroup)
        return;

    delete mObjectGroup;
    mObjectGroup = objectGroup;
}

/**
 * Swaps the object group of this tile with \a objectGroup. The tile releases
 * ownership over its existing object group and takes ownership over the new
 * one.
 *
 * @return The previous object group referenced by this tile.
 */
ObjectGroup *Tile::swapObjectGroup(ObjectGroup *objectGroup)
{
    ObjectGroup *previousObjectGroup = mObjectGroup;
    mObjectGroup = objectGroup;
    return previousObjectGroup;
}

/**
 * Sets the animation frames to be used by this tile. Resets any currently
 * running animation.
 */
void Tile::setFrames(const QVector<Frame> &frames)
{
    resetAnimation();
    mFrames = frames;
}

/**
 * Resets the tile animation. Returns whether this caused the current tileId to
 * change.
 */
bool Tile::resetAnimation()
{
    if (!isAnimated())
        return false;

    Frame previousFrame = mFrames.at(mCurrentFrameIndex);
    Frame currentFrame = mFrames.at(0);

    mCurrentFrameIndex = 0;
    mUnusedTime = 0;

    return previousFrame.tileId != currentFrame.tileId;
}

/**
 * Advances this tile animation by the given amount of milliseconds. Returns
 * whether this caused the current tileId to change.
 */
bool Tile::advanceAnimation(int ms)
{
    if (!isAnimated())
        return false;

    mUnusedTime += ms;

    Frame frame = mFrames.at(mCurrentFrameIndex);
    const int previousTileId = frame.tileId;

    while (frame.duration > 0 && mUnusedTime > frame.duration) {
        mUnusedTime -= frame.duration;
        mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mFrames.size();

        frame = mFrames.at(mCurrentFrameIndex);
    }

    return previousTileId != frame.tileId;
}
