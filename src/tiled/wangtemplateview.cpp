/*
 * wangtemplateview.cpp
 * Copyright 2017, Benjamin Trotter <bdtrotte@ucsc.edu>
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

#include "wangtemplateview.h"

#include "utils.h"
#include "wangtemplatemodel.h"
#include "wangset.h"
#include "zoomable.h"

#include <QAbstractItemDelegate>
#include <QApplication>
#include <QCoreApplication>
#include <QGesture>
#include <QGestureEvent>
#include <QPainter>
#include <QPinchGesture>
#include <QWheelEvent>

using namespace Tiled;
using namespace Internal;

namespace {
/* The delegate for drawing the wang tile templates
 * */
class WangTemplateDelegate : public QAbstractItemDelegate
{
public:
    WangTemplateDelegate(WangTemplateView *wangtemplateView, QObject *parent = nullptr)
        : QAbstractItemDelegate(parent)
        , mWangTemplateView(wangtemplateView)
    { }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    WangTemplateView *mWangTemplateView;
};

//returns a color on the rainbow! 0 = red
static QColor getColor(int color, int max)
{
    if (color == 0)
        return QColor(0,0,0,0);
    float hue = ((float)color-1)/(float)max;
    return QColor::fromHsvF(hue,1,1);
}

static void paintTemplateTile(QPainter *painter,
                              WangId wangId,
                              int edgeCount,
                              int cornerCount,
                              const QRect &rect)
{
    painter->save();

    QRect centeredRect = rect.translated(-rect.center());
    centeredRect.adjust(2,2,-2,-2);

    painter->translate(rect.center());
    painter->setClipRect(centeredRect);
    painter->setRenderHint(QPainter::Antialiasing);

    painter->setBrush(Qt::red);
    painter->setPen(QPen(Qt::gray, 1));

    //paints corners
    if (cornerCount > 1) {
        QPolygon p;

        if (edgeCount > 1) {
            //top right
            p.append(QPoint(centeredRect.right()/3, centeredRect.top()/3));
            p.append(QPoint(centeredRect.right()/3, centeredRect.top()));
            p.append(centeredRect.topRight());
            p.append(QPoint(centeredRect.right(), centeredRect.top()/3));

            painter->setBrush(getColor(wangId.cornerColor(0), cornerCount));
            painter->drawPolygon(p);
            p.clear();
            //bot right
            p.append(QPoint(centeredRect.right()/3, centeredRect.bottom()/3));
            p.append(QPoint(centeredRect.right()/3, centeredRect.bottom()));
            p.append(centeredRect.bottomRight());
            p.append(QPoint(centeredRect.right(), centeredRect.bottom()/3));

            painter->setBrush(getColor(wangId.cornerColor(1), cornerCount));
            painter->drawPolygon(p);
            p.clear();
            //bot left
            p.append(QPoint(centeredRect.left()/3, centeredRect.bottom()/3));
            p.append(QPoint(centeredRect.left()/3, centeredRect.bottom()));
            p.append(centeredRect.bottomLeft());
            p.append(QPoint(centeredRect.left(), centeredRect.bottom()/3));

            painter->setBrush(getColor(wangId.cornerColor(2), cornerCount));
            painter->drawPolygon(p);
            p.clear();
            //top left
            p.append(QPoint(centeredRect.left()/3, centeredRect.top()/3));
            p.append(QPoint(centeredRect.left()/3, centeredRect.top()));
            p.append(centeredRect.topLeft());
            p.append(QPoint(centeredRect.left(), centeredRect.top()/3));

            painter->setBrush(getColor(wangId.cornerColor(3), cornerCount));
            painter->drawPolygon(p);
        } else {
            //top right
            p.append(QPoint(0, 0));
            p.append(QPoint(0, centeredRect.top()));
            p.append(centeredRect.topRight());
            p.append(QPoint(centeredRect.right(), 0));

            painter->setBrush(getColor(wangId.cornerColor(0), cornerCount));
            painter->drawPolygon(p);
            p.clear();
            //bottom right
            p.append(QPoint(0, 0));
            p.append(QPoint(0, centeredRect.bottom()));
            p.append(centeredRect.bottomRight());
            p.append(QPoint(centeredRect.right(), 0));

            painter->setBrush(getColor(wangId.cornerColor(1), cornerCount));
            painter->drawPolygon(p);
            p.clear();
            //bottom left
            p.append(QPoint(0, 0));
            p.append(QPoint(0, centeredRect.bottom()));
            p.append(centeredRect.bottomLeft());
            p.append(QPoint(centeredRect.left(), 0));

            painter->setBrush(getColor(wangId.cornerColor(2), cornerCount));
            painter->drawPolygon(p);
            p.clear();
            //top left
            p.append(QPoint(0, 0));
            p.append(QPoint(0, centeredRect.top()));
            p.append(centeredRect.topLeft());
            p.append(QPoint(centeredRect.left(), 0));

            painter->setBrush(getColor(wangId.cornerColor(3), cornerCount));
            painter->drawPolygon(p);
            p.clear();
        }
    }

    //paints edges
    if (edgeCount > 1) {
        QPolygon p;

        if (cornerCount > 1) {
            //top
            p.append(QPoint(0, 0));
            p.append(QPoint(centeredRect.left()/3, centeredRect.top()/3));
            p.append(QPoint(centeredRect.left()/3, centeredRect.top()));
            p.append(QPoint(centeredRect.right()/3, centeredRect.top()));
            p.append(QPoint(centeredRect.right()/3, centeredRect.top()/3));

            painter->setBrush(getColor(wangId.edgeColor(0), edgeCount));
            painter->drawPolygon(p);
            p.clear();

            //right
            p.append(QPoint(0, 0));
            p.append(QPoint(centeredRect.right()/3, centeredRect.top()/3));
            p.append(QPoint(centeredRect.right(), centeredRect.top()/3));
            p.append(QPoint(centeredRect.right(), centeredRect.bottom()/3));
            p.append(QPoint(centeredRect.right()/3, centeredRect.bottom()/3));

            painter->setBrush(getColor(wangId.edgeColor(1), edgeCount));
            painter->drawPolygon(p);
            p.clear();
            //bottom
            p.append(QPoint(0, 0));
            p.append(QPoint(centeredRect.right()/3, centeredRect.bottom()/3));
            p.append(QPoint(centeredRect.right()/3, centeredRect.bottom()));
            p.append(QPoint(centeredRect.left()/3, centeredRect.bottom()));
            p.append(QPoint(centeredRect.left()/3, centeredRect.bottom()/3));

            painter->setBrush(getColor(wangId.edgeColor(2), edgeCount));
            painter->drawPolygon(p);
            p.clear();
            //left
            p.append(QPoint(0, 0));
            p.append(QPoint(centeredRect.left()/3, centeredRect.top()/3));
            p.append(QPoint(centeredRect.left(), centeredRect.top()/3));
            p.append(QPoint(centeredRect.left(), centeredRect.bottom()/3));
            p.append(QPoint(centeredRect.left()/3, centeredRect.bottom()/3));

            painter->setBrush(getColor(wangId.edgeColor(3), edgeCount));
            painter->drawPolygon(p);
        } else {
            //top
            p.append(QPoint(0, 0));
            p.append(centeredRect.topLeft());
            p.append(centeredRect.topRight());

            painter->setBrush(getColor(wangId.edgeColor(0), edgeCount));
            painter->drawPolygon(p);
            p.clear();
            //right
            p.append(QPoint(0, 0));
            p.append(centeredRect.bottomRight());
            p.append(centeredRect.topRight());

            painter->setBrush(getColor(wangId.edgeColor(1), edgeCount));
            painter->drawPolygon(p);
            p.clear();
            //bottom
            p.append(QPoint(0, 0));
            p.append(centeredRect.bottomRight());
            p.append(centeredRect.bottomLeft());

            painter->setBrush(getColor(wangId.edgeColor(2), edgeCount));
            painter->drawPolygon(p);
            p.clear();
            //left
            p.append(QPoint(0, 0));
            p.append(centeredRect.topLeft());
            p.append(centeredRect.bottomLeft());

            painter->setBrush(getColor(wangId.edgeColor(3), edgeCount));
            painter->drawPolygon(p);
        }
    } else if (cornerCount <= 1) {
        painter->drawRect(centeredRect);
    }

    painter->restore();
}

void WangTemplateDelegate::paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    const WangTemplateModel *model = static_cast<const WangTemplateModel*>(index.model());
    const WangId wangId = model->wangIdAt(index);
    if(wangId == 0)
        return;

    if (WangSet *wangSet = mWangTemplateView->wangSet())
        paintTemplateTile(painter, wangId,
                      wangSet->edgeColors(),
                      wangSet->cornerColors(),
                      option.rect);
    else
        paintTemplateTile(painter, wangId, 1, 1, option.rect);

    //Highlight currently selected tile.
    if (mWangTemplateView->currentIndex() == index) {
        painter->setBrush(QColor(0,0,255,100));
        painter->setPen(Qt::NoPen);
        painter->drawRect(option.rect);
    }

    //Shade tile if used already
    if (mWangTemplateView->wangIdIsUsed(wangId)) {
        painter->setBrush(QColor(0,0,0,100));
        painter->setPen(Qt::NoPen);
        painter->drawRect(option.rect.adjusted(2,2,-2,-2));
    }
}

QSize WangTemplateDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    //
    return QSize(32 * mWangTemplateView->scale(),
                 32 * mWangTemplateView->scale());
}

} // anonymous namespace

WangTemplateView::WangTemplateView(QWidget *parent)
    : QListView(parent)
    , mZoomable(new Zoomable(this))
{
    setWrapping(true);
    setFlow(QListView::LeftToRight);
    setResizeMode(QListView::Adjust);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setItemDelegate(new WangTemplateDelegate(this, this));
    setTabKeyNavigation(true);

    connect(mZoomable, &Zoomable::scaleChanged,
            this, &WangTemplateView::adjustScale);
}

qreal WangTemplateView::scale() const
{
    return mZoomable->scale();
}

void WangTemplateView::setModel(QAbstractItemModel *model)
{
    QListView::setModel(model);
    updateBackgroundColor();
}

void WangTemplateView::updateBackgroundColor()
{
    QColor base = QApplication::palette().dark().color();

    QPalette p = palette();
    p.setColor(QPalette::Base, base);
    setPalette(p);
}

WangSet *WangTemplateView::wangSet() const
{
    WangTemplateModel *model = wangTemplateModel();
    if (model)
        return model->wangSet();
    else
        return nullptr;
}

bool WangTemplateView::wangIdIsUsed(WangId wangId) const
{
    if (WangSet *set = wangSet())
        return set->wangIdIsUsed(wangId);

    return false;
}

bool WangTemplateView::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture) {
        QGestureEvent *gestureEvent = static_cast<QGestureEvent *>(event);
        if (QGesture *gesture = gestureEvent->gesture(Qt::PinchGesture))
            mZoomable->handlePinchGesture(static_cast<QPinchGesture *>(gesture));
    } else if (event->type() == QEvent::ShortcutOverride) {
        auto keyEvent = static_cast<QKeyEvent*>(event);
        if (Utils::isZoomInShortcut(keyEvent) ||
                Utils::isZoomOutShortcut(keyEvent) ||
                Utils::isResetZoomShortcut(keyEvent)) {
            event->accept();
            return true;
        }
    }

    return QListView::event(event);
}

void WangTemplateView::keyPressEvent(QKeyEvent *event)
{
    if (Utils::isZoomInShortcut(event)) {
        mZoomable->zoomIn();
        return;
    }
    if (Utils::isZoomOutShortcut(event)) {
        mZoomable->zoomOut();
        return;
    }
    if (Utils::isResetZoomShortcut(event)) {
        mZoomable->resetZoom();
        return;
    }
    return QListView::keyPressEvent(event);
}

void WangTemplateView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier &&
            event->orientation() == Qt::Vertical)
    {
        mZoomable->handleWheelDelta(event->delta());
        return;
    }

    QListView::wheelEvent(event);
}

void WangTemplateView::adjustScale()
{
    if (WangTemplateModel *model = wangTemplateModel())
        model->resetModel();
}
