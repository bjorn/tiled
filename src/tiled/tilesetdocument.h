/*
 * tilesetdocument.h
 * Copyright 2015-2016, Thorbjørn Lindeijer <bjorn@lindeijer.nl>
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

#ifndef TILED_INTERNAL_TILESETDOCUMENT_H
#define TILED_INTERNAL_TILESETDOCUMENT_H

#include "document.h"
#include "tileset.h"

#include <QList>
#include <QPointer>

namespace Tiled {

class TilesetFormat;

namespace Internal {

class MapDocument;
class TilesetTerrainModel;

/**
 * Represents an editable tileset.
 */
class TilesetDocument : public Document
{
    Q_OBJECT

public:
    TilesetDocument(const SharedTileset &tileset, const QString &fileName = QString());

    bool save(const QString &fileName, QString *error = nullptr) override;

    TilesetFormat *readerFormat() const;
    void setReaderFormat(TilesetFormat *format);

    FileFormat *writerFormat() const override;
    void setWriterFormat(TilesetFormat *format);

    QString displayName() const override;

    const SharedTileset &tileset() const;

    bool isEmbedded() const;
    void setClean();

    const QList<MapDocument*> &mapDocuments() const;
    void addMapDocument(MapDocument *mapDocument);
    void removeMapDocument(MapDocument *mapDocument);

    void setTilesetFileName(const QString &fileName);
    void setTilesetName(const QString &name);
    void setTilesetTileOffset(const QPoint &tileOffset);

    void addTiles(const QList<Tile*> &tiles);
    void removeTiles(const QList<Tile*> &tiles);

    TilesetTerrainModel *terrainModel() const { return mTerrainModel; }

signals:
    /**
     * This signal is currently used when adding or removing tiles from a tileset.
     *
     * @todo Emit more specific signals.
     */
    void tilesetChanged(Tileset *tileset);

    void tilesetFileNameChanged(Tileset *tileset);
    void tilesetNameChanged(Tileset *tileset);
    void tilesetTileOffsetChanged(Tileset *tileset);

    void tileImageSourceChanged(Tile *tile);

    /**
     * Emits the signal notifying tileset models about changes to tile terrain
     * information. All the \a tiles need to be from the same tileset.
     */
    void tileTerrainChanged(const QList<Tile*> &tiles);

    /**
     * Emits the signal notifying about the terrain probability of a tile changing.
     */
    void tileProbabilityChanged(Tile *tile);

    /**
     * Emits the signal notifying the TileCollisionEditor about the object group
     * of a tile changing.
     */
    void tileObjectGroupChanged(Tile *tile);

    /**
     * Emits the signal notifying about the animation of a tile changing.
     */
    void tileAnimationChanged(Tile *tile);

private slots:
    void onTerrainRemoved(Terrain *terrain);

private:
    SharedTileset mTileset;
    QList<MapDocument*> mMapDocuments;

    QPointer<TilesetFormat> mReaderFormat;
    QPointer<TilesetFormat> mWriterFormat;

    TilesetTerrainModel *mTerrainModel;
};


inline const SharedTileset &TilesetDocument::tileset() const
{
    return mTileset;
}

inline bool TilesetDocument::isEmbedded() const
{
    return fileName().isEmpty() && mMapDocuments.count() == 1;
}

/**
 * Returns the map documents this tileset is used in.
 */
inline const QList<MapDocument*> &TilesetDocument::mapDocuments() const
{
    return mMapDocuments;
}

} // namespace Internal
} // namespace Tiled

#endif // TILED_INTERNAL_TILESETDOCUMENT_H
