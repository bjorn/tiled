/*
 * rotatemapobject.cpp
 * Copyright 2011, Przemysław Grzywacz <nexather@gmail.com>
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

#include "rotatemapobject.h"

#include "mapdocument.h"
#include "mapobject.h"

#include <QCoreApplication>

namespace Tiled {
namespace Internal {


RotateMapObject::RotateMapObject(MapDocument *mapDocument,
                MapObject *mapObject,
                qreal oldAngle)
    : mMapDocument(mapDocument),
      mMapObject(mapObject),
      mOldAngle(oldAngle),
      mNewAngle(mapObject->angle())
{
    setText(QCoreApplication::translate("Undo Commands", "Rotate Object"));
}

void RotateMapObject::undo()
{
    mMapObject->setAngle(mOldAngle);
    mMapDocument->emitObjectChanged(mMapObject);
}

void RotateMapObject::redo() {
    mMapObject->setAngle(mNewAngle);
    mMapDocument->emitObjectChanged(mMapObject);
}


} // namespace Internal
} // namespace Tiled
