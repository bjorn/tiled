/*
 * tilesetmodel.h
 * Copyright 2008-2009, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009, Edward Hutchins <eah1@yahoo.com>
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

#ifndef TILESETMODEL_H
#define TILESETMODEL_H

#include <QAbstractListModel>

namespace Tiled {

class Tile;
class Tileset;

namespace Internal {

/**
 * A model wrapping a tileset of a map. Used to display the tiles.
 */
class TilesetModel : public QAbstractListModel
{
    Q_OBJECT

public:
    /**
     * The TerrainRole allows querying the terrain info.
     */
    enum UserRoles {
        TerrainRole = Qt::UserRole
    };

    /**
     * Constructor.
     *
     * @param tileset the initial tileset to display
     */
    TilesetModel(Tileset *tileset, QObject *parent = nullptr);

    /**
     * Returns the number of rows.
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * Returns the number of columns.
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * Returns the data stored under the given <i>role</i> for the item
     * referred to by the <i>index</i>.
     */
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;



    /**
     * Returns a small size hint, to prevent the headers from affecting the
     * minimum width and height of the sections.
     */
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    /**
     * Returns the tile at the given index.
     */
    Tile *tileAt(const QModelIndex &index) const;

    int tileIdAt(const QModelIndex &index) const;

    /**
     * Returns the index of the given \a tile. The tile is required to be from
     * the tileset used by this model.
     */
    QModelIndex tileIndex(const Tile *tile) const;

    /**
     * Returns the tileset associated with this model.
     */
    Tileset *tileset() const { return mTileset; }

    /**
     * Sets the tileset associated with this model.
     */
    void setTileset(Tileset *tileset);

    /**
     * Refreshes the list of tile IDs. Should be called after tiles are added
     * or removed from the tileset.
     */
    void tilesetChanged();

    /**
     * Performs a reset on the model.
     */
    void resetModel();

    bool removeRows(int row, int count, const QModelIndex &parent);
    Qt::DropActions supportedDropActions() const override;

public slots:
    /**
     * Should be called when anything changes about the given \a tiles that
     * affects their display in any views on this model.
     *
     * Tiles that are not from the tileset displayed by this model are simply
     * ignored. All tiles in the list are assumed to be from the same tileset.
     *
     * \sa MapDocument::tileTerrainChanged
     */
    void tilesChanged(const QList<Tile*> &tiles);

    /**
     * Should be called when anything changes about the given \a tile that
     * affects its display in any views on this model.
     *
     * \sa MapDocument::tileAnimationChanged
     */
    void tileChanged(Tile *tile);

private:
    void refreshTileIds();

    Tileset *mTileset;
    QList<int> mTileIds;
};

} // namespace Internal
} // namespace Tiled

#endif // TILESETMODEL_H
