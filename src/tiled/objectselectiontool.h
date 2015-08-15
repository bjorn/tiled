/*
 * objectselectiontool.h
 * Copyright 2010-2013, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#ifndef OBJECTSELECTIONTOOL_H
#define OBJECTSELECTIONTOOL_H

#include "abstractobjecttool.h"

#include <QList>
#include <QSet>
#include <QVector>

class QGraphicsItem;

namespace Tiled {
namespace Internal {

class RotateHandle;
class ResizeHandle;
class MapObjectItem;
class SelectionRectangle;

class ObjectSelectionTool : public AbstractObjectTool
{
    Q_OBJECT

public:
    explicit ObjectSelectionTool(QObject *parent = 0);
    ~ObjectSelectionTool();

    void activate(MapScene *scene);
    void deactivate(MapScene *scene);

    void keyPressed(QKeyEvent *);
    void mouseEntered();
    void mouseMoved(const QPointF &pos,
                    Qt::KeyboardModifiers modifiers);
    void mousePressed(QGraphicsSceneMouseEvent *event);
    void mouseReleased(QGraphicsSceneMouseEvent *event);
    void modifiersChanged(Qt::KeyboardModifiers modifiers);

    void languageChanged();

private slots:
    void updateHandles();
    void updateHandleVisibility();

    void objectsRemoved(const QList<MapObject *> &);

private:
    enum Action {
        NoAction,
        Selecting,
        Moving,
        Rotating,
        Resizing
    };

    enum Mode {
        Resize,
        Rotate,
    };

    void updateSelection(const QPointF &pos,
                         Qt::KeyboardModifiers modifiers);

    void startSelecting();

    void startMoving(Qt::KeyboardModifiers modifiers);
    void updateMovingItems(const QPointF &pos,
                           Qt::KeyboardModifiers modifiers);
    void finishMoving(const QPointF &pos);

    void startRotating();
    void updateRotatingItems(const QPointF &pos,
                             Qt::KeyboardModifiers modifiers);
    void finishRotating(const QPointF &pos);

    void startResizing();
    void updateResizingItems(const QPointF &pos,
                             Qt::KeyboardModifiers modifiers);
    void updateResizingSingleItem(const QPointF &resizingOrigin,
                                  const QPointF &screenPos,
                                  Qt::KeyboardModifiers modifiers);
    void finishResizing(const QPointF &pos);
    
    void setMode(Mode mode);
    void saveSelectionState();

    void refreshCursor();

    QPointF snapToGrid(const QPointF &pos,
                       Qt::KeyboardModifiers modifiers);

    QList<MapObject*> changingObjects() const;

    struct MovingObject
    {
        MapObjectItem *item;
        QPointF oldItemPosition;

        QPointF oldPosition;
        QSizeF oldSize;
        QPolygonF oldPolygon;
        qreal oldRotation;
    };

    SelectionRectangle *mSelectionRectangle;
    QGraphicsItem *mOriginIndicator;
    RotateHandle *mRotateHandles[4];
    ResizeHandle *mResizeHandles[8];
    bool mMousePressed;
    MapObjectItem *mHoveredObjectItem;
    MapObjectItem *mClickedObjectItem;
    RotateHandle *mClickedRotateHandle;
    ResizeHandle *mClickedResizeHandle;

    QVector<MovingObject> mMovingObjects;

    QPointF mAlignPosition;
    QPointF mOrigin;
    bool mResizingLimitHorizontal;
    bool mResizingLimitVertical;
    Mode mMode;
    Action mAction;
    QPointF mStart;
    QPoint mScreenStart;
    Qt::KeyboardModifiers mModifiers;
};

} // namespace Internal
} // namespace Tiled

#endif // OBJECTSELECTIONTOOL_H
