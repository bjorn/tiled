/*
 * isometricrenderer.cpp
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

#include "isometricrenderer.h"

#include "map.h"
#include "mapobject.h"
#include "tile.h"
#include "tilelayer.h"
#include "tileset.h"

#include <cmath>

using namespace Tiled;

QSize IsometricRenderer::mapSize() const
{
    // Map width and height contribute equally in both directions
    const int side = map()->height() + map()->width();
    return QSize(side * map()->tileWidth() / 2,
                 side * map()->tileHeight() / 2);
}

QRect IsometricRenderer::boundingRect(const QRect &rect) const
{
    const int tileWidth = map()->tileWidth();
    const int tileHeight = map()->tileHeight();

    const int originX = map()->height() * tileWidth / 2;
    const QPoint pos((rect.x() - (rect.y() + rect.height()))
                     * tileWidth / 2 + originX,
                     (rect.x() + rect.y()) * tileHeight / 2);

    const int side = rect.height() + rect.width();
    const QSize size(side * tileWidth / 2,
                     side * tileHeight / 2);

    return QRect(pos, size);
}

QRectF IsometricRenderer::boundingRect(const MapObject *object) const
{
    if (!object->cell().isEmpty()) {
        const QPointF bottomCenter = pixelToScreenCoords(object->position());
        const Tile *tile = object->cell().tile;
        const QSize imgSize = tile->image().size();
        const QPoint tileOffset = tile->totalOffset();
        const QSizeF objectSize = object->size();
        const QSizeF scale(objectSize.width() / imgSize.width(), objectSize.height() / imgSize.height());

        return QRectF(bottomCenter.x() + (tileOffset.x() * scale.width()) - objectSize.width() / 2,
                      bottomCenter.y() + (tileOffset.y() * scale.height()) - objectSize.height(),
                      objectSize.width(),
                      objectSize.height()).adjusted(-1, -1, 1, 1);
    } else if (!object->polygon().isEmpty()) {
        qreal extraSpace = qMax(objectLineWidth(), qreal(1));

        // Make some more room for the starting dot
        extraSpace += objectLineWidth() * 4;

        const QPointF &pos = object->position();
        const QPolygonF polygon = object->polygon().translated(pos);
        const QPolygonF screenPolygon = pixelToScreenCoords(polygon);
        return screenPolygon.boundingRect().adjusted(-extraSpace,
                                                     -extraSpace - 1,
                                                     extraSpace,
                                                     extraSpace);
    } else {
        // Take the bounding rect of the projected object, and then add a few
        // pixels on all sides to correct for the line width.
        const QRectF base = pixelRectToScreenPolygon(object->bounds()).boundingRect();
        const qreal extraSpace = qMax(objectLineWidth() / 2, qreal(1));

        return base.adjusted(-extraSpace,
                             -extraSpace - 1,
                             extraSpace, extraSpace);
    }
}

QPainterPath IsometricRenderer::shape(const MapObject *object) const
{
    QPainterPath path;
    if (!object->cell().isEmpty()) {
        path.addRect(boundingRect(object));
    } else {
        switch (object->shape()) {
        case MapObject::Ellipse:
        case MapObject::Rectangle:
            path.addPolygon(pixelRectToScreenPolygon(object->bounds()));
            break;
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
        }
    }
    return path;
}

void IsometricRenderer::drawGrid(QPainter *painter, const QRectF &rect,
                                 QColor gridColor) const
{
    const int tileWidth = map()->tileWidth();
    const int tileHeight = map()->tileHeight();

    QRect r = rect.toAlignedRect();
    r.adjust(-tileWidth / 2, -tileHeight / 2,
             tileWidth / 2, tileHeight / 2);

    const int startX = qMax(qreal(0), screenToTileCoords(r.topLeft()).x());
    const int startY = qMax(qreal(0), screenToTileCoords(r.topRight()).y());
    const int endX = qMin(qreal(map()->width()),
                          screenToTileCoords(r.bottomRight()).x());
    const int endY = qMin(qreal(map()->height()),
                          screenToTileCoords(r.bottomLeft()).y());

    gridColor.setAlpha(128);

    QPen gridPen(gridColor);
    gridPen.setCosmetic(true);
    gridPen.setDashPattern(QVector<qreal>() << 2 << 2);
    painter->setPen(gridPen);

    for (int y = startY; y <= endY; ++y) {
        const QPointF start = tileToScreenCoords(startX, y);
        const QPointF end = tileToScreenCoords(endX, y);
        painter->drawLine(start, end);
    }
    for (int x = startX; x <= endX; ++x) {
        const QPointF start = tileToScreenCoords(x, startY);
        const QPointF end = tileToScreenCoords(x, endY);
        painter->drawLine(start, end);
    }
}

void IsometricRenderer::drawTileLayer(QPainter *painter,
                                      const TileLayer *layer,
                                      const QRectF &exposed) const
{
    const int tileWidth = map()->tileWidth();
    const int tileHeight = map()->tileHeight();

    if (tileWidth <= 0 || tileHeight <= 1)
        return;

    QRect rect = exposed.toAlignedRect();
    if (rect.isNull())
        rect = boundingRect(layer->bounds());

    QMargins drawMargins = layer->drawMargins();
    drawMargins.setTop(drawMargins.top() - tileHeight);
    drawMargins.setRight(drawMargins.right() - tileWidth);

    rect.adjust(-drawMargins.right(),
                -drawMargins.bottom(),
                drawMargins.left(),
                drawMargins.top());

    // Determine the tile and pixel coordinates to start at
    QPointF tilePos = screenToTileCoords(rect.x(), rect.y());
    QPoint rowItr = QPoint((int) std::floor(tilePos.x()),
                           (int) std::floor(tilePos.y()));
    QPointF startPos = tileToScreenCoords(rowItr);
    startPos.rx() -= tileWidth / 2;
    startPos.ry() += tileHeight;

    // Compensate for the layer position
    rowItr -= QPoint(layer->x(), layer->y());

    /* Determine in which half of the tile the top-left corner of the area we
     * need to draw is. If we're in the upper half, we need to start one row
     * up due to those tiles being visible as well. How we go up one row
     * depends on whether we're in the left or right half of the tile.
     */
    const bool inUpperHalf = startPos.y() - rect.y() > tileHeight / 2;
    const bool inLeftHalf = rect.x() - startPos.x() < tileWidth / 2;

    if (inUpperHalf) {
        if (inLeftHalf) {
            --rowItr.rx();
            startPos.rx() -= tileWidth / 2;
        } else {
            --rowItr.ry();
            startPos.rx() += tileWidth / 2;
        }
        startPos.ry() -= tileHeight / 2;
    }

    // Determine whether the current row is shifted half a tile to the right
    bool shifted = inUpperHalf ^ inLeftHalf;

    CellRenderer renderer(painter);

    for (int y = startPos.y() * 2; y - tileHeight * 2 < rect.bottom() * 2;
         y += tileHeight)
    {
        QPoint columnItr = rowItr;

        for (int x = startPos.x(); x < rect.right(); x += tileWidth) {
            if (layer->contains(columnItr)) {
                const Cell &cell = layer->cellAt(columnItr);
                if (!cell.isEmpty()) {
                    renderer.render(cell, QPointF(x, (float)y / 2), QSizeF(0, 0),
                                    CellRenderer::BottomLeft);
                }
            }

            // Advance to the next column
            ++columnItr.rx();
            --columnItr.ry();
        }

        // Advance to the next row
        if (!shifted) {
            ++rowItr.rx();
            startPos.rx() += tileWidth / 2;
            shifted = true;
        } else {
            ++rowItr.ry();
            startPos.rx() -= tileWidth / 2;
            shifted = false;
        }
    }
}

void IsometricRenderer::drawTileSelection(QPainter *painter,
                                          const QRegion &region,
                                          const QColor &color,
                                          const QRectF &exposed) const
{
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);
    foreach (const QRect &r, region.rects()) {
        QPolygonF polygon = tileRectToScreenPolygon(r);
        if (QRectF(polygon.boundingRect()).intersects(exposed))
            painter->drawConvexPolygon(polygon);
    }
}

void IsometricRenderer::drawMapObject(QPainter *painter,
                                      const MapObject *object,
                                      const QColor &color) const
{
    painter->save();

    QPen pen(Qt::black);
    pen.setCosmetic(true);

    const Cell &cell = object->cell();

    if (!cell.isEmpty()) {
        const Tile *tile = cell.tile;
        const QSize imgSize = tile->size();
        const QPointF pos = pixelToScreenCoords(object->position());
        const QPointF tileOffset = tile->totalOffset();

        CellRenderer(painter).render(cell, pos, object->size(),
                                     CellRenderer::BottomCenter);

        if (testFlag(ShowTileObjectOutlines)) {
            QRectF rect(QPointF(pos.x() - imgSize.width() / 2 + tileOffset.x(),
                                pos.y() - imgSize.height() + tileOffset.y()),
                        imgSize);

            pen.setStyle(Qt::SolidLine);
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
        const qreal shadowOffset = (lineWidth == 0 ? 1 : lineWidth) / scale;

        QColor brushColor = color;
        brushColor.setAlpha(50);
        QBrush brush(brushColor);

        pen.setJoinStyle(Qt::RoundJoin);
        pen.setCapStyle(Qt::RoundCap);
        pen.setWidth(lineWidth);

        QPen colorPen(pen);
        colorPen.setColor(color);

        painter->setPen(pen);
        painter->setRenderHint(QPainter::Antialiasing);

        // TODO: Do something sensible to make null-sized objects usable

        switch (object->shape()) {
        case MapObject::Ellipse: {
            QPolygonF polygon = pixelRectToScreenPolygon(object->bounds());

            float tw = map()->tileWidth();
            float th = map()->tileHeight();
            QPointF transformScale(1, 1);
            if (tw > th)
                transformScale = QPointF(1, th/tw);
            else
                transformScale = QPointF(tw/th, 1);

            QPointF l1 = polygon.at(1) - polygon.at(0);
            QPointF l2 = polygon.at(3) - polygon.at(0);
            QTransform trans;
            trans.scale(transformScale.x(), transformScale.y());
            trans.rotate(45);
            QTransform iTrans = trans.inverted();
            QPointF l1x = iTrans.map(l1);
            QPointF l2x = iTrans.map(l2);
            QSizeF ellipseSize(l1x.manhattanLength(), l2x.manhattanLength());

            if (ellipseSize.width() > 0 && ellipseSize.height() > 0) {
                painter->save();
                painter->setPen(pen);
                painter->translate(polygon.at(0));
                painter->scale(transformScale.x(), transformScale.y());
                painter->rotate(45);
                painter->drawEllipse(QRectF(QPointF(0, 0), ellipseSize));
                painter->restore();
            }

            painter->setBrush(Qt::NoBrush);
            painter->drawPolygon(polygon);

            painter->setPen(colorPen);
            painter->setBrush(Qt::NoBrush);
            painter->translate(QPointF(0, -shadowOffset));
            painter->drawPolygon(polygon);

            painter->setBrush(brush);
            if (ellipseSize.width() > 0 && ellipseSize.height() > 0) {
                painter->save();
                painter->translate(polygon.at(0));
                painter->scale(transformScale.x(), transformScale.y());
                painter->rotate(45);
                painter->drawEllipse(QRectF(QPointF(0, 0), ellipseSize));
                painter->restore();
            }
            break;
        }
        case MapObject::Rectangle: {
            QPolygonF polygon = pixelRectToScreenPolygon(object->bounds());
            painter->drawPolygon(polygon);

            painter->setPen(colorPen);
            painter->setBrush(brush);
            polygon.translate(0, -shadowOffset);
            painter->drawPolygon(polygon);
            break;
        }
        case MapObject::Polygon: {
            const QPointF &pos = object->position();
            const QPolygonF polygon = object->polygon().translated(pos);
            QPolygonF screenPolygon = pixelToScreenCoords(polygon);

            QPen thickPen(pen);
            QPen thickColorPen(colorPen);
            thickPen.setWidthF(thickPen.widthF() * 4);
            thickColorPen.setWidthF(thickColorPen.widthF() * 4);

            painter->drawPolygon(screenPolygon);
            painter->setPen(thickPen);
            painter->drawPoint(screenPolygon.first());

            painter->setPen(colorPen);
            painter->setBrush(brush);
            screenPolygon.translate(0, -shadowOffset);

            painter->drawPolygon(screenPolygon);
            painter->setPen(thickColorPen);
            painter->drawPoint(screenPolygon.first());

            break;
        }
        case MapObject::Polyline: {
            const QPointF &pos = object->position();
            const QPolygonF polygon = object->polygon().translated(pos);
            QPolygonF screenPolygon = pixelToScreenCoords(polygon);

            QPen thickPen(pen);
            QPen thickColorPen(colorPen);
            thickPen.setWidthF(thickPen.widthF() * 4);
            thickColorPen.setWidthF(thickColorPen.widthF() * 4);

            painter->drawPolyline(screenPolygon);
            painter->setPen(thickPen);
            painter->drawPoint(screenPolygon.first());

            pen.setColor(color);
            painter->setPen(pen);
            screenPolygon.translate(0, -shadowOffset);

            painter->drawPolyline(screenPolygon);
            painter->setPen(thickColorPen);
            painter->drawPoint(screenPolygon.first());
            break;
        }
        }
    }

    painter->restore();
}

QPointF IsometricRenderer::pixelToTileCoords(qreal x, qreal y) const
{
    const int tileHeight = map()->tileHeight();

    return QPointF(x / tileHeight, y / tileHeight);
}

QPointF IsometricRenderer::tileToPixelCoords(qreal x, qreal y) const
{
    const int tileHeight = map()->tileHeight();

    return QPointF(x * tileHeight, y * tileHeight);
}

QPointF IsometricRenderer::screenToTileCoords(qreal x, qreal y) const
{
    const int tileWidth = map()->tileWidth();
    const int tileHeight = map()->tileHeight();

    x -= map()->height() * tileWidth / 2;
    const qreal tileY = y / tileHeight;
    const qreal tileX = x / tileWidth;

    return QPointF(tileY + tileX,
                   tileY - tileX);
}

QPointF IsometricRenderer::tileToScreenCoords(qreal x, qreal y) const
{
    const int tileWidth = map()->tileWidth();
    const int tileHeight = map()->tileHeight();
    const int originX = map()->height() * tileWidth / 2;

    return QPointF((x - y) * tileWidth / 2 + originX,
                   (x + y) * tileHeight / 2);
}

QPointF IsometricRenderer::screenToPixelCoords(qreal x, qreal y) const
{
    const int tileWidth = map()->tileWidth();
    const int tileHeight = map()->tileHeight();

    x -= map()->height() * tileWidth / 2;
    const qreal tileY = y / tileHeight;
    const qreal tileX = x / tileWidth;

    return QPointF((tileY + tileX) * tileHeight,
                   (tileY - tileX) * tileHeight);
}

QPointF IsometricRenderer::pixelToScreenCoords(qreal x, qreal y) const
{
    const int tileWidth = map()->tileWidth();
    const int tileHeight = map()->tileHeight();
    const int originX = map()->height() * tileWidth / 2;
    const qreal tileY = y / tileHeight;
    const qreal tileX = x / tileHeight;

    return QPointF((tileX - tileY) * tileWidth / 2 + originX,
                   (tileX + tileY) * tileHeight / 2);
}

QPolygonF IsometricRenderer::pixelRectToScreenPolygon(const QRectF &rect) const
{
    QPolygonF polygon;
    polygon << QPointF(pixelToScreenCoords(rect.topLeft()));
    polygon << QPointF(pixelToScreenCoords(rect.topRight()));
    polygon << QPointF(pixelToScreenCoords(rect.bottomRight()));
    polygon << QPointF(pixelToScreenCoords(rect.bottomLeft()));
    return polygon;
}

QPolygonF IsometricRenderer::tileRectToScreenPolygon(const QRect &rect) const
{
    const int tileWidth = map()->tileWidth();
    const int tileHeight = map()->tileHeight();

    const QPointF topRight = tileToScreenCoords(rect.topRight());
    const QPointF bottomRight = tileToScreenCoords(rect.bottomRight());
    const QPointF bottomLeft = tileToScreenCoords(rect.bottomLeft());

    QPolygonF polygon;
    polygon << QPointF(tileToScreenCoords(rect.topLeft()));
    polygon << QPointF(topRight.x() + tileWidth / 2,
                       topRight.y() + tileHeight / 2);
    polygon << QPointF(bottomRight.x(), bottomRight.y() + tileHeight);
    polygon << QPointF(bottomLeft.x() - tileWidth / 2,
                       bottomLeft.y() + tileHeight / 2);
    return polygon;
}
