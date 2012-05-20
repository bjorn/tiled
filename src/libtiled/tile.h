/*
 * tile.h
 * Copyright 2008-2009, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#ifndef TILE_H
#define TILE_H

#include "object.h"
#include "tileset.h"

#include <QPixmap>

namespace Tiled {

class TILEDSHARED_EXPORT Tile : public Object
{
public:
    Tile(const QPixmap &image, int id, Tileset *tileset):
        mId(id),
        mTileset(tileset),
        mImage(image),
		mTerrain(-1)
    {}

    /**
     * Returns ID of this tile within its tileset.
     */
    int id() const { return mId; }

    /**
     * Returns the tileset that this tile is part of.
     */
    Tileset *tileset() const { return mTileset; }

    /**
     * Returns the image of this tile.
     */
    const QPixmap &image() const { return mImage; }

    /**
     * Sets the image of this tile.
     */
    void setImage(const QPixmap &image) { mImage = image; }

    /**
     * Returns the width of this tile.
     */
    int width() const { return mImage.width(); }

    /**
     * Returns the height of this tile.
     */
    int height() const { return mImage.height(); }

    /**
     * Returns the size of this tile.
     */
    QSize size() const { return mImage.size(); }

    /**
     * Terrain type related stuff (this shoudl be the only bit-twiddley stuff there is)
     */
    TerrainType *cornerTerrain(int corner) const { int t = cornerTerrainType(corner); return t >= 0 ? mTileset->terrainType(t) : NULL; }

    int cornerTerrainType(int corner) const { unsigned int t = (terrain() >> (3 - corner)*8) & 0xFF; return t == 0xFF ? -1 : (int)t; }
    void setCornerTerrainType(int corner, int terrain)
    {
        unsigned int mask = 0xFF << (3 - corner)*8;
        unsigned int insert = terrain << (3 - corner)*8;
        mTerrain = (mTerrain & ~mask) | (insert & mask);
    }

    // returns the terrain terrain id for edges, and the whole tile which can be compared against other tiles
    unsigned short topEdge() const { return terrain() >> 16; }
    unsigned short bottomEdge() const { return terrain() & 0xFFFF; }
    unsigned short leftEdge() const { return((terrain() >> 16) & 0xFF00) | ((terrain() >> 8) & 0xFF); }
    unsigned short rightEdge() const { return ((terrain() >> 8) & 0xFF00) | (terrain() & 0xFF); }
    unsigned int terrain() const { return this == NULL ? 0xFFFFFFFF : mTerrain; }

private:
    int mId;
    Tileset *mTileset;
    QPixmap mImage;
    unsigned int mTerrain;
};

} // namespace Tiled

#endif // TILE_H
