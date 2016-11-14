/*
 * orthogonalrenderer.cpp
 * Copyright 2009-2011, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "orthogonalrenderer.h"

#include "map.h"
#include "mapobject.h"
#include "tile.h"
#include "tilelayer.h"
#include "tileset.h"

#include <QtCore/qmath.h>

using namespace Tiled;

QSize OrthogonalRenderer::mapSize() const
{
    return QSize(map()->width() * map()->tileWidth(),
                 map()->height() * map()->tileHeight());
}

QRect OrthogonalRenderer::boundingRect(const QRect &rect) const
{
    const int tileWidth = map()->tileWidth();
    const int tileHeight = map()->tileHeight();

    return QRect(rect.x() * tileWidth,
                 rect.y() * tileHeight,
                 rect.width() * tileWidth,
                 rect.height() * tileHeight);
}

QRectF OrthogonalRenderer::boundingRect(const MapObject *object) const
{
    const QRectF bounds = object->bounds();

    QRectF boundingRect;

    if (!object->cell().isEmpty()) {
        const QPointF bottomLeft = bounds.topLeft();
        const Tile *tile = object->cell().tile;
        const QSize imgSize = tile->image().size();
        const QPoint tileOffset = tile->totalOffset();
        const QSizeF objectSize = object->size();
        const QSizeF scale(objectSize.width() / imgSize.width(), objectSize.height() / imgSize.height());

        boundingRect = QRectF(bottomLeft.x() + (tileOffset.x() * scale.width()),
                              bottomLeft.y() + (tileOffset.y() * scale.height()) - objectSize.height(),
                              objectSize.width(),
                              objectSize.height()).adjusted(-1, -1, 1, 1);
    } else {
        qreal extraSpace = qMax(objectLineWidth(), qreal(1));

        switch (object->shape()) {
        case MapObject::Ellipse:
        case MapObject::Rectangle:
            if (bounds.isNull()) {
                boundingRect = bounds.adjusted(-10 - extraSpace,
                                               -10 - extraSpace,
                                               10 + extraSpace + 1,
                                               10 + extraSpace + 1);
            } else {
                boundingRect = bounds.adjusted(-extraSpace,
                                               -extraSpace,
                                               extraSpace + 1,
                                               extraSpace + 1);
            }
            break;

        case MapObject::Polygon:
        case MapObject::Polyline: {
            // Make some more room for the starting dot
            extraSpace += objectLineWidth() * 4;

            const QPointF &pos = object->position();
            const QPolygonF polygon = object->polygon().translated(pos);
            QPolygonF screenPolygon = pixelToScreenCoords(polygon);
            boundingRect = screenPolygon.boundingRect().adjusted(-extraSpace,
                                                                 -extraSpace,
                                                                 extraSpace + 1,
                                                                 extraSpace + 1);
            break;
        }
        }
    }

    return boundingRect;
}

QPainterPath OrthogonalRenderer::shape(const MapObject *object) const
{
    QPainterPath path;

    if (!object->cell().isEmpty()) {
        path.addRect(boundingRect(object));
    } else {
        switch (object->shape()) {
        case MapObject::Rectangle: {
            const QRectF bounds = object->bounds();

            if (bounds.isNull()) {
                path.addEllipse(bounds.topLeft(), 20, 20);
            } else {
                path.addRoundedRect(bounds, 10, 10);
            }
            break;
        }
        case MapObject::Polygon:
        case MapObject::Polyline: {
            const QPointF &pos = object->position();
            const QPolygonF polygon = object->polygon().translated(pos);
            const QPolygonF screenPolygon = pixelToScreenCoords(polygon);
            if (object->shape() == MapObject::Polygon) {
                path.addPolygon(screenPolygon);
            } else {
                for (int i = 1; i < screenPolygon.size(); ++i) {
                    path.addPolygon(lineToPolygon(screenPolygon[i - 1],
                                                  screenPolygon[i]));
                }
                path.setFillRule(Qt::WindingFill);
            }
            break;
        }
        case MapObject::Ellipse: {
            const QRectF bounds = object->bounds();

            if (bounds.isNull()) {
                path.addEllipse(bounds.topLeft(), 20, 20);
            } else {
                path.addEllipse(bounds);
            }
            break;
        }
        }
    }

    return path;
}

void OrthogonalRenderer::drawGrid(QPainter *painter, const QRectF &rect,
                                  QColor gridColor) const
{
    const int tileWidth = map()->tileWidth();
    const int tileHeight = map()->tileHeight();

    if (tileWidth <= 0 || tileHeight <= 0)
        return;

    const int startX = qMax(0, (int) (rect.x() / tileWidth) * tileWidth);
    const int startY = qMax(0, (int) (rect.y() / tileHeight) * tileHeight);
    const int endX = qMin(qCeil(rect.right()),
                          map()->width() * tileWidth + 1);
    const int endY = qMin(qCeil(rect.bottom()),
                          map()->height() * tileHeight + 1);

    gridColor.setAlpha(128);

    QPen gridPen(gridColor);
    gridPen.setCosmetic(true);
    gridPen.setDashPattern(QVector<qreal>() << 2 << 2);

    if (startY < endY) {
        gridPen.setDashOffset(startY);
        painter->setPen(gridPen);
        for (int x = startX; x < endX; x += tileWidth)
            painter->drawLine(x, startY, x, endY - 1);
    }

    if (startX < endX) {
        gridPen.setDashOffset(startX);
        painter->setPen(gridPen);
        for (int y = startY; y < endY; y += tileHeight)
            painter->drawLine(startX, y, endX - 1, y);
    }
}

void OrthogonalRenderer::drawTileLayer(QPainter *painter,
                                       const TileLayer *layer,
                                       const QRectF &exposed) const
{

    const int tileWidth = map()->tileWidth();
    const int tileHeight = map()->tileHeight();
    const QPointF layerPos(layer->x() * tileWidth,
                           layer->y() * tileHeight);

    int startX = 0;
    int startY = 0;
    int endX = layer->width() - 1;
    int endY = layer->height() - 1;

    if (!exposed.isNull()) {
        QMargins drawMargins = layer->drawMargins();
        drawMargins.setTop(drawMargins.top() - tileHeight);
        drawMargins.setRight(drawMargins.right() - tileWidth);

        QRectF rect = exposed.adjusted(-drawMargins.right(),
                                       -drawMargins.bottom(),
                                       drawMargins.left(),
                                       drawMargins.top());

        rect.translate(-layerPos);

        startX = qMax(qFloor(rect.x() / tileWidth), 0);
        startY = qMax(qFloor(rect.y() / tileHeight), 0);
        endX = qMin(qCeil(rect.right()) / tileWidth, endX);
        endY = qMin(qCeil(rect.bottom()) / tileHeight, endY);
    }

    // Return immediately when there is nothing to draw
    if (startX > endX || startY > endY)
        return;

    const QTransform savedTransform = painter->transform();
    painter->translate(layerPos);

    CellRenderer renderer(painter);

    Map::RenderOrder renderOrder = map()->renderOrder();

    int incX = 1, incY = 1;
    switch (renderOrder) {
    case Map::RightUp:
        std::swap(startY, endY);
        incY = -1;
        break;
    case Map::LeftDown:
        std::swap(startX, endX);
        incX = -1;
        break;
    case Map::LeftUp:
        std::swap(startX, endX);
        std::swap(startY, endY);
        incX = -1;
        incY = -1;
        break;
    case Map::RightDown:
    default:
        break;
    }

    endX += incX;
    endY += incY;

    for (int y = startY; y != endY; y += incY) {
        for (int x = startX; x != endX; x += incX) {
            const Cell &cell = layer->cellAt(x, y);
            if (cell.isEmpty())
                continue;

            renderer.render(cell,
                            QPointF(x * tileWidth, (y + 1) * tileHeight),
                            QSizeF(0, 0),
                            CellRenderer::BottomLeft);
        }
    }

    renderer.flush();

    painter->setTransform(savedTransform);
}

void OrthogonalRenderer::drawTileSelection(QPainter *painter,
                                           const QRegion &region,
                                           const QColor &color,
                                           const QRectF &exposed) const
{
    foreach (const QRect &r, region.rects()) {
        const QRectF toFill = QRectF(boundingRect(r)).intersected(exposed);
        if (!toFill.isEmpty())
            painter->fillRect(toFill, color);
    }
}

void OrthogonalRenderer::drawMapObject(QPainter *painter,
                                       const MapObject *object,
                                       const QColor &color) const
{
    painter->save();

    const QRectF bounds = object->bounds();
    QRectF rect(bounds);

    painter->translate(rect.topLeft());
    rect.moveTopLeft(QPointF(0, 0));

    const Cell &cell = object->cell();

    if (!cell.isEmpty()) {
        CellRenderer(painter).render(cell, QPointF(), object->size(),
                                     CellRenderer::BottomLeft);

        if (testFlag(ShowTileObjectOutlines)) {
            const Tile *tile = cell.tile;
            const QSize imgSize = tile->size();
            const QPointF tileOffset = tile->totalOffset();
            QRectF rect(QPointF(tileOffset.x(),
                                tileOffset.y() - imgSize.height()),
                        imgSize);

            QPen pen(Qt::SolidLine);
            pen.setCosmetic(true);
            painter->setPen(pen);
            painter->drawRect(rect);
            pen.setStyle(Qt::DotLine);
            pen.setColor(color);
            painter->setPen(pen);
            painter->drawRect(rect);
        }
    } else {
        const qreal lineWidth = objectLineWidth();
        const qreal scale = painterScale();
        const qreal shadowDist = (lineWidth == 0 ? 1 : lineWidth) / scale;
        const QPointF shadowOffset = QPointF(shadowDist * 0.5,
                                             shadowDist * 0.5);

        QPen linePen(color, lineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        linePen.setCosmetic(true);
        QPen shadowPen(linePen);
        shadowPen.setColor(Qt::black);

        QColor brushColor = color;
        brushColor.setAlpha(50);
        const QBrush fillBrush(brushColor);

        painter->setRenderHint(QPainter::Antialiasing);

        // Trying to draw an ellipse with 0-width is causing a hang in
        // CoreGraphics when drawing the path requested by the
        // QCoreGraphicsPaintEngine. Draw them as rectangle instead.
        MapObject::Shape shape = object->shape();
        if (shape == MapObject::Ellipse &&
                ((rect.width() == qreal(0)) ^ (rect.height() == qreal(0)))) {
            shape = MapObject::Rectangle;
        }

        switch (shape) {
        case MapObject::Rectangle: {
            if (rect.isNull())
                rect = QRectF(QPointF(-10, -10), QSizeF(20, 20));

            // Draw the shadow
            painter->setPen(shadowPen);
            painter->drawRect(rect.translated(shadowOffset));

            painter->setPen(linePen);
            painter->setBrush(fillBrush);
            painter->drawRect(rect);
            break;
        }

        case MapObject::Polyline: {
            QPolygonF screenPolygon = pixelToScreenCoords(object->polygon());

            QPen thickShadowPen(shadowPen);
            QPen thickLinePen(linePen);
            thickShadowPen.setWidthF(thickShadowPen.widthF() * 4);
            thickLinePen.setWidthF(thickLinePen.widthF() * 4);

            painter->setPen(shadowPen);
            painter->drawPolyline(screenPolygon.translated(shadowOffset));
            painter->setPen(thickShadowPen);
            painter->drawPoint(screenPolygon.first() + shadowOffset);

            painter->setPen(linePen);
            painter->setBrush(fillBrush);
            painter->drawPolyline(screenPolygon);
            painter->setPen(thickLinePen);
            painter->drawPoint(screenPolygon.first());
            break;
        }

        case MapObject::Polygon: {
            QPolygonF screenPolygon = pixelToScreenCoords(object->polygon());

            QPen thickShadowPen(shadowPen);
            QPen thickLinePen(linePen);
            thickShadowPen.setWidthF(thickShadowPen.widthF() * 4);
            thickLinePen.setWidthF(thickLinePen.widthF() * 4);

            painter->setPen(shadowPen);
            painter->drawPolygon(screenPolygon.translated(shadowOffset));
            painter->setPen(thickShadowPen);
            painter->drawPoint(screenPolygon.first() + shadowOffset);

            painter->setPen(linePen);
            painter->setBrush(fillBrush);
            painter->drawPolygon(screenPolygon);
            painter->setPen(thickLinePen);
            painter->drawPoint(screenPolygon.first());
            break;
        }

        case MapObject::Ellipse: {
            if (rect.isNull())
                rect = QRectF(QPointF(-10, -10), QSizeF(20, 20));

            // Draw the shadow
            painter->setPen(shadowPen);
            painter->drawEllipse(rect.translated(shadowOffset));

            painter->setPen(linePen);
            painter->setBrush(fillBrush);
            painter->drawEllipse(rect);
            break;
        }
        }
    }

    painter->restore();
}

QPointF OrthogonalRenderer::pixelToTileCoords(qreal x, qreal y) const
{
    return QPointF(x / map()->tileWidth(),
                   y / map()->tileHeight());
}

QPointF OrthogonalRenderer::tileToPixelCoords(qreal x, qreal y) const
{
    return QPointF(x * map()->tileWidth(),
                   y * map()->tileHeight());
}

QPointF OrthogonalRenderer::screenToTileCoords(qreal x, qreal y) const
{
    return QPointF(x / map()->tileWidth(),
                   y / map()->tileHeight());
}

QPointF OrthogonalRenderer::tileToScreenCoords(qreal x, qreal y) const
{
    return QPointF(x * map()->tileWidth(),
                   y * map()->tileHeight());
}

QPointF OrthogonalRenderer::screenToPixelCoords(qreal x, qreal y) const
{
    return QPointF(x, y);
}

QPointF OrthogonalRenderer::pixelToScreenCoords(qreal x, qreal y) const
{
    return QPointF(x, y);
}
