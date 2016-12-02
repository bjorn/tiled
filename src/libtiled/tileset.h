/*
 * tileset.h
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

#ifndef TILESET_H
#define TILESET_H

#include "imagereference.h"
#include "object.h"

#include <QColor>
#include <QList>
#include <QVector>
#include <QPoint>
#include <QSharedPointer>
#include <QString>
#include <QPixmap>

class QImage;

namespace Tiled {

class Tile;
class Tileset;
class Terrain;

typedef QSharedPointer<Tileset> SharedTileset;

/**
 * A tileset, representing a set of tiles.
 *
 * This class is meant to be used by either loading tiles from a tileset image
 * (using loadFromImage) or by adding/removing individual tiles (using
 * addTile, insertTiles and removeTiles). These two use-cases are not meant to
 * be mixed.
 */
class TILEDSHARED_EXPORT Tileset : public Object
{
public:
    /**
     * Creates a new tileset with the given parameters. Using this function
     * makes sure the internal weak pointer is initialized, which enables the
     * sharedPointer() function.
     *
     * @param name        the name of the tileset
     * @param tileWidth   the width of the tiles in the tileset
     * @param tileHeight  the height of the tiles in the tileset
     * @param tileSpacing the spacing between the tiles in the tileset image
     * @param margin      the margin around the tiles in the tileset image
     */
    static SharedTileset create(const QString &name,
                                int tileWidth,
                                int tileHeight,
                                int tileSpacing = 0,
                                int margin = 0)
    {
        SharedTileset tileset(new Tileset(name, tileWidth, tileHeight,
                                          tileSpacing, margin));
        tileset->mWeakPointer = tileset;
        return tileset;
    }

private:
    /**
     * Private constructor. Use create() instead.
     */
    Tileset(QString name, int tileWidth, int tileHeight,
            int tileSpacing = 0, int margin = 0);

public:
    ~Tileset();

    const QString &name() const;
    void setName(const QString &name);

    const QString &fileName() const;
    void setFileName(const QString &fileName);
    bool isExternal() const;

    int tileWidth() const;
    int tileHeight() const;

    QSize tileSize() const;
    void setTileSize(QSize tileSize);

    int tileSpacing() const;
    void setTileSpacing(int tileSpacing);

    int margin() const;
    void setMargin(int margin);

    QPoint tileOffset() const;
    void setTileOffset(QPoint offset);

    const QMap<int, Tile*> &tiles() const;
    inline Tile *findTile(int id) const;
    Tile *tileAt(int id) const { return findTile(id); } // provided for Python
    Tile *findOrCreateTile(int id);
    void setTileOrder(Tile *tile, int index);
    void rebuildTileOrder();
    int tileOrder(const Tile *tile) const;
    void setOrderedTileset(const QMap<int, Tile*> order);
    const QMap<int, Tile*> orderedTiles();
    int tileCount() const;

    int columnCount() const;
    int rowCount() const;
    void setColumnCount(int columnCount);
    int expectedColumnCount() const;
    int expectedRowCount() const;
    void syncExpectedColumnsAndRows();

    int imageWidth() const;
    int imageHeight() const;

    QColor transparentColor() const;
    void setTransparentColor(const QColor &c);

    void setImageReference(const ImageReference &reference);

    bool loadFromImage(const QImage &image, const QString &fileName);
    bool loadFromImage(const QString &fileName);
    bool loadImage();

    SharedTileset findSimilarTileset(const QVector<SharedTileset> &tilesets) const;

    const QString &imageSource() const;
    void setImageSource(const QString &imageSource);
    bool isCollection() const;

    int columnCountForWidth(int width) const;
    int rowCountForHeight(int height) const;

    const QList<Terrain*> &terrains() const;
    int terrainCount() const;
    Terrain *terrain(int index) const;

    Terrain *addTerrain(const QString &name, int imageTileId);
    void insertTerrain(int index, Terrain *terrain);
    Terrain *takeTerrainAt(int index);

    int terrainTransitionPenalty(int terrainType0, int terrainType1) const;

    Tile *addTile(const QPixmap &image, const QString &source = QString());
    void addTiles(const QList<Tile*> &tiles);
    void removeTiles(const QList<Tile *> &tiles);
    void deleteTile(int id);

    void setNextTileId(int nextId);
    int nextTileId() const;
    int takeNextTileId();

    void setTileImage(Tile *tile,
                      const QPixmap &image,
                      const QString &source = QString());

    void markTerrainDistancesDirty();

    SharedTileset sharedPointer() const;

    void setLoaded(bool loaded);
    bool loaded() const;
    bool imageLoaded() const;

private:
    void updateTileSize();
    void recalculateTerrainDistances();

    QString mName;
    QString mFileName;
    ImageReference mImageReference;
    int mTileWidth;
    int mTileHeight;
    int mTileSpacing;
    int mMargin;
    QPoint mTileOffset;
    int mColumnCount;
    int mExpectedColumnCount;
    int mExpectedRowCount;
    QMap<int, Tile*> mTiles;
    QMultiMap<int, Tile*> mOrderedTiles;
    int mNextTileId;
    QList<Terrain*> mTerrainTypes;
    bool mTerrainDistancesDirty;
    bool mLoaded;
    bool mNeedsReorder;

    QWeakPointer<Tileset> mWeakPointer;
};


/**
 * Returns the name of this tileset.
 */
inline const QString &Tileset::name() const
{
    return mName;
}

/**
 * Sets the name of this tileset.
 */
inline void Tileset::setName(const QString &name)
{
    mName = name;
}

/**
 * Returns the file name of this tileset. When the tileset isn't an
 * external tileset, the file name is empty.
 */
inline const QString &Tileset::fileName() const
{
    return mFileName;
}

/**
 * Sets the filename of this tileset.
 */
inline void Tileset::setFileName(const QString &fileName)
{
    mFileName = fileName;
}

/**
 * Returns whether this tileset is external.
 */
inline bool Tileset::isExternal() const
{
    return !mFileName.isEmpty();
}

/**
 * Returns the maximum width of the tiles in this tileset.
 */
inline int Tileset::tileWidth() const
{
    return mTileWidth;
}

/**
 * Returns the maximum height of the tiles in this tileset.
 */
inline int Tileset::tileHeight() const
{
    return mTileHeight;
}

/**
 * Returns the maximum size of the tiles in this tileset.
 */
inline QSize Tileset::tileSize() const
{
    return QSize(mTileWidth, mTileHeight);
}

/**
 * Returns the spacing between the tiles in the tileset image.
 */
inline int Tileset::tileSpacing() const
{
    return mTileSpacing;
}

/**
 * Returns the margin around the tiles in the tileset image.
 */
inline int Tileset::margin() const
{
    return mMargin;
}

/**
 * Returns the offset that is applied when drawing the tiles in this
 * tileset.
 */
inline QPoint Tileset::tileOffset() const
{
    return mTileOffset;
}

/**
 * @see tileOffset
 */
inline void Tileset::setTileOffset(QPoint offset)
{
    mTileOffset = offset;
}

/**
 * Returns a const reference to the list of tiles in this tileset.
 */
inline const QMap<int, Tile *> &Tileset::tiles() const
{
    return mTiles;
}

/**
 * Returns the tile with the given tile ID. The tile IDs are local to this
 * tileset.
 */
inline Tile *Tileset::findTile(int id) const
{
    return mTiles.value(id);
}

/**
 * Returns the number of tiles in this tileset.
 *
 * Note that the tiles are not necessarily consecutive.
 */
inline int Tileset::tileCount() const
{
    return mTiles.size();
}

/**
 * Returns the number of tile columns in the tileset image.
 */
inline int Tileset::columnCount() const
{
    return mColumnCount;
}

/**
 * Sets the column count to use when displaying this tileset. For tileset image
 * based tilesets, this reflects the number of tile columns in the image.
 */
inline void Tileset::setColumnCount(int columnCount)
{
    mColumnCount = columnCount;
}

/**
 * Returns the number of tile columns expected to be in the tileset image. This
 * may differ from the actual amount of columns encountered when loading the
 * image, and can be used for automatically adjusting tile indexes.
 */
inline int Tileset::expectedColumnCount() const
{
    return mExpectedColumnCount;
}

/**
 * Returns the number of tile rows expected to be in the tileset image. This
 * may differ from the actual amount of rows encountered when loading the
 * image, and is checked when automatically adjusting tile indexes.
 */
inline int Tileset::expectedRowCount() const
{
    return mExpectedRowCount;
}

/**
 * Sets the expected column and row count to the actual column count. Usually
 * called after checking with the user whether he wants the map to be adjusted
 * to a change to the tileset image size.
 */
inline void Tileset::syncExpectedColumnsAndRows()
{
    mExpectedColumnCount = mColumnCount;
    mExpectedRowCount = rowCount();
}

/**
 * Returns the width of the tileset image.
 */
inline int Tileset::imageWidth() const
{
    return mImageReference.size.width();
}

/**
 * Returns the height of the tileset image.
 */
inline int Tileset::imageHeight() const
{
    return mImageReference.size.height();
}

/**
 * Returns the transparent color, or an invalid color if no transparent
 * color is used.
 */
inline QColor Tileset::transparentColor() const
{
    return mImageReference.transparentColor;
}

/**
 * Convenience override that loads the image using the QImage constructor.
 */
inline bool Tileset::loadFromImage(const QString &fileName)
{
    return loadFromImage(QImage(fileName), fileName);
}

/**
 * Returns the file name of the external image that contains the tiles in
 * this tileset. Is an empty string when this tileset doesn't have a
 * tileset image.
 */
inline const QString &Tileset::imageSource() const
{
    return mImageReference.source;
}

/**
 * Returns whether this tileset is a collection of images. In this case, the
 * tileset itself has no image source.
 */
inline bool Tileset::isCollection() const
{
    return imageSource().isEmpty();
}

/**
 * Returns a const reference to the list of terrains in this tileset.
 */
inline const QList<Terrain *> &Tileset::terrains() const
{
    return mTerrainTypes;
}

/**
 * Returns the number of terrain types in this tileset.
 */
inline int Tileset::terrainCount() const
{
    return mTerrainTypes.size();
}

/**
 * Returns the terrain type at the given \a index.
 */
inline Terrain *Tileset::terrain(int index) const
{
    return index >= 0 ? mTerrainTypes[index] : nullptr;
}

/**
 * Sets the next id to be used for tiles in this tileset.
 */
inline void Tileset::setNextTileId(int nextId)
{
    Q_ASSERT(nextId > 0);
    mNextTileId = nextId;
}

/**
 * Returns the next tile id for this tileset.
 */
inline int Tileset::nextTileId() const
{
    return mNextTileId;
}

/**
 * Returns the next tile id for this tileset and allocates a new one.
 */
inline int Tileset::takeNextTileId()
{
    return mNextTileId++;
}

/**
 * Used by the Tile class when its terrain information changes.
 */
inline void Tileset::markTerrainDistancesDirty()
{
    mTerrainDistancesDirty = true;
}

inline SharedTileset Tileset::sharedPointer() const
{
    return SharedTileset(mWeakPointer);
}

/**
 * Sets whether this tileset was loaded successfully. This variable is true by
 * default, but it can be set to false to indicate a failed attempt at loading
 * an external tileset.
 */
inline void Tileset::setLoaded(bool loaded)
{
    mLoaded = loaded;
}

/**
 * Returns whether this tileset was loaded succesfully. Only valid for
 * external tilesets (fileName() != empty).
 */
inline bool Tileset::loaded() const
{
    return mLoaded;
}

/**
 * Returns whether the image used by this tileset was loaded succesfully. Only
 * valid for tilesets based on a single image (imageSource() != empty).
 */
inline bool Tileset::imageLoaded() const
{
    return mImageReference.loaded;
}

} // namespace Tiled

#endif // TILESET_H
