/*
 * invertyaxishelper.h
 * Copyright 2013, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2018, Adrian Frances <adrianfranceslillo@gmail.com>
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

#pragma once

#include "mapdocument.h"
#include "maprenderer.h"
#include "preferences.h"

namespace Tiled {

class InvertYAxisHelper
{
public:
    InvertYAxisHelper(MapDocument *mapDocument)
        : mMapDocument(mapDocument)
    {}
                
    int tileY(int y) const;
    qreal pixelY(qreal y) const;

private:
    MapDocument *mMapDocument;
};

/**
 * Inverts Y coordinate in tiles when applicable.
 */
inline int InvertYAxisHelper::tileY(int y) const
{
    // Check if Invert Y Axis is set
    if (Preferences::instance()->invertYAxis())
        return mMapDocument->map()->height() - 1 - y;
    return y;
}

/**
 * Inverts Y coordinate in pixels when applicable.
 */
inline qreal InvertYAxisHelper::pixelY(qreal y) const
{
    // Obtain the map document
    if (Preferences::instance()->invertYAxis()) {
        const MapRenderer *renderer = mMapDocument->renderer();
        QRect boundingRect = renderer->boundingRect(QRect(QPoint(), mMapDocument->map()->size()));
        return boundingRect.height() - y;
    }
    return y;
}

} // Namespace Tiled
