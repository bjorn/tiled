/*
 * mapview.cpp
 * Copyright 2008-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "mapview.h"

#include "flexiblescrollbar.h"
#include "mapdocument.h"
#include "mapobject.h"
#include "maprenderer.h"
#include "mapscene.h"
#include "objectgroup.h"
#include "preferences.h"
#include "tileanimationdriver.h"
#include "utils.h"
#include "zoomable.h"

#include <QApplication>
#include <QCursor>
#include <QGesture>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QScrollBar>
#include <QWheelEvent>

#ifndef QT_NO_OPENGL
#include <QOpenGLWidget>
#endif

using namespace Tiled;

Preference<bool> MapView::ourAutoScrollingEnabled { "Interface/AutoScrolling", false };
Preference<bool> MapView::ourSmoothScrollingEnabled { "Interface/SmoothScrolling", true };

MapView::MapView(QWidget *parent, Mode mode)
    : QGraphicsView(parent)
    , mMode(mode)
    , mZoomable(new Zoomable(this))
    , mPanningDriver(new TileAnimationDriver(this))
{
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
#ifdef Q_OS_MAC
    setFrameStyle(QFrame::NoFrame);
#endif

#ifndef QT_NO_OPENGL
    Preferences *prefs = Preferences::instance();
    setUseOpenGL(prefs->useOpenGL());
    connect(prefs, &Preferences::useOpenGLChanged, this, &MapView::setUseOpenGL);
#endif

    QWidget *v = viewport();

    /* Since Qt 4.5, setting this attribute yields significant repaint
     * reduction when the view is being resized. */
    if (mMode == StaticContents)
        v->setAttribute(Qt::WA_StaticContents);

    /* Since Qt 4.6, mouse tracking is disabled when no graphics item uses
     * hover events. We need to set it since our scene wants the events. */
    v->setMouseTracking(true);

    // Adjustment for antialiasing is done by the items that need it
    setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing);

    grabGesture(Qt::PinchGesture);

    setVerticalScrollBar(new FlexibleScrollBar(Qt::Vertical, this));
    setHorizontalScrollBar(new FlexibleScrollBar(Qt::Horizontal, this));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    connect(horizontalScrollBar(), &QAbstractSlider::valueChanged, this, &MapView::updateViewRect);
    connect(horizontalScrollBar(), &QAbstractSlider::rangeChanged, this, &MapView::updateViewRect);
    connect(verticalScrollBar(), &QAbstractSlider::valueChanged, this, &MapView::updateViewRect);
    connect(verticalScrollBar(), &QAbstractSlider::rangeChanged, this, &MapView::updateViewRect);

    connect(mZoomable, &Zoomable::scaleChanged, this, &MapView::adjustScale);

    connect(mPanningDriver, &TileAnimationDriver::update, this, &MapView::updatePanning);
}

MapView::~MapView()
{
    setScrollingMode(NoScrolling); // Just in case we didn't get a hide event
}

void MapView::setScene(MapScene *scene)
{
    if (MapScene *currentScene = mapScene())
        currentScene->disconnect(this);

    QGraphicsView::setScene(scene);

    if (scene) {
        updateSceneRect(scene->sceneRect());
        connect(scene, &MapScene::mapDocumentChanged,
                this, &MapView::setMapDocument);
    }

    setMapDocument(scene ? scene->mapDocument() : nullptr);
}

MapScene *MapView::mapScene() const
{
    return static_cast<MapScene*>(scene());
}

qreal MapView::scale() const
{
    return mZoomable->scale();
}

void MapView::setScale(qreal scale)
{
    mZoomable->setScale(scale);
}

void MapView::fitMapInView()
{
    MapScene *scene = mapScene();
    if (!scene)
        return;

    const QRectF rect = scene->mapBoundingRect();
    if (rect.isEmpty())
        return;

    // Scale and center map to fit in view, while avoiding to go below the
    // minimum scale. For extremely large maps, avoid putting more than about
    // 256 * 256 tiles within the view for performance reasons.
    qreal desiredScale = std::min(width() / rect.width(),
                                  height() / rect.height()) * 0.95;

    auto tileSize = mMapDocument->map()->tileSize();
    qreal scale256 = std::min(width() / (256.0 * tileSize.width()),
                              height() / (256.0 * tileSize.height()));

    centerOn(rect.center());
    setScale(std::max(std::max(desiredScale, scale256), 0.015625));
}

void MapView::adjustScale(qreal scale)
{
    const QTransform newTransform = QTransform::fromScale(scale, scale);

    bool sceneRectUpdated = false;
    if (scale < transform().m11()) {
        // When zooming out, we first need to expand the scene rect
        updateSceneRect(scene()->sceneRect(), newTransform);
        sceneRectUpdated = true;
    }

    setTransform(newTransform);

    if (!sceneRectUpdated)
        updateSceneRect(scene()->sceneRect());

    setRenderHint(QPainter::SmoothPixmapTransform,
                  mZoomable->smoothTransform());

    updateViewRect();
}

void MapView::setUseOpenGL(bool useOpenGL)
{
#ifndef QT_NO_OPENGL
    if (useOpenGL) {
        if (!qobject_cast<QOpenGLWidget*>(viewport())) {
            QSurfaceFormat format = QSurfaceFormat::defaultFormat();
            format.setDepthBufferSize(0);   // No need for a depth buffer
            format.setSamples(4);           // Enable anti-aliasing

            QOpenGLWidget *openGLWidget = new QOpenGLWidget(this);
            openGLWidget->setFormat(format);
            setViewport(openGLWidget);
        }
    } else {
        if (qobject_cast<QOpenGLWidget*>(viewport()))
            setViewport(nullptr);
    }

    QWidget *v = viewport();
    if (mMode == StaticContents)
        v->setAttribute(Qt::WA_StaticContents);
    v->setMouseTracking(true);
#else
    Q_UNUSED(useOpenGL)
#endif
}

void MapView::updateSceneRect(const QRectF &sceneRect)
{
    updateSceneRect(sceneRect, transform());
}

void MapView::updateSceneRect(const QRectF &sceneRect, const QTransform &transform)
{
    // Calculate a scene rect that includes a margin on the edge of the map,
    // taking into account the scale
    const QSize maxSize = maximumViewportSize();
    const qreal marginWidth = maxSize.width() * 0.9;
    const qreal marginHeight = maxSize.height() * 0.9;

    QRectF viewRect = transform.mapRect(sceneRect);
    viewRect.adjust(-marginWidth, -marginHeight, marginWidth, marginHeight);
    const QRectF expandedSceneRect = transform.inverted().mapRect(viewRect);

    setSceneRect(expandedSceneRect);
}

void MapView::updateViewRect()
{
    const QRectF viewRect = mapToScene(viewport()->geometry()).boundingRect();
    if (mViewRect == viewRect)
        return;

    mViewRect = viewRect;

    if (MapScene *scene = mapScene())
        scene->setViewRect(viewRect);

    emit viewRectChanged();
}

void MapView::focusMapObject(MapObject *mapObject)
{
    // FIXME: This is not always the visual center
    const QPointF center = mapObject->bounds().center();
    const QPointF screenCoords = mMapDocument->renderer()->pixelToScreenCoords(center);
    forceCenterOn(screenCoords, *mapObject->objectGroup());
}

void MapView::setPanDirections(PanDirections directions)
{
    if (mPanDirections == directions)
        return;

    mPanDirections = directions;
    updatePanningDriverState();
}

void MapView::updatePanningDriverState()
{
    const bool run = (mPanDirections && ourSmoothScrollingEnabled) || mScrollingMode == AutoScrolling;
    if (run && mPanningDriver->state() != QAbstractAnimation::Running)
        mPanningDriver->start();
    else if (!run && mPanningDriver->state() == QAbstractAnimation::Running)
        mPanningDriver->stop();
}

void MapView::updatePanning(int deltaTime)
{
    QPoint distance;

    if (mScrollingMode == AutoScrolling) {
        distance = (mLastMousePos - mScrollStartPos) * deltaTime / 100;
    } else if (mPanDirections && ourSmoothScrollingEnabled) {
        if (mPanDirections & Left)
            distance.rx() -= 1;
        if (mPanDirections & Right)
            distance.rx() += 1;
        if (mPanDirections & Up)
            distance.ry() -= 1;
        if (mPanDirections & Down)
            distance.ry() += 1;

        distance = Utils::dpiScaled(distance * deltaTime / 2);
    }

    scrollBy(distance);
}

void MapView::scrollBy(QPoint distance)
{
    if (distance.isNull())
        return;

    if (distance.x()) {
        auto hor = static_cast<FlexibleScrollBar*>(horizontalScrollBar());
        if (isRightToLeft())
            hor->forceSetValue(hor->value() - distance.x());
        else
            hor->forceSetValue(hor->value() + distance.x());
    }

    if (distance.y()) {
        auto ver = static_cast<FlexibleScrollBar*>(verticalScrollBar());
        ver->forceSetValue(ver->value() + distance.y());
    }

    // When scrolling the mouse does not move, but the view below it does.
    // This affects the mouse scene position, which needs to be updated.
    mLastMouseScenePos = mapToScene(viewport()->mapFromGlobal(mLastMousePos));
}

void MapView::setMapDocument(MapDocument *mapDocument)
{
    if (mMapDocument == mapDocument)
        return;

    if (mMapDocument)
        mMapDocument->disconnect(this);

    mMapDocument = mapDocument;

    if (mapDocument) {
        connect(mapDocument, &MapDocument::focusMapObjectRequested,
                this, &MapView::focusMapObject);
    }
}

void MapView::setScrollingMode(ScrollingMode mode)
{
    if (mScrollingMode == mode)
        return;

    mScrollingMode = mode;
    setInteractive(mode == NoScrolling);

    switch (mScrollingMode) {
    case DragScrolling:
    case AutoScrolling:
        mLastMousePos = QCursor::pos();
        mScrollStartPos = mLastMousePos;
        QApplication::setOverrideCursor(mScrollingMode == DragScrolling ? Qt::ClosedHandCursor : Qt::SizeAllCursor);
        viewport()->grabMouse();
        updatePanningDriverState();
        break;
    case NoScrolling:
        viewport()->releaseMouse();
        QApplication::restoreOverrideCursor();
        updatePanningDriverState();
        break;
    }
}

/**
 * Centers the view on the given scene position, even when this requires
 * extending the range of the scroll bars.
 *
 * This code is based on QGraphicsView::centerOn.
 */
void MapView::forceCenterOn(QPointF pos)
{
    // Let's wait until the initial paint event before we position the view,
    // otherwise layout changes may still affect the position.
    if (!mViewInitialized) {
        mInitialCenterPos = pos;
        mHasInitialCenterPos = true;
        return;
    }

    // This is only to make it update QGraphicsViewPrivate::lastCenterPoint,
    // just in case this is important.
    QGraphicsView::centerOn(pos);

    auto hBar = static_cast<FlexibleScrollBar*>(horizontalScrollBar());
    auto vBar = static_cast<FlexibleScrollBar*>(verticalScrollBar());
    bool hScroll = hBar->minimum() != 0 || hBar->maximum() != 0;
    bool vScroll = vBar->minimum() != 0 || vBar->maximum() != 0;

    qreal width = viewport()->width();
    qreal height = viewport()->height();
    QPointF viewPoint = transform().map(pos);

    if (hScroll) {
        if (isRightToLeft()) {
            qint64 horizontal = 0;
            horizontal += hBar->minimum();
            horizontal += hBar->maximum();
            horizontal -= int(viewPoint.x() - width / 2.0);
            hBar->forceSetValue(static_cast<int>(horizontal));
        } else {
            hBar->forceSetValue(int(viewPoint.x() - width / 2.0));
        }
    }
    if (vScroll)
        vBar->forceSetValue(int(viewPoint.y() - height / 2.0));
}

/**
 * Centers the view on the given \a position on the given \a layer, taking into
 * account that the layer may have an offset and a parallax factor.
 *
 * \sa forceCenterOn(QPointF)
 */
void MapView::forceCenterOn(QPointF position, const Layer &layer)
{
    position += layer.totalOffset();

    if (Preferences::instance()->parallaxEnabled()) {
        const QPointF parallaxFactor = layer.effectiveParallaxFactor();
        if (!qFuzzyIsNull(parallaxFactor.x()))
            position.rx() /= parallaxFactor.x();
        if (!qFuzzyIsNull(parallaxFactor.y()))
            position.ry() /= parallaxFactor.y();
    }

    forceCenterOn(position);
}

bool MapView::event(QEvent *e)
{
    // Ignore space bar events since they're handled by the MainWindow
    if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease) {
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Space) {
            e->ignore();
            return false;
        }
    } else if (e->type() == QEvent::Gesture) {
        QGestureEvent *gestureEvent = static_cast<QGestureEvent *>(e);
        if (QGesture *gesture = gestureEvent->gesture(Qt::PinchGesture)) {
            QPinchGesture *pinch = static_cast<QPinchGesture *>(gesture);
            if (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged)
                handlePinchGesture(pinch);
        }
    } else if (e->type() == QEvent::ShortcutOverride) {
        auto keyEvent = static_cast<QKeyEvent*>(e);
        if (Utils::isZoomInShortcut(keyEvent) ||
                Utils::isZoomOutShortcut(keyEvent) ||
                Utils::isResetZoomShortcut(keyEvent)) {
            e->accept();
            return true;
        }
    }

    return QGraphicsView::event(e);
}

void MapView::paintEvent(QPaintEvent *event)
{
    if (!mViewInitialized) {
        mViewInitialized = true;

        if (mHasInitialCenterPos)
            forceCenterOn(mInitialCenterPos);
        else
            fitMapInView();
    }

    QGraphicsView::paintEvent(event);
}

void MapView::hideEvent(QHideEvent *event)
{
    // Disable hand scrolling when the view gets hidden in any way
    setScrollingMode(NoScrolling);
    QGraphicsView::hideEvent(event);
}

void MapView::resizeEvent(QResizeEvent *event)
{
    if (QGraphicsScene *s = scene())
        updateSceneRect(s->sceneRect());

    QGraphicsView::resizeEvent(event);
}

void MapView::keyPressEvent(QKeyEvent *event)
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

    // Allow the scene to consume the event (in this case, allows movement of
    // selected objects to take precedence over moving the view).
    QCoreApplication::sendEvent(scene(), event);
    if (event->isAccepted())
        return;

    switch (event->key()) {
    case Qt::Key_Down:
        setPanDirections(mPanDirections | Down);
        break;
    case Qt::Key_Left:
        setPanDirections(mPanDirections | Left);
        break;
    case Qt::Key_Right:
        setPanDirections(mPanDirections | Right);
        break;
    case Qt::Key_Up:
        setPanDirections(mPanDirections | Up);
        break;
    }

    if (!ourSmoothScrollingEnabled) {
        if ((mPanDirections & (Left | Right)) == Left) {
            horizontalScrollBar()->triggerAction(isRightToLeft() ? QScrollBar::SliderSingleStepAdd
                                                                 : QScrollBar::SliderSingleStepSub);
        } else if ((mPanDirections & (Left | Right)) == Right) {
            horizontalScrollBar()->triggerAction(isRightToLeft() ? QScrollBar::SliderSingleStepSub
                                                                 : QScrollBar::SliderSingleStepAdd);
        }
        if ((mPanDirections & (Up | Down)) == Up)
            verticalScrollBar()->triggerAction(QScrollBar::SliderSingleStepSub);
        else if ((mPanDirections & (Up | Down)) == Down)
            verticalScrollBar()->triggerAction(QScrollBar::SliderSingleStepAdd);
    }
}

void MapView::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat()) {
        switch (event->key()) {
        case Qt::Key_Down:
            setPanDirections(mPanDirections & ~Down);
            break;
        case Qt::Key_Left:
            setPanDirections(mPanDirections & ~Left);
            break;
        case Qt::Key_Right:
            setPanDirections(mPanDirections & ~Right);
            break;
        case Qt::Key_Up:
            setPanDirections(mPanDirections & ~Up);
            break;
        }
    }

    QGraphicsView::keyReleaseEvent(event);
}

/**
 * Override to support zooming in and out using the mouse wheel.
 */
void MapView::wheelEvent(QWheelEvent *event)
{
    auto *hBar = static_cast<FlexibleScrollBar*>(horizontalScrollBar());
    auto *vBar = static_cast<FlexibleScrollBar*>(verticalScrollBar());

    bool wheelZoomsByDefault = Preferences::instance()->wheelZoomsByDefault();
    bool control = event->modifiers() & Qt::ControlModifier;

    if ((wheelZoomsByDefault != control) && event->angleDelta().y()) {
        // No automatic anchoring since we'll do it manually
        setTransformationAnchor(QGraphicsView::NoAnchor);

        mZoomable->handleWheelDelta(event->angleDelta().y());
        adjustCenterFromMousePosition(mLastMousePos);

        // Restore the centering anchor
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        return;
    }

    // By default, the scroll area forwards the wheel events to the scroll
    // bars, which apply their bounds. This custom wheel handling is here to
    // override the bounds checking.
    //
    // This also disables QGraphicsSceneWheelEvent, but Tiled does not rely
    // on that event.
    QPoint pixels = event->pixelDelta();

    if (pixels.isNull()) {
        QPointF steps = event->angleDelta() / 8.0 / 15.0;
        int lines = QApplication::wheelScrollLines();
        pixels.setX(int(steps.x() * lines * hBar->singleStep()));
        pixels.setY(int(steps.y() * lines * vBar->singleStep()));
    } else {
        pixels = Utils::dpiScaled(pixels);
    }

    scrollBy(-pixels);
}

/**
 * Activates hand scrolling when the middle mouse button is pressed.
 */
void MapView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && isActiveWindow()) {
        setScrollingMode(ourAutoScrollingEnabled ? AutoScrolling : DragScrolling);
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

/**
 * Deactivates hand scrolling when the middle mouse button is released.
 */
void MapView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        setScrollingMode(NoScrolling);
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void MapView::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    emit focused();
}

/**
 * Moves the view with the mouse while hand scrolling.
 */
void MapView::mouseMoveEvent(QMouseEvent *event)
{
    switch (mScrollingMode) {
    case DragScrolling: {
        auto *hBar = static_cast<FlexibleScrollBar*>(horizontalScrollBar());
        auto *vBar = static_cast<FlexibleScrollBar*>(verticalScrollBar());
        const QPoint d = event->globalPos() - mLastMousePos;

        int horizontalValue = hBar->value() + (isRightToLeft() ? d.x() : -d.x());
        int verticalValue = vBar->value() - d.y();

        // Panning can freely move the map without restriction on boundaries
        hBar->forceSetValue(horizontalValue);
        vBar->forceSetValue(verticalValue);

        mLastMousePos = event->globalPos();
        return;
    }
    case AutoScrolling:
    case NoScrolling:
        break;
    }

    if (event->buttons() & Qt::LeftButton) {
        auto *hBar = static_cast<FlexibleScrollBar*>(horizontalScrollBar());
        auto *vBar = static_cast<FlexibleScrollBar*>(verticalScrollBar());

        QPoint pos = event->pos();
        QRectF mRect = viewport()->rect();

        int horizontalValue = 0,  verticalValue = 0;

        if ((pos.x() + 5) > mRect.width()) {
            horizontalValue = hBar->value() + 5;
            cursor().setPos(mLastMousePos - *(new QPoint(5, 0)));
        }

        if ((pos.x() - 5) < 0) {
            horizontalValue = hBar->value() + (hBar->value() <= 0? 0 : -5);
            cursor().setPos(mLastMousePos + *(new QPoint(5, 0)));
        }

        if ((pos.y() + 5) > mRect.height()) {
            verticalValue = vBar->value() + 5;
            cursor().setPos(mLastMousePos - *(new QPoint(0, 5)));
        }

        if ((pos.y() - 5) < 0) {
            verticalValue = vBar->value() + (vBar->value() <= 0 ? 0 : -5);
            cursor().setPos(mLastMousePos + *(new QPoint(0, 5)));
        }

        if (horizontalValue) {
            hBar->forceSetValue(horizontalValue);
        }

        if (verticalValue) {
            vBar->forceSetValue(verticalValue);
        }

    }


    QGraphicsView::mouseMoveEvent(event);
    mLastMousePos = event->globalPos();
    mLastMouseScenePos = mapToScene(viewport()->mapFromGlobal(mLastMousePos));
}

void MapView::handlePinchGesture(QPinchGesture *pinch)
{
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    mZoomable->handlePinchGesture(pinch);

    QPoint centerPoint = pinch->hotSpot().toPoint();
    adjustCenterFromMousePosition(centerPoint);

    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
}

void MapView::adjustCenterFromMousePosition(QPoint mousePos)
{
    // Place the last known mouse scene pos below the mouse again
    QWidget *view = viewport();
    QPointF viewCenterScenePos = mapToScene(view->rect().center());
    QPointF mouseScenePos = mapToScene(view->mapFromGlobal(mousePos));
    QPointF diff = viewCenterScenePos - mouseScenePos;
    QGraphicsView::centerOn(mLastMouseScenePos + diff);
}

#include "moc_mapview.cpp"
