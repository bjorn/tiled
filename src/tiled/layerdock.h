/*
 * layerdock.h
 * Copyright 2008-2013, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2010, Andrew G. Crowell <overkill9999@gmail.com>
 * Copyright 2016, Mamed Ibrahimov <ibramlab@gmail.com>
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

#ifndef LAYERDOCK_H
#define LAYERDOCK_H

#include "mapdocument.h"

#include <QDockWidget>
#include <QItemDelegate>
#include <QPixmap>
#include <QTreeView>
#include <QToolButton>

class QLabel;
class QModelIndex;
class QTreeView;
class QUndoStack;

namespace Tiled {
namespace Internal {

class LayerView;

/**
 * The dock widget that displays the map layers.
 */
class LayerDock : public QDockWidget
{
    Q_OBJECT

public:
    /**
     * Constructor.
     */
    explicit LayerDock(QWidget *parent = nullptr);

    /**
     * Sets the map for which the layers should be displayed.
     */
    void setMapDocument(MapDocument *mapDocument);

protected:
    void changeEvent(QEvent *e) override;

private slots:
    void updateOpacitySlider();
    void layerChanged(int index);
    void editLayerName();
    void sliderValueChanged(int opacity);

private:
    void retranslateUi();

    QLabel *mOpacityLabel;
    QSlider *mOpacitySlider;
    LayerView *mLayerView;
    MapDocument *mMapDocument;
    bool mUpdatingSlider;
    bool mChangingLayerOpacity;
};

/**
 * Delegate for drawing an eye icon in LayerView when the layer is visible.
 */
class LayerVisibilityDelegate: public QItemDelegate
{
public:
    explicit LayerVisibilityDelegate(QObject *parent = 0);

protected:
    void drawCheck(QPainter *painter, const QStyleOptionViewItem &option,
        const QRect &rect, Qt::CheckState state) const;

private:
    QPixmap mPixmap;
};

/**
 * This view makes sure the size hint makes sense and implements the context
 * menu.
 */
class LayerView : public QTreeView
{
    Q_OBJECT

public:
    explicit LayerView(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    void setMapDocument(MapDocument *mapDocument);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void currentRowChanged(const QModelIndex &index);
    void indexPressed(const QModelIndex &index);
    void currentLayerIndexChanged(int index);

private:
    MapDocument *mMapDocument;
};

} // namespace Internal
} // namespace Tiled

#endif // LAYERDOCK_H
