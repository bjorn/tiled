/*
 * snaphelper.h
 * Copyright 2015, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#ifndef SNAPHELPER_H
#define SNAPHELPER_H

#include "maprenderer.h"

namespace Tiled {
namespace Internal {

class SnapHelper
{
public:
    SnapHelper(const MapRenderer *renderer, Qt::KeyboardModifiers modifiers = 0);

    bool snaps() const { return mSnapToGrid || mSnapToFineGrid; }

    void snap(QPointF &pixelPos) const;

private:
    const MapRenderer *mRenderer;
    bool mSnapToGrid;
    bool mSnapToFineGrid;
};

} // namespace Internal
} // namespace Tiled

#endif // SNAPHELPER_H
