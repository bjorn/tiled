/*
 * mapobjectmodel.cpp
 * Copyright 2012, Tim Baker <treectrl@hotmail.com>
 * Copyright 2012-2017, Thorbjørn Lindeijer <bjorn@lindeijer.nl>
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

#include "mapobjectmodel.h"

#include "changelayer.h"
#include "changemapobject.h"
#include "grouplayer.h"
#include "layermodel.h"
#include "map.h"
#include "mapdocument.h"
#include "objectgroup.h"
#include "renamelayer.h"
#include "invertyaxishelper.h"

#include <QApplication>
#include <QPalette>
#include <QStyle>

using namespace Tiled;

MapObjectModel::MapObjectModel(QObject *parent)
    : QAbstractItemModel(parent)
    , mMapDocument(nullptr)
    , mMap(nullptr)
    , mObjectGroupIcon(QLatin1String(":/images/16x16/layer-object.png"))
    , mRectangleIcon(QLatin1String(":images/24x24/object-rectangle.png"))
    , mImageIcon(QLatin1String(":images/24x24/object-image.png"))
    , mPolygonIcon(QLatin1String(":images/24x24/object-polygon.png"))
    , mPolylineIcon(QLatin1String(":images/24x24/object-polyline.png"))
    , mEllipseIcon(QLatin1String(":images/24x24/object-ellipse.png"))
    , mTextIcon(QLatin1String(":images/24x24/object-text.png"))
    , mPointIcon(QLatin1String(":images/24x24/object-point.png"))
{
    mObjectGroupIcon.addFile(QLatin1String(":images/32x32/layer-object.png"));
}

QModelIndex MapObjectModel::index(int row, int column,
                                  const QModelIndex &parent) const
{
    if (ObjectGroup *objectGroup = toObjectGroup(parent)) {
        if (row < objectGroup->objectCount())
            return createIndex(row, column, objectGroup->objectAt(row));
        return QModelIndex();
    }

    GroupLayer *parentLayer = toGroupLayer(parent); // may be nullptr
    const QList<Layer *> &layers = filteredChildLayers(parentLayer);

    if (row < layers.size())
        return createIndex(row, column, layers.at(row));

    return QModelIndex();
}

QModelIndex MapObjectModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    Object *object = static_cast<Object*>(index.internalPointer());
    switch (object->typeId()) {
    case Object::LayerType:
        if (Layer *layer = static_cast<Layer*>(object)->parentLayer())
            return this->index(layer);
        break;
    case Object::MapObjectType:
        return this->index(static_cast<MapObject*>(object)->objectGroup());
    default:
        break;
    }

    return QModelIndex();
}

int MapObjectModel::rowCount(const QModelIndex &parent) const
{
    if (!mMapDocument)
        return 0;
    if (!parent.isValid())
        return filteredChildLayers(nullptr).size();

    Object *object = static_cast<Object*>(parent.internalPointer());
    if (object->typeId() == Object::LayerType) {
        Layer *layer = static_cast<Layer*>(object);
        switch (layer->layerType()) {
        case Layer::GroupLayerType:
            return filteredChildLayers(static_cast<GroupLayer*>(layer)).size();
        case Layer::ObjectGroupType:
            return static_cast<ObjectGroup*>(layer)->objectCount();
        default:
            break;
        }
    }
    return 0;
}

int MapObjectModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return ColumnCount;
}

QVariant MapObjectModel::data(const QModelIndex &index, int role) const
{
    if (MapObject *mapObject = toMapObject(index)) {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            switch (index.column()) {
            case Name:
                return mapObject->name();
            case Type:
                return mapObject->effectiveType();
            case Id:
                return mapObject->id();
            case Position:
                return QLatin1Char('(')
                        + QString::number(mapObject->x())
                        + QLatin1String(", ")
                        + QString::number(InvertYAxisHelper(mapDocument()).tileY(mapObject->y()))
                        + QLatin1Char(')');
            }
            break;
        case Qt::DecorationRole:
            if (index.column() == Name) {
                switch (mapObject->shape()) {
                case MapObject::Rectangle:
                    return mapObject->isTileObject() ? mImageIcon : mRectangleIcon;
                case MapObject::Polygon:
                    return mPolygonIcon;
                case MapObject::Polyline:
                    return mPolylineIcon;
                case MapObject::Ellipse:
                    return mEllipseIcon;
                case MapObject::Text:
                    return mTextIcon;
                case MapObject::Point:
                    return mPointIcon;
                }
            }
            break;
        case Qt::ForegroundRole:
            if (index.column() == 1) {
                const QPalette palette = QApplication::palette();
                const auto typeColorGroup = mapObject->type().isEmpty() ? QPalette::Disabled
                                                                        : QPalette::Active;
                return palette.brush(typeColorGroup, QPalette::WindowText);
            }
            return QVariant();
        case Qt::CheckStateRole:
            if (index.column() > 0)
                return QVariant();
            return mapObject->isVisible() ? Qt::Checked : Qt::Unchecked;
        case OpacityRole:
            return qreal(1);
        default:
            return QVariant();
        }
    }
    if (Layer *layer = toLayer(index)) {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return index.column() ? QVariant() : layer->name();
        case Qt::DecorationRole:
            if (index.column() == 0) {
                if (layer->isObjectGroup())
                    return mObjectGroupIcon;
                else
                    return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
            }
            return QVariant();
        case Qt::CheckStateRole:
            if (index.column() > 0)
                return QVariant();
            return layer->isVisible() ? Qt::Checked : Qt::Unchecked;
        case OpacityRole:
            return layer->opacity();
        default:
            return QVariant();
        }
    }
    return QVariant();
}

bool MapObjectModel::setData(const QModelIndex &index, const QVariant &value,
                             int role)
{
    if (MapObject *mapObject = toMapObject(index)) {
        switch (role) {
        case Qt::CheckStateRole: {
            Qt::CheckState c = static_cast<Qt::CheckState>(value.toInt());
            const bool visible = (c == Qt::Checked);
            if (visible != mapObject->isVisible()) {
                QUndoCommand *command = new ChangeMapObject(mMapDocument,
                                                            mapObject,
                                                            MapObject::VisibleProperty,
                                                            visible);
                mMapDocument->undoStack()->push(command);
            }
            return true;
        }
        case Qt::EditRole: {
            const QString s = value.toString();
            if (index.column() == 0 && s != mapObject->name()) {
                QUndoStack *undo = mMapDocument->undoStack();
                undo->beginMacro(tr("Change Object Name"));
                undo->push(new ChangeMapObject(mMapDocument, mapObject,
                                               MapObject::NameProperty, s));
                undo->endMacro();
            }
            if (index.column() == 1 && s != mapObject->type()) {
                QUndoStack *undo = mMapDocument->undoStack();
                undo->beginMacro(tr("Change Object Type"));
                undo->push(new ChangeMapObject(mMapDocument, mapObject,
                                               MapObject::TypeProperty, s));
                undo->endMacro();
            }
            return true;
        }
        }
        return false;
    }
    if (Layer *layer = toLayer(index)) {
        switch (role) {
        case Qt::CheckStateRole: {
            Qt::CheckState c = static_cast<Qt::CheckState>(value.toInt());
            const bool visible = (c == Qt::Checked);
            if (visible != layer->isVisible()) {
                QUndoCommand *command = new SetLayerVisible(mMapDocument,
                                                            layer,
                                                            visible);
                mMapDocument->undoStack()->push(command);
            }
            return true;
        }
        case Qt::EditRole: {
            const QString newName = value.toString();
            if (layer->name() != newName) {
                RenameLayer *rename = new RenameLayer(mMapDocument, layer,
                                                      newName);
                mMapDocument->undoStack()->push(rename);
            }
            return true;
        }
        }
        return false;
    }
    return false;
}

Qt::ItemFlags MapObjectModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags rc = QAbstractItemModel::flags(index);
    if (index.column() == 0)
        rc |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
    else if (toMapObject(index)) {
        if (index.column() == Type)// allow to edit only type column
            rc |= Qt::ItemIsEditable;
    }
    return rc;
}

QVariant MapObjectModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case Name: return tr("Name");
        case Type: return tr("Type");
        case Id: return tr("ID");
        case Position: return tr("Position");
        }
    }
    return QVariant();
}

QModelIndex MapObjectModel::index(Layer *layer) const
{
    Q_ASSERT(layer);
    Q_ASSERT(layer->isObjectGroup() || layer->isGroupLayer());
    Q_ASSERT(layer->map() == mMap);

    const int row = filteredChildLayers(layer->parentLayer()).indexOf(layer);
    return createIndex(row, 0, layer);
}

QModelIndex MapObjectModel::index(MapObject *mapObject, int column) const
{
    Q_ASSERT(mapObject);
    Q_ASSERT(mapObject->objectGroup());
    Q_ASSERT(mapObject->objectGroup()->map() == mMap);

    const int row = mapObject->objectGroup()->objects().indexOf(mapObject);
    return createIndex(row, column, mapObject);
}

Layer *MapObjectModel::toLayer(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    Object *object = static_cast<Object*>(index.internalPointer());
    if (object->typeId() == Object::LayerType)
        return static_cast<Layer*>(object);

    return nullptr;
}

MapObject *MapObjectModel::toMapObject(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    Object *object = static_cast<Object*>(index.internalPointer());
    if (object->typeId() == Object::MapObjectType)
        return static_cast<MapObject*>(object);

    return nullptr;
}

ObjectGroup *MapObjectModel::toObjectGroup(const QModelIndex &index) const
{
    if (Layer *layer = toLayer(index))
        return layer->asObjectGroup();
    return nullptr;
}

GroupLayer *MapObjectModel::toGroupLayer(const QModelIndex &index) const
{
    if (Layer *layer = toLayer(index))
        return layer->asGroupLayer();
    return nullptr;
}

ObjectGroup *MapObjectModel::toObjectGroupContext(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    Object *object = static_cast<Object*>(index.internalPointer());
    if (object->typeId() == Object::LayerType) {
        if (auto objectGroup = static_cast<Layer*>(object)->asObjectGroup())
            return objectGroup;
    } else if (object->typeId() == Object::MapObjectType) {
        return static_cast<MapObject*>(object)->objectGroup();
    }

    return nullptr;
}

void MapObjectModel::setMapDocument(MapDocument *mapDocument)
{
    if (mMapDocument == mapDocument)
        return;

    if (mMapDocument)
        mMapDocument->disconnect(this);

    beginResetModel();
    mMapDocument = mapDocument;
    mMap = nullptr;

    mFilteredLayers.clear();

    if (mMapDocument) {
        mMap = mMapDocument->map();

        connect(mMapDocument, &MapDocument::layerAdded,
                this, &MapObjectModel::layerAdded);
        connect(mMapDocument, &MapDocument::layerChanged,
                this, &MapObjectModel::layerChanged);
        connect(mMapDocument, &MapDocument::layerAboutToBeRemoved,
                this, &MapObjectModel::layerAboutToBeRemoved);
        connect(mMapDocument, &MapDocument::tileTypeChanged,
                this, &MapObjectModel::tileTypeChanged);
    }

    endResetModel();
}

void MapObjectModel::layerAdded(Layer *layer)
{
    if (layer->isObjectGroup() || layer->isGroupLayer()) {
        const auto &siblings = layer->siblings();

        Layer *prev = nullptr;
        for (int i = siblings.indexOf(layer) - 1; i >= 0; --i) {
            auto sibling = siblings.at(i);
            if (sibling->isObjectGroup() || sibling->isGroupLayer()) {
                prev = sibling;
                break;
            }
        }

        auto &filtered = filteredChildLayers(layer->parentLayer());
        int row = prev ? filtered.indexOf(prev) + 1 : 0;

        QModelIndex parent;
        if (layer->parentLayer())
            parent = index(layer->parentLayer());

        beginInsertRows(parent, row, row);
        filtered.insert(row, layer);
        endInsertRows();
    }
}

void MapObjectModel::layerChanged(Layer *layer)
{
    if (layer->isObjectGroup() || layer->isGroupLayer()) {
        QModelIndex index = this->index(layer);
        emit dataChanged(index, index);
    }
}

void MapObjectModel::layerAboutToBeRemoved(GroupLayer *groupLayer, int index)
{
    const auto &layers = groupLayer ? groupLayer->layers() : mMap->layers();
    Layer *layer = layers.at(index);

    if (layer->isObjectGroup() || layer->isGroupLayer()) {
        auto &filtered = filteredChildLayers(groupLayer);
        const int row = filtered.indexOf(layer);

        QModelIndex parent = groupLayer ? this->index(groupLayer) : QModelIndex();

        beginRemoveRows(parent, row, row);
        filtered.removeAt(row);
        endRemoveRows();
    }
}

void MapObjectModel::tileTypeChanged(Tile *tile)
{
    LayerIterator it(mMap);

    while (Layer *layer = it.next()) {
        if (ObjectGroup *objectGroup = layer->asObjectGroup()) {
            for (MapObject *mapObject : objectGroup->objects()) {
                if (!mapObject->type().isEmpty())
                    continue;

                const auto &cell = mapObject->cell();
                if (cell.tileset() == tile->tileset() && cell.tileId() == tile->id()) {
                    QModelIndex index = this->index(mapObject, 1);
                    emit dataChanged(index, index);
                }
            }
        }
    }
}

void MapObjectModel::emitObjectsChanged(const QList<MapObject *> &objects,
                                        const QList<Column> &columns,
                                        const QVector<int> &roles)
{
    emit objectsChanged(objects);
    if (columns.isEmpty())
        return;

    auto minMaxPair = std::minmax_element(columns.begin(), columns.end());
    for (auto object : objects) {
        emit dataChanged(index(object, *minMaxPair.first),
                         index(object, *minMaxPair.second),
                         roles);
    }
}

void MapObjectModel::emitObjectsChanged(const QList<MapObject *> &objects, Column column)
{
    emitObjectsChanged(objects,
                       QList<MapObjectModel::Column>() << column);
}

QList<Layer *> &MapObjectModel::filteredChildLayers(GroupLayer *parentLayer) const
{
    if (!mFilteredLayers.contains(parentLayer)) {
        QList<Layer*> &filtered = mFilteredLayers[parentLayer];
        const auto &layers = parentLayer ? parentLayer->layers() : mMap->layers();
        for (Layer *layer : layers)
            if (layer->isObjectGroup() || layer->isGroupLayer())
                filtered.append(layer);
        return filtered;
    }

    return mFilteredLayers[parentLayer];
}

void MapObjectModel::insertObject(ObjectGroup *og, int index, MapObject *o)
{
    const int row = (index >= 0) ? index : og->objectCount();
    beginInsertRows(this->index(og), row, row);
    og->insertObject(row, o);
    endInsertRows();
    emit objectsAdded(QList<MapObject*>() << o);
}

int MapObjectModel::removeObject(ObjectGroup *og, MapObject *o)
{
    QList<MapObject*> objects;
    objects << o;

    const int row = og->objects().indexOf(o);
    beginRemoveRows(index(og), row, row);
    og->removeObjectAt(row);
    endRemoveRows();
    emit objectsRemoved(objects);
    return row;
}

void MapObjectModel::moveObjects(ObjectGroup *og, int from, int to, int count)
{
    const QModelIndex parent = index(og);
    if (!beginMoveRows(parent, from, from + count - 1, parent, to)) {
        Q_ASSERT(false); // The code should never attempt this
        return;
    }

    og->moveObjects(from, to, count);
    endMoveRows();
}

void MapObjectModel::setObjectPolygon(MapObject *o, const QPolygonF &polygon)
{
    if (o->polygon() == polygon)
        return;

    o->setPolygon(polygon);
    emit objectsChanged(QList<MapObject*>() << o);
}

void MapObjectModel::setObjectPosition(MapObject *o, const QPointF &pos)
{
    if (o->position() == pos)
        return;

    o->setPosition(pos);
    emit objectsChanged(QList<MapObject*>() << o);
}

void MapObjectModel::setObjectSize(MapObject *o, const QSizeF &size)
{
    if (o->size() == size)
        return;

    o->setSize(size);
    emit objectsChanged(QList<MapObject*>() << o);
}

void MapObjectModel::setObjectRotation(MapObject *o, qreal rotation)
{
    if (o->rotation() == rotation)
        return;

    o->setRotation(rotation);
    emit objectsChanged(QList<MapObject*>() << o);
}

void MapObjectModel::setObjectProperty(MapObject *o,
                                       MapObject::Property property,
                                       const QVariant &value)
{
    if (o->mapObjectProperty(property) == value)
        return;

    o->setMapObjectProperty(property, value);

    QList<MapObject*> objects = QList<MapObject*>() << o;

    // Notify views about certain property changes
    switch (property) {
    case MapObject::NameProperty:
    case MapObject::VisibleProperty: {
        QModelIndex index = this->index(o, 0);
        emit dataChanged(index, index);
        break;
    }
    case MapObject::TypeProperty: {
        QModelIndex index = this->index(o, 1);
        emit dataChanged(index, index);
        emit objectsTypeChanged(objects);
        break;
    }
    default:
        break;
    }

    emit objectsChanged(objects);
}
