/*
 * tilesetdock.h
 * Copyright 2008-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009, Edward Hutchins <eah1@yahoo.com>
 * Copyright 2012, Stefan Beller <stefanbeller@googlemail.com>
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

#ifndef TILESETDOCK_H
#define TILESETDOCK_H

#include "tileset.h"

#include <QDockWidget>
#include <QList>
#include <QMap>

class QAction;
class QActionGroup;
class QComboBox;
class QMenu;
class QModelIndex;
class QSignalMapper;
class QStackedWidget;
class QTabBar;
class QToolBar;
class QToolButton;

namespace Tiled {

class Terrain;
class Tile;
class TileLayer;
class Tileset;

namespace Internal {

class MapDocument;
class TilesetView;
class TileStamp;
class Zoomable;

/**
 * The dock widget that displays the tilesets. Also keeps track of the
 * currently selected tile.
 */
class TilesetDock : public QDockWidget
{
    Q_OBJECT

public:
    /**
     * Constructor.
     */
    TilesetDock(QWidget *parent = nullptr);

    ~TilesetDock();

    /**
     * Sets the map for which the tilesets should be displayed.
     */
    void setMapDocument(MapDocument *mapDocument);

    /**
     * Returns the currently selected tile.
     */
    Tile *currentTile() const { return mCurrentTile; }

    void selectTilesInStamp(const TileStamp &);

signals:
    /**
     * Emitted when the current tile changed.
     */
    void currentTileChanged(Tile *tile);

    /**
     * Emitted when the currently selected tiles changed.
     */
    void stampCaptured(const TileStamp &);

    /**
     * Emitted when files are dropped at the tileset dock.
     */
    void tilesetsDropped(const QStringList &paths);

    void newTileset();

protected:
    void changeEvent(QEvent *e) override;

    void dragEnterEvent(QDragEnterEvent *) override;
    void dropEvent(QDropEvent *) override;

private slots:
    void currentTilesetChanged();
    void selectionChanged();
    void currentChanged(const QModelIndex &index);

    void updateActions();
    void updateCurrentTiles();
    void indexPressed(const QModelIndex &index);

    void tilesetAdded(int index, Tileset *tileset);
    void tilesetChanged(Tileset *tileset);
    void tilesetRemoved(Tileset *tileset);
    void tilesetMoved(int from, int to);
    void tilesetReplaced(int index, Tileset *tileset);
    void tilesetNameChanged(Tileset *tileset);

    void tileImageSourceChanged(Tile *tile);
    void tileAnimationChanged(Tile *tile);

    void removeTileset();
    void removeTileset(int index);
    void moveTileset(int from, int to);

    void editTilesetProperties();
    void importTileset();
    void exportTileset();

    void editTerrain();
    void addTiles();
    void removeTiles();
    void reorderTiles(bool enabled);

    void documentAboutToClose(MapDocument *mapDocument);

    void refreshTilesetMenu();

private:
    void setCurrentTile(Tile *tile);
    void setCurrentTiles(TileLayer *tiles);
    void retranslateUi();

    Tileset *currentTileset() const;
    TilesetView *currentTilesetView() const;
    TilesetView *tilesetViewAt(int index) const;

    void setupTilesetModel(TilesetView *view, Tileset *tileset);

    MapDocument *mMapDocument;

    // Shared tileset references because the dock wants to add new tiles
    QVector<SharedTileset> mTilesets;

    QTabBar *mTabBar;
    QStackedWidget *mViewStack;
    QToolBar *mToolBar;
    Tile *mCurrentTile;
    TileLayer *mCurrentTiles;
    const Terrain *mTerrain;

    QAction *mNewTileset;
    QAction *mImportTileset;
    QAction *mExportTileset;
    QAction *mPropertiesTileset;
    QAction *mDeleteTileset;
    QAction *mEditTerrain;
    QAction *mAddTiles;
    QAction *mRemoveTiles;
    QAction *mReorderTiles;

    QMap<MapDocument *, QString> mCurrentTilesets;

    QToolButton *mTilesetMenuButton;
    QMenu *mTilesetMenu; //opens on click of mTilesetMenu
    QActionGroup *mTilesetActionGroup;
    QSignalMapper *mTilesetMenuMapper; //needed due to dynamic content

    Zoomable *mZoomable;
    QComboBox *mZoomComboBox;

    bool mEmittingStampCaptured;
    bool mSynchronizingSelection;
};

} // namespace Internal
} // namespace Tiled

#endif // TILESETDOCK_H
