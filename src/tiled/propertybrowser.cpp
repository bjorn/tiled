/*
 * propertybrowser.cpp
 * Copyright 2013, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
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

#include "propertybrowser.h"

#include "changelayer.h"
#include "changeimagelayerposition.h"
#include "changeimagelayerproperties.h"
#include "changemapobject.h"
#include "changemapproperty.h"
#include "changeobjectgroupproperties.h"
#include "changeproperties.h"
#include "changetileimagesource.h"
#include "changetileprobability.h"
#include "flipmapobjects.h"
#include "imagelayer.h"
#include "map.h"
#include "mapdocument.h"
#include "mapobject.h"
#include "movemapobject.h"
#include "objectgroup.h"
#include "preferences.h"
#include "replacetileset.h"
#include "resizemapobject.h"
#include "renamelayer.h"
#include "renameterrain.h"
#include "rotatemapobject.h"
#include "terrain.h"
#include "tile.h"
#include "tilelayer.h"
#include "tilesetchanges.h"
#include "tilesetdocument.h"
#include "tilesetformat.h"
#include "tilesetterrainmodel.h"
#include "tmxmapformat.h"
#include "utils.h"
#include "varianteditorfactory.h"
#include "variantpropertymanager.h"

#include <QtGroupPropertyManager>

#include <QCoreApplication>
#include <QDebug>
#include <QMessageBox>

namespace Tiled {
namespace Internal {

PropertyBrowser::PropertyBrowser(QWidget *parent)
    : QtTreePropertyBrowser(parent)
    , mUpdating(false)
    , mObject(nullptr)
    , mDocument(nullptr)
    , mMapDocument(nullptr)
    , mVariantManager(new VariantPropertyManager(this))
    , mGroupManager(new QtGroupPropertyManager(this))
    , mCustomPropertiesGroup(nullptr)
{
    VariantEditorFactory *variantEditorFactory = new VariantEditorFactory(this);

    setFactoryForManager(mVariantManager, variantEditorFactory);
    setResizeMode(ResizeToContents);
    setRootIsDecorated(false);
    setPropertiesWithoutValueMarked(true);

    mStaggerAxisNames.append(tr("X"));
    mStaggerAxisNames.append(tr("Y"));

    mStaggerIndexNames.append(tr("Odd"));
    mStaggerIndexNames.append(tr("Even"));

    mOrientationNames.append(QCoreApplication::translate("Tiled::Internal::NewMapDialog", "Orthogonal"));
    mOrientationNames.append(QCoreApplication::translate("Tiled::Internal::NewMapDialog", "Isometric"));
    mOrientationNames.append(QCoreApplication::translate("Tiled::Internal::NewMapDialog", "Isometric (Staggered)"));
    mOrientationNames.append(QCoreApplication::translate("Tiled::Internal::NewMapDialog", "Hexagonal (Staggered)"));

    mLayerFormatNames.append(QCoreApplication::translate("PreferencesDialog", "XML"));
    mLayerFormatNames.append(QCoreApplication::translate("PreferencesDialog", "Base64 (uncompressed)"));
    mLayerFormatNames.append(QCoreApplication::translate("PreferencesDialog", "Base64 (gzip compressed)"));
    mLayerFormatNames.append(QCoreApplication::translate("PreferencesDialog", "Base64 (zlib compressed)"));
    mLayerFormatNames.append(QCoreApplication::translate("PreferencesDialog", "CSV"));

    mRenderOrderNames.append(QCoreApplication::translate("PreferencesDialog", "Right Down"));
    mRenderOrderNames.append(QCoreApplication::translate("PreferencesDialog", "Right Up"));
    mRenderOrderNames.append(QCoreApplication::translate("PreferencesDialog", "Left Down"));
    mRenderOrderNames.append(QCoreApplication::translate("PreferencesDialog", "Left Up"));

    mFlippingFlagNames.append(tr("Horizontal"));
    mFlippingFlagNames.append(tr("Vertical"));

    mDrawOrderNames.append(tr("Top Down"));
    mDrawOrderNames.append(tr("Manual"));

    connect(mVariantManager, SIGNAL(valueChanged(QtProperty*,QVariant)),
            SLOT(valueChanged(QtProperty*,QVariant)));

    connect(variantEditorFactory, &VariantEditorFactory::resetProperty,
            this, &PropertyBrowser::resetProperty);

    connect(Preferences::instance(), &Preferences::objectTypesChanged,
            this, &PropertyBrowser::objectTypesChanged);
}

void PropertyBrowser::setObject(Object *object)
{
    if (mObject == object)
        return;

    removeProperties();
    mObject = object;

    addProperties();
}

void PropertyBrowser::setDocument(Document *document)
{
    MapDocument *mapDocument = qobject_cast<MapDocument*>(document);
    TilesetDocument *tilesetDocument = qobject_cast<TilesetDocument*>(document);

    if (mDocument == document)
        return;

    if (mDocument) {
        mDocument->disconnect(this);
        if (mTilesetDocument)
            mTilesetDocument->terrainModel()->disconnect(this);
    }

    mDocument = document;
    mMapDocument = mapDocument;
    mTilesetDocument = tilesetDocument;

    if (mapDocument) {
        connect(mapDocument, SIGNAL(mapChanged()),
                SLOT(mapChanged()));
        connect(mapDocument, SIGNAL(objectsChanged(QList<MapObject*>)),
                SLOT(objectsChanged(QList<MapObject*>)));
        connect(mapDocument, SIGNAL(objectsTypeChanged(QList<MapObject*>)),
                SLOT(objectsTypeChanged(QList<MapObject*>)));
        connect(mapDocument, SIGNAL(layerChanged(int)),
                SLOT(layerChanged(int)));
        connect(mapDocument, SIGNAL(objectGroupChanged(ObjectGroup*)),
                SLOT(objectGroupChanged(ObjectGroup*)));
        connect(mapDocument, SIGNAL(imageLayerChanged(ImageLayer*)),
                SLOT(imageLayerChanged(ImageLayer*)));

        connect(mapDocument, &MapDocument::selectedObjectsChanged,
                this, &PropertyBrowser::selectedObjectsChanged);
    }

    if (tilesetDocument) {
        connect(tilesetDocument, &TilesetDocument::tilesetFileNameChanged,
                this, &PropertyBrowser::tilesetChanged);
        connect(tilesetDocument, &TilesetDocument::tilesetNameChanged,
                this, &PropertyBrowser::tilesetChanged);
        connect(tilesetDocument, &TilesetDocument::tilesetTileOffsetChanged,
                this, &PropertyBrowser::tilesetChanged);
        connect(tilesetDocument, &TilesetDocument::tilesetChanged,
                this, &PropertyBrowser::tilesetChanged);

        connect(tilesetDocument, &TilesetDocument::tileProbabilityChanged,
                this, &PropertyBrowser::tileChanged);
        connect(tilesetDocument, &TilesetDocument::tileImageSourceChanged,
                this, &PropertyBrowser::tileChanged);

        TilesetTerrainModel *terrainModel = tilesetDocument->terrainModel();
        connect(terrainModel, &TilesetTerrainModel::terrainChanged,
                this, &PropertyBrowser::terrainChanged);
    }

    if (document) {
        // For custom properties:
        connect(document, &Document::propertyAdded,
                this, &PropertyBrowser::propertyAdded);
        connect(document, &Document::propertyRemoved,
                this, &PropertyBrowser::propertyRemoved);
        connect(document, &Document::propertyChanged,
                this, &PropertyBrowser::propertyChanged);
        connect(document, &Document::propertiesChanged,
                this, &PropertyBrowser::propertiesChanged);

        connect(document, &Document::selectedTilesChanged,
                this, &PropertyBrowser::selectedTilesChanged);
    }
}

bool PropertyBrowser::isCustomPropertyItem(QtBrowserItem *item) const
{
    return item && mPropertyToId[item->property()] == CustomProperty;
}

void PropertyBrowser::editCustomProperty(const QString &name)
{
    QtVariantProperty *property = mNameToProperty.value(name);
    if (!property)
        return;

    const QList<QtBrowserItem*> propertyItems = items(property);
    if (!propertyItems.isEmpty())
        editItem(propertyItems.first());
}

bool PropertyBrowser::event(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();

    return QtTreePropertyBrowser::event(event);
}

void PropertyBrowser::mapChanged()
{
    if (mObject == mMapDocument->map())
        updateProperties();
}

void PropertyBrowser::objectsChanged(const QList<MapObject *> &objects)
{
    if (mObject && mObject->typeId() == Object::MapObjectType)
        if (objects.contains(static_cast<MapObject*>(mObject)))
            updateProperties();
}

void PropertyBrowser::objectsTypeChanged(const QList<MapObject *> &objects)
{
    if (mObject && mObject->typeId() == Object::MapObjectType)
        if (objects.contains(static_cast<MapObject*>(mObject)))
            updateCustomProperties();
}

void PropertyBrowser::layerChanged(int index)
{
    if (mObject == mMapDocument->map()->layerAt(index))
        updateProperties();
}

void PropertyBrowser::objectGroupChanged(ObjectGroup *objectGroup)
{
    if (mObject == objectGroup)
        updateProperties();
}

void PropertyBrowser::imageLayerChanged(ImageLayer *imageLayer)
{
    if (mObject == imageLayer)
        updateProperties();
}

void PropertyBrowser::tilesetChanged(Tileset *tileset)
{
    if (mObject == tileset)
        updateProperties();
}

void PropertyBrowser::tileChanged(Tile *tile)
{
    if (mObject == tile)
        updateProperties();
}

void PropertyBrowser::terrainChanged(Tileset *tileset, int index)
{
    if (mObject == tileset->terrain(index))
        updateProperties();
}

void PropertyBrowser::propertyAdded(Object *object, const QString &name)
{
    if (!mDocument->currentObjects().contains(object))
        return;
    if (mNameToProperty.keys().contains(name)) {
        if (mObject == object) {
            mUpdating = true;
            mNameToProperty[name]->setValue(mObject->property(name));
            mUpdating = false;
        }
    } else {
        // Determine the property preceding the new property, if any
        const int index = mObject->properties().keys().indexOf(name);
        const QList<QtProperty *> properties = mCustomPropertiesGroup->subProperties();
        QtProperty *precedingProperty = (index > 0) ? properties.at(index - 1) : nullptr;

        mUpdating = true;
        QVariant value = object->property(name);
        QtVariantProperty *property = mVariantManager->addProperty(value.type(), name);
        property->setValue(value);
        mCustomPropertiesGroup->insertSubProperty(property, precedingProperty);
        mPropertyToId.insert(property, CustomProperty);
        mNameToProperty.insert(name, property);
        mUpdating = false;
    }
    updatePropertyColor(name);
}

void PropertyBrowser::propertyRemoved(Object *object, const QString &name)
{
    if (!mDocument->currentObjects().contains(object))
        return;
    if (mObject == object) {
        bool deleteProperty = true;

        const QList<Object *> currentObjects = mDocument->currentObjects();
        for (Object *obj : currentObjects) {
            if (mObject == obj)
                continue;
            if (obj->properties().contains(name)) {
                // An other selected object still has this property, so just clear the value.
                mUpdating = true;
                mNameToProperty[name]->setValue(tr(""));
                mUpdating = false;
                deleteProperty = false;
                break;
            }
        }

        // No other selected objects have this property so delete it.
        if (deleteProperty)
            delete mNameToProperty.take(name);
    }
    updatePropertyColor(name);
}

void PropertyBrowser::propertyChanged(Object *object, const QString &name)
{
    if (mObject == object) {
        mUpdating = true;
        mNameToProperty[name]->setValue(object->property(name));
        mUpdating = false;
    }
    if (mDocument->currentObjects().contains(object))
        updatePropertyColor(name);
}

void PropertyBrowser::propertiesChanged(Object *object)
{
    if (mDocument->currentObjects().contains(object))
        updateCustomProperties();
}

void PropertyBrowser::selectedObjectsChanged()
{
    updateCustomProperties();
}

void PropertyBrowser::selectedTilesChanged()
{
    updateCustomProperties();
}

void PropertyBrowser::objectTypesChanged()
{
    if (mObject && mObject->typeId() == Object::MapObjectType)
        updateCustomProperties();
}

void PropertyBrowser::valueChanged(QtProperty *property, const QVariant &val)
{
    if (mUpdating)
        return;
    if (!mObject || !mDocument)
        return;
    if (!mPropertyToId.contains(property))
        return;

    const PropertyId id = mPropertyToId.value(property);

    if (id == CustomProperty) {
        QUndoStack *undoStack = mDocument->undoStack();
        undoStack->push(new SetProperty(mDocument,
                                        mDocument->currentObjects(),
                                        property->propertyName(),
                                        val));
        return;
    }

    switch (mObject->typeId()) {
    case Object::MapType:       applyMapValue(id, val); break;
    case Object::MapObjectType: applyMapObjectValue(id, val); break;
    case Object::LayerType:     applyLayerValue(id, val); break;
    case Object::TilesetType:   applyTilesetValue(id, val); break;
    case Object::TileType:      applyTileValue(id, val); break;
    case Object::TerrainType:   applyTerrainValue(id, val); break;
    }
}

void PropertyBrowser::resetProperty(QtProperty *property)
{
    switch (mVariantManager->propertyType(property)) {
    case QVariant::Color:
        // At the moment it is only possible to reset color values
        mVariantManager->setValue(property, QColor());
        break;

    default:
        qWarning() << "Resetting of property type not supported right now";
    }
}

void PropertyBrowser::addMapProperties()
{
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Map"));

    QtVariantProperty *orientationProperty =
            createProperty(OrientationProperty,
                           QtVariantPropertyManager::enumTypeId(),
                           tr("Orientation"),
                           groupProperty);

    orientationProperty->setAttribute(QLatin1String("enumNames"), mOrientationNames);

    createProperty(WidthProperty, QVariant::Int, tr("Width"), groupProperty)->setEnabled(false);
    createProperty(HeightProperty, QVariant::Int, tr("Height"), groupProperty)->setEnabled(false);
    createProperty(TileWidthProperty, QVariant::Int, tr("Tile Width"), groupProperty);
    createProperty(TileHeightProperty, QVariant::Int, tr("Tile Height"), groupProperty);

    createProperty(HexSideLengthProperty, QVariant::Int, tr("Tile Side Length (Hex)"), groupProperty);

    QtVariantProperty *staggerAxisProperty =
            createProperty(StaggerAxisProperty,
                           QtVariantPropertyManager::enumTypeId(),
                           tr("Stagger Axis"),
                           groupProperty);

    staggerAxisProperty->setAttribute(QLatin1String("enumNames"), mStaggerAxisNames);

    QtVariantProperty *staggerIndexProperty =
            createProperty(StaggerIndexProperty,
                           QtVariantPropertyManager::enumTypeId(),
                           tr("Stagger Index"),
                           groupProperty);

    staggerIndexProperty->setAttribute(QLatin1String("enumNames"), mStaggerIndexNames);

    QtVariantProperty *layerFormatProperty =
            createProperty(LayerFormatProperty,
                           QtVariantPropertyManager::enumTypeId(),
                           tr("Tile Layer Format"),
                           groupProperty);

    layerFormatProperty->setAttribute(QLatin1String("enumNames"), mLayerFormatNames);

    QtVariantProperty *renderOrderProperty =
            createProperty(RenderOrderProperty,
                           QtVariantPropertyManager::enumTypeId(),
                           tr("Tile Render Order"),
                           groupProperty);

    renderOrderProperty->setAttribute(QLatin1String("enumNames"), mRenderOrderNames);

    createProperty(ColorProperty, QVariant::Color, tr("Background Color"), groupProperty);
    addProperty(groupProperty);
}

static QStringList objectTypeNames()
{
    QStringList names;
    for (const ObjectType &type : Preferences::instance()->objectTypes())
        names.append(type.name);
    return names;
}

void PropertyBrowser::addMapObjectProperties()
{
    // DEFAULT MAP OBJECT PROPERTIES
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Object"));

    createProperty(IdProperty, QVariant::Int, tr("ID"), groupProperty)->setEnabled(false);
    createProperty(NameProperty, QVariant::String, tr("Name"), groupProperty);

    QtVariantProperty *typeProperty =
            createProperty(TypeProperty, QVariant::String, tr("Type"), groupProperty);
    typeProperty->setAttribute(QLatin1String("suggestions"), objectTypeNames());

    createProperty(VisibleProperty, QVariant::Bool, tr("Visible"), groupProperty);
    createProperty(XProperty, QVariant::Double, tr("X"), groupProperty);
    createProperty(YProperty, QVariant::Double, tr("Y"), groupProperty);
    createProperty(WidthProperty, QVariant::Double, tr("Width"), groupProperty);
    createProperty(HeightProperty, QVariant::Double, tr("Height"), groupProperty);
    createProperty(RotationProperty, QVariant::Double, tr("Rotation"), groupProperty);

    if (!static_cast<const MapObject*>(mObject)->cell().isEmpty()) {
        QtVariantProperty *flippingProperty =
                createProperty(FlippingProperty, VariantPropertyManager::flagTypeId(),
                               tr("Flipping"), groupProperty);

        flippingProperty->setAttribute(QLatin1String("flagNames"), mFlippingFlagNames);
    }

    addProperty(groupProperty);
}

void PropertyBrowser::addLayerProperties(QtProperty *parent)
{
    createProperty(NameProperty, QVariant::String, tr("Name"), parent);
    createProperty(VisibleProperty, QVariant::Bool, tr("Visible"), parent);

    QtVariantProperty *opacityProperty =
            createProperty(OpacityProperty, QVariant::Double, tr("Opacity"), parent);
    opacityProperty->setAttribute(QLatin1String("minimum"), 0.0);
    opacityProperty->setAttribute(QLatin1String("maximum"), 1.0);
    opacityProperty->setAttribute(QLatin1String("singleStep"), 0.1);
}

void PropertyBrowser::addTileLayerProperties()
{
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Tile Layer"));
    addLayerProperties(groupProperty);
    createProperty(OffsetXProperty, QVariant::Double, tr("Horizontal Offset"), groupProperty);
    createProperty(OffsetYProperty, QVariant::Double, tr("Vertical Offset"), groupProperty);
    addProperty(groupProperty);
}

void PropertyBrowser::addObjectGroupProperties()
{
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Object Layer"));
    addLayerProperties(groupProperty);
    createProperty(OffsetXProperty, QVariant::Double, tr("Horizontal Offset"), groupProperty);
    createProperty(OffsetYProperty, QVariant::Double, tr("Vertical Offset"), groupProperty);

    createProperty(ColorProperty, QVariant::Color, tr("Color"), groupProperty);

    QtVariantProperty *drawOrderProperty =
            createProperty(DrawOrderProperty,
                           QtVariantPropertyManager::enumTypeId(),
                           tr("Drawing Order"),
                           groupProperty);

    drawOrderProperty->setAttribute(QLatin1String("enumNames"), mDrawOrderNames);

    addProperty(groupProperty);
}

void PropertyBrowser::addImageLayerProperties()
{
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Image Layer"));
    addLayerProperties(groupProperty);

    QtVariantProperty *imageSourceProperty = createProperty(ImageSourceProperty,
                                                            VariantPropertyManager::filePathTypeId(),
                                                            tr("Image"), groupProperty);

    imageSourceProperty->setAttribute(QLatin1String("filter"),
                                      Utils::readableImageFormatsFilter());

    createProperty(ColorProperty, QVariant::Color, tr("Transparent Color"), groupProperty);
    createProperty(OffsetXProperty, QVariant::Double, tr("Horizontal Offset"), groupProperty);
    createProperty(OffsetYProperty, QVariant::Double, tr("Vertical Offset"), groupProperty);
    addProperty(groupProperty);
}

void PropertyBrowser::addTilesetProperties()
{
    const Tileset *tileset = static_cast<const Tileset*>(mObject);

    QtProperty *groupProperty = mGroupManager->addProperty(tr("Tileset"));

    if (mMapDocument) {
        auto property = createProperty(FileNameProperty, VariantPropertyManager::filePathTypeId(), tr("Filename"), groupProperty);

        QString filter = QCoreApplication::translate("MainWindow", "All Files (*)");
        filter += QLatin1String(";;");
        filter += TsxTilesetFormat().nameFilter();
        FormatHelper<TilesetFormat> helper(FileFormat::Read, filter);

        property->setAttribute(QStringLiteral("filter"), helper.filter());
    }

    QtVariantProperty *nameProperty = createProperty(NameProperty, QVariant::String, tr("Name"), groupProperty);
    nameProperty->setEnabled(mTilesetDocument);

    QtVariantProperty *tileOffsetProperty = createProperty(TileOffsetProperty, QVariant::Point, tr("Drawing Offset"), groupProperty);
    tileOffsetProperty->setEnabled(mTilesetDocument);

    QtVariantProperty *columnsProperty = createProperty(ColumnCountProperty, QVariant::Int, tr("Columns"), groupProperty);
    columnsProperty->setAttribute(QLatin1String("minimum"), 1);

    // Next properties we should add only for non 'Collection of Images' tilesets
    if (!tileset->isCollection()) {
        QtVariantProperty *parametersProperty =
                createProperty(TilesetImageParametersProperty, VariantPropertyManager::tilesetParametersTypeId(), tr("Image"), groupProperty);

        parametersProperty->setEnabled(mTilesetDocument);

        QtVariantProperty *imageSourceProperty = createProperty(ImageSourceProperty, QVariant::String, tr("Source"), parametersProperty);
        QtVariantProperty *tileWidthProperty = createProperty(TileWidthProperty, QVariant::Int, tr("Tile Width"), parametersProperty);
        QtVariantProperty *tileHeightProperty = createProperty(TileHeightProperty, QVariant::Int, tr("Tile Height"), parametersProperty);
        QtVariantProperty *marginProperty = createProperty(MarginProperty, QVariant::Int, tr("Margin"), parametersProperty);
        QtVariantProperty *spacingProperty = createProperty(SpacingProperty, QVariant::Int, tr("Spacing"), parametersProperty);
        QtVariantProperty *colorProperty = createProperty(ColorProperty, QVariant::Color, tr("Transparent Color"), parametersProperty);

        // These properties can't be directly edited. To change the parameters,
        // the TilesetParametersEdit is used.
        imageSourceProperty->setEnabled(false);
        tileWidthProperty->setEnabled(false);
        tileHeightProperty->setEnabled(false);
        marginProperty->setEnabled(false);
        spacingProperty->setEnabled(false);
        colorProperty->setEnabled(false);
    }
    addProperty(groupProperty);
}

void PropertyBrowser::addTileProperties()
{
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Tile"));
    createProperty(IdProperty, QVariant::Int, tr("ID"), groupProperty)->setEnabled(false);
    createProperty(WidthProperty, QVariant::Int, tr("Width"), groupProperty)->setEnabled(false);
    createProperty(HeightProperty, QVariant::Int, tr("Height"), groupProperty)->setEnabled(false);

    QtVariantProperty *probabilityProperty = createProperty(TileProbabilityProperty,
                                                            QVariant::Double,
                                                            tr("Probability"),
                                                            groupProperty);
    probabilityProperty->setAttribute(QLatin1String("decimals"), 3);
    probabilityProperty->setToolTip(tr("Relative chance this tile will be picked"));
    probabilityProperty->setEnabled(mTilesetDocument);

    const Tile *tile = static_cast<const Tile*>(mObject);
    if (!tile->imageSource().isEmpty()) {
        QtVariantProperty *imageSourceProperty = createProperty(ImageSourceProperty,
                                                                VariantPropertyManager::filePathTypeId(),
                                                                tr("Image"), groupProperty);

        imageSourceProperty->setAttribute(QLatin1String("filter"),
                                          Utils::readableImageFormatsFilter());
        imageSourceProperty->setEnabled(mTilesetDocument);
    }

    addProperty(groupProperty);
}

void PropertyBrowser::addTerrainProperties()
{
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Terrain"));
    QtVariantProperty *nameProperty = createProperty(NameProperty, QVariant::String, tr("Name"), groupProperty);
    nameProperty->setEnabled(mTilesetDocument);
    addProperty(groupProperty);
}

void PropertyBrowser::applyMapValue(PropertyId id, const QVariant &val)
{
    QUndoCommand *command = nullptr;

    switch (id) {
    case TileWidthProperty:
        command = new ChangeMapProperty(mMapDocument, ChangeMapProperty::TileWidth,
                                        val.toInt());
        break;
    case TileHeightProperty:
        command = new ChangeMapProperty(mMapDocument, ChangeMapProperty::TileHeight,
                                        val.toInt());
        break;
    case OrientationProperty: {
        Map::Orientation orientation = static_cast<Map::Orientation>(val.toInt() + 1);
        command = new ChangeMapProperty(mMapDocument, orientation);
        break;
    }
    case HexSideLengthProperty: {
        command = new ChangeMapProperty(mMapDocument, ChangeMapProperty::HexSideLength,
                                        val.toInt());
        break;
    }
    case StaggerAxisProperty: {
        Map::StaggerAxis staggerAxis = static_cast<Map::StaggerAxis>(val.toInt());
        command = new ChangeMapProperty(mMapDocument, staggerAxis);
        break;
    }
    case StaggerIndexProperty: {
        Map::StaggerIndex staggerIndex = static_cast<Map::StaggerIndex>(val.toInt());
        command = new ChangeMapProperty(mMapDocument, staggerIndex);
        break;
    }
    case LayerFormatProperty: {
        Map::LayerDataFormat format = static_cast<Map::LayerDataFormat>(val.toInt());
        command = new ChangeMapProperty(mMapDocument, format);
        break;
    }
    case RenderOrderProperty: {
        Map::RenderOrder renderOrder = static_cast<Map::RenderOrder>(val.toInt());
        command = new ChangeMapProperty(mMapDocument, renderOrder);
        break;
    }
    case ColorProperty:
        command = new ChangeMapProperty(mMapDocument, val.value<QColor>());
        break;
    default:
        break;
    }

    if (command)
        mDocument->undoStack()->push(command);
}

QUndoCommand *PropertyBrowser::applyMapObjectValueTo(PropertyId id, const QVariant &val, MapObject *mapObject)
{
    QUndoCommand *command = nullptr;

    switch (id) {
    case NameProperty:
    case TypeProperty:
        command = new ChangeMapObject(mMapDocument, mapObject,
                                      mIdToProperty[NameProperty]->value().toString(),
                                      mIdToProperty[TypeProperty]->value().toString());
        break;
    case VisibleProperty:
        command = new SetMapObjectVisible(mMapDocument, mapObject, val.toBool());
        break;
    case XProperty: {
        const QPointF oldPos = mapObject->position();
        const QPointF newPos(val.toReal(), oldPos.y());
        command = new MoveMapObject(mMapDocument, mapObject, newPos, oldPos);
        break;
    }
    case YProperty: {
        const QPointF oldPos = mapObject->position();
        const QPointF newPos(oldPos.x(), val.toReal());
        command = new MoveMapObject(mMapDocument, mapObject, newPos, oldPos);
        break;
    }
    case WidthProperty: {
        const QSizeF oldSize = mapObject->size();
        const QSizeF newSize(val.toReal(), oldSize.height());
        command = new ResizeMapObject(mMapDocument, mapObject, newSize, oldSize);
        break;
    }
    case HeightProperty: {
        const QSizeF oldSize = mapObject->size();
        const QSizeF newSize(oldSize.width(), val.toReal());
        command = new ResizeMapObject(mMapDocument, mapObject, newSize, oldSize);
        break;
    }
    case RotationProperty: {
        const qreal newRotation = val.toDouble();
        const qreal oldRotation = mapObject->rotation();
        command = new RotateMapObject(mMapDocument, mapObject, newRotation, oldRotation);
        break;
    }
    case FlippingProperty: {
        const int flippingFlags = val.toInt();
        const bool flippedHorizontally = flippingFlags & 1;
        const bool flippedVertically = flippingFlags & 2;

        // You can only change one checkbox at a time
        if (mapObject->cell().flippedHorizontally != flippedHorizontally) {
            command = new FlipMapObjects(mMapDocument,
                                         QList<MapObject*>() << mapObject,
                                         FlipHorizontally);
        } else if (mapObject->cell().flippedVertically != flippedVertically) {
            command = new FlipMapObjects(mMapDocument,
                                         QList<MapObject*>() << mapObject,
                                         FlipVertically);
        }
    }
    default:
        break;
    }

    return command;
}

void PropertyBrowser::applyMapObjectValue(PropertyId id, const QVariant &val)
{
    MapObject *mapObject = static_cast<MapObject*>(mObject);

    QUndoCommand *command = applyMapObjectValueTo(id, val, mapObject);

    mDocument->undoStack()->beginMacro(command->text());
    mDocument->undoStack()->push(command);

    //Used to share non-custom properties.
    QList<MapObject*> selectedObjects = mMapDocument->selectedObjects();
    if (selectedObjects.size() > 1) {
        for (MapObject *obj : selectedObjects) {
            if (obj != mapObject) {
                if (QUndoCommand *cmd = applyMapObjectValueTo(id, val, obj))
                    mDocument->undoStack()->push(cmd);
            }
        }
    }

    mDocument->undoStack()->endMacro();
}

void PropertyBrowser::applyLayerValue(PropertyId id, const QVariant &val)
{
    Layer *layer = static_cast<Layer*>(mObject);
    const int layerIndex = mMapDocument->map()->layers().indexOf(layer);
    QUndoCommand *command = nullptr;

    switch (id) {
    case NameProperty:
        command = new RenameLayer(mMapDocument, layerIndex, val.toString());
        break;
    case VisibleProperty:
        command = new SetLayerVisible(mMapDocument, layerIndex, val.toBool());
        break;
    case OpacityProperty:
        command = new SetLayerOpacity(mMapDocument, layerIndex, val.toDouble());
        break;
    case OffsetXProperty:
    case OffsetYProperty: {
        QPointF offset = layer->offset();

        if (id == OffsetXProperty)
            offset.setX(val.toDouble());
        else
            offset.setY(val.toDouble());

        command = new SetLayerOffset(mMapDocument, layerIndex, offset);
    }
    default:
        switch (layer->layerType()) {
        case Layer::TileLayerType:   applyTileLayerValue(id, val);   break;
        case Layer::ObjectGroupType: applyObjectGroupValue(id, val); break;
        case Layer::ImageLayerType:  applyImageLayerValue(id, val);  break;
        }
        break;
    }

    if (command)
        mDocument->undoStack()->push(command);
}

void PropertyBrowser::applyTileLayerValue(PropertyId id, const QVariant &val)
{
    Q_UNUSED(id)
    Q_UNUSED(val)
}

void PropertyBrowser::applyObjectGroupValue(PropertyId id, const QVariant &val)
{
    ObjectGroup *objectGroup = static_cast<ObjectGroup*>(mObject);
    QUndoCommand *command = nullptr;

    switch (id) {
    case ColorProperty: {
        const QColor color = val.value<QColor>();
        command = new ChangeObjectGroupProperties(mMapDocument,
                                                  objectGroup,
                                                  color,
                                                  objectGroup->drawOrder());
        break;
    }
    case DrawOrderProperty: {
        ObjectGroup::DrawOrder drawOrder = static_cast<ObjectGroup::DrawOrder>(val.toInt());
        command = new ChangeObjectGroupProperties(mMapDocument,
                                                  objectGroup,
                                                  objectGroup->color(),
                                                  drawOrder);
        break;
    }
    default:
        break;
    }

    if (command)
        mDocument->undoStack()->push(command);
}

void PropertyBrowser::applyImageLayerValue(PropertyId id, const QVariant &val)
{
    ImageLayer *imageLayer = static_cast<ImageLayer*>(mObject);
    QUndoStack *undoStack = mDocument->undoStack();

    switch (id) {
    case ImageSourceProperty: {
        const QString imageSource = val.toString();
        const QColor &color = imageLayer->transparentColor();
        undoStack->push(new ChangeImageLayerProperties(mMapDocument,
                                                       imageLayer,
                                                       color,
                                                       imageSource));
        break;
    }
    case ColorProperty: {
        const QColor color = val.value<QColor>();
        const QString &imageSource = imageLayer->imageSource();
        undoStack->push(new ChangeImageLayerProperties(mMapDocument,
                                                       imageLayer,
                                                       color,
                                                       imageSource));
        break;
    }
    default:
        break;
    }
}

void PropertyBrowser::applyTilesetValue(PropertyBrowser::PropertyId id, const QVariant &val)
{
    Tileset *tileset = static_cast<Tileset*>(mObject);
    QUndoStack *undoStack = mDocument->undoStack();

    switch (id) {
    case FileNameProperty: {
        QString fileName = val.toString();
        QString error;
        SharedTileset newTileset = Tiled::readTileset(fileName, &error);
        if (!newTileset) {
            QMessageBox::critical(window(), tr("Error Reading Tileset"), error);
            return;
        }

        int index = mMapDocument->map()->tilesets().indexOf(tileset->sharedPointer());
        if (index != -1)
            undoStack->push(new ReplaceTileset(mMapDocument, index, newTileset));

        break;
    }
    case NameProperty:
        Q_ASSERT(mTilesetDocument);
        undoStack->push(new RenameTileset(mTilesetDocument, val.toString()));
        break;
    case TileOffsetProperty:
        Q_ASSERT(mTilesetDocument);
        undoStack->push(new ChangeTilesetTileOffset(mTilesetDocument,
                                                    val.toPoint()));
        break;
    case ColumnCountProperty:
        Q_ASSERT(mTilesetDocument);
        undoStack->push(new ChangeTilesetColumnCount(mTilesetDocument,
                                                     val.toInt()));
        break;
    default:
        break;
    }
}

void PropertyBrowser::applyTileValue(PropertyId id, const QVariant &val)
{
    Q_ASSERT(mTilesetDocument);

    Tile *tile = static_cast<Tile*>(mObject);
    QUndoStack *undoStack = mDocument->undoStack();

    switch (id) {
    case TileProbabilityProperty:
        undoStack->push(new ChangeTileProbability(mTilesetDocument,
                                                  mMapDocument->selectedTiles(),
                                                  val.toFloat()));
        break;
    case ImageSourceProperty:
        undoStack->push(new ChangeTileImageSource(mTilesetDocument,
                                                  tile, val.toString()));
        break;
    default:
        break;
    }
}

void PropertyBrowser::applyTerrainValue(PropertyId id, const QVariant &val)
{
    Q_ASSERT(mTilesetDocument);

    Terrain *terrain = static_cast<Terrain*>(mObject);

    if (id == NameProperty) {
        QUndoStack *undoStack = mDocument->undoStack();
        undoStack->push(new RenameTerrain(mTilesetDocument,
                                          terrain->id(),
                                          val.toString()));
    }
}

QtVariantProperty *PropertyBrowser::createProperty(PropertyId id, int type,
                                                   const QString &name,
                                                   QtProperty *parent)
{
    QtVariantProperty *property = mVariantManager->addProperty(type, name);
    if (!property) {
        // fall back to string property for unsupported property types
        property = mVariantManager->addProperty(QVariant::String, name);
    }

    if (type == QVariant::Bool)
        property->setAttribute(QLatin1String("textVisible"), false);

    parent->addSubProperty(property);
    mPropertyToId.insert(property, id);

    if (id != CustomProperty) {
        Q_ASSERT(!mIdToProperty.contains(id));
        mIdToProperty.insert(id, property);
    } else {
        mNameToProperty.insert(name, property);
    }

    return property;
}

void PropertyBrowser::addProperties()
{
    if (!mObject)
        return;

    mUpdating = true;

    // Add the built-in properties for each object type
    switch (mObject->typeId()) {
    case Object::MapType:               addMapProperties(); break;
    case Object::MapObjectType:         addMapObjectProperties(); break;
    case Object::LayerType:
        switch (static_cast<Layer*>(mObject)->layerType()) {
        case Layer::TileLayerType:      addTileLayerProperties();   break;
        case Layer::ObjectGroupType:    addObjectGroupProperties(); break;
        case Layer::ImageLayerType:     addImageLayerProperties();  break;
        }
        break;
    case Object::TilesetType:           addTilesetProperties(); break;
    case Object::TileType:              addTileProperties(); break;
    case Object::TerrainType:           addTerrainProperties(); break;
    }

    // Make sure the color property is collapsed, to save space
    if (QtProperty *colorProperty = mIdToProperty.value(ColorProperty))
        setExpanded(items(colorProperty).first(), false);

    // Add a node for the custom properties
    mCustomPropertiesGroup = mGroupManager->addProperty(tr("Custom Properties"));
    addProperty(mCustomPropertiesGroup);

    mUpdating = false;

    updateProperties();
    updateCustomProperties();
}

void PropertyBrowser::removeProperties()
{
    mVariantManager->clear();
    mGroupManager->clear();
    mPropertyToId.clear();
    mIdToProperty.clear();
    mNameToProperty.clear();
    mCustomPropertiesGroup = nullptr;
}

void PropertyBrowser::updateProperties()
{
    Q_ASSERT(mObject);

    mUpdating = true;

    switch (mObject->typeId()) {
    case Object::MapType: {
        const Map *map = static_cast<const Map*>(mObject);
        mIdToProperty[WidthProperty]->setValue(map->width());
        mIdToProperty[HeightProperty]->setValue(map->height());
        mIdToProperty[TileWidthProperty]->setValue(map->tileWidth());
        mIdToProperty[TileHeightProperty]->setValue(map->tileHeight());
        mIdToProperty[OrientationProperty]->setValue(map->orientation() - 1);
        mIdToProperty[HexSideLengthProperty]->setValue(map->hexSideLength());
        mIdToProperty[StaggerAxisProperty]->setValue(map->staggerAxis());
        mIdToProperty[StaggerIndexProperty]->setValue(map->staggerIndex());
        mIdToProperty[LayerFormatProperty]->setValue(map->layerDataFormat());
        mIdToProperty[RenderOrderProperty]->setValue(map->renderOrder());
        mIdToProperty[ColorProperty]->setValue(map->backgroundColor());
        break;
    }
    case Object::MapObjectType: {
        const MapObject *mapObject = static_cast<const MapObject*>(mObject);
        mIdToProperty[IdProperty]->setValue(mapObject->id());
        mIdToProperty[NameProperty]->setValue(mapObject->name());
        mIdToProperty[TypeProperty]->setValue(mapObject->type());
        mIdToProperty[VisibleProperty]->setValue(mapObject->isVisible());
        mIdToProperty[XProperty]->setValue(mapObject->x());
        mIdToProperty[YProperty]->setValue(mapObject->y());
        mIdToProperty[WidthProperty]->setValue(mapObject->width());
        mIdToProperty[HeightProperty]->setValue(mapObject->height());
        mIdToProperty[RotationProperty]->setValue(mapObject->rotation());

        if (QtVariantProperty *property = mIdToProperty[FlippingProperty]) {
            int flippingFlags = 0;
            if (mapObject->cell().flippedHorizontally)
                flippingFlags |= 1;
            if (mapObject->cell().flippedVertically)
                flippingFlags |= 2;
            property->setValue(flippingFlags);
        }
        break;
    }
    case Object::LayerType: {
        const Layer *layer = static_cast<const Layer*>(mObject);

        mIdToProperty[NameProperty]->setValue(layer->name());
        mIdToProperty[VisibleProperty]->setValue(layer->isVisible());
        mIdToProperty[OpacityProperty]->setValue(layer->opacity());
        mIdToProperty[OffsetXProperty]->setValue(layer->offset().x());
        mIdToProperty[OffsetYProperty]->setValue(layer->offset().y());

        switch (layer->layerType()) {
        case Layer::TileLayerType:
            break;
        case Layer::ObjectGroupType: {
            const ObjectGroup *objectGroup = static_cast<const ObjectGroup*>(layer);
            const QColor color = objectGroup->color();
            mIdToProperty[ColorProperty]->setValue(color);
            mIdToProperty[DrawOrderProperty]->setValue(objectGroup->drawOrder());
            break;
        }
        case Layer::ImageLayerType:
            const ImageLayer *imageLayer = static_cast<const ImageLayer*>(layer);
            mIdToProperty[ImageSourceProperty]->setValue(imageLayer->imageSource());
            mIdToProperty[ColorProperty]->setValue(imageLayer->transparentColor());
            break;
        }
        break;
    }
    case Object::TilesetType: {
        Tileset *tileset = static_cast<Tileset*>(mObject);

        if (QtVariantProperty *fileNameProperty = mIdToProperty.value(FileNameProperty))
            fileNameProperty->setValue(tileset->fileName());

        mIdToProperty[NameProperty]->setValue(tileset->name());
        mIdToProperty[TileOffsetProperty]->setValue(tileset->tileOffset());
        mIdToProperty[ColumnCountProperty]->setValue(tileset->columnCount());
        mIdToProperty[ColumnCountProperty]->setEnabled(mTilesetDocument && tileset->isCollection());

        if (!tileset->isCollection()) {
            // todo: Consider when to set the TilesetDocument on the parameters edit
//            EmbeddedTileset embeddedTileset(mMapDocument, tileset);

//            mIdToProperty[TilesetImageParametersProperty]->setValue(QVariant::fromValue(embeddedTileset));
            mIdToProperty[ImageSourceProperty]->setValue(tileset->imageSource());
            mIdToProperty[TileWidthProperty]->setValue(tileset->tileWidth());
            mIdToProperty[TileHeightProperty]->setValue(tileset->tileHeight());
            mIdToProperty[MarginProperty]->setValue(tileset->margin());
            mIdToProperty[SpacingProperty]->setValue(tileset->tileSpacing());
            mIdToProperty[ColorProperty]->setValue(tileset->transparentColor());
        }
        break;
    }
    case Object::TileType: {
        const Tile *tile = static_cast<const Tile*>(mObject);
        const QSize tileSize = tile->size();
        mIdToProperty[IdProperty]->setValue(tile->id());
        mIdToProperty[WidthProperty]->setValue(tileSize.width());
        mIdToProperty[HeightProperty]->setValue(tileSize.height());
        mIdToProperty[TileProbabilityProperty]->setValue(tile->probability());
        if (QtVariantProperty *imageSourceProperty = mIdToProperty.value(ImageSourceProperty))
            imageSourceProperty->setValue(tile->imageSource());
        break;
    }
    case Object::TerrainType: {
        const Terrain *terrain = static_cast<const Terrain*>(mObject);
        mIdToProperty[NameProperty]->setValue(terrain->name());
        break;
    }
    }

    mUpdating = false;
}

void PropertyBrowser::updateCustomProperties()
{
    if (!mObject)
        return;

    bool wasUpdating = mUpdating;
    mUpdating = true;

    qDeleteAll(mNameToProperty);
    mNameToProperty.clear();

    mCombinedProperties = mObject->properties();
    // Add properties from selected objects which mObject does not contain to mCombinedProperties.
    for (Object *obj : mDocument->currentObjects()) {
        if (obj == mObject)
            continue;

        QMapIterator<QString,QVariant> it(obj->properties());

        while (it.hasNext()) {
            it.next();
            if (!mCombinedProperties.contains(it.key()))
                mCombinedProperties.insert(it.key(), QString());
        }
    }

    // Add properties based on object type, if defined
    if (mObject->typeId() == Object::MapObjectType) {
        const QString currentType = static_cast<MapObject*>(mObject)->type();
        const ObjectTypes objectTypes = Preferences::instance()->objectTypes();
        for (const ObjectType &type : objectTypes) {
            if (type.name == currentType) {
                QMapIterator<QString,QVariant> it(type.defaultProperties);
                while (it.hasNext()) {
                    it.next();
                    if (!mCombinedProperties.contains(it.key()))
                        mCombinedProperties.insert(it.key(), it.value());
                }
            }
        }
    }

    QMapIterator<QString,QVariant> it(mCombinedProperties);

    while (it.hasNext()) {
        it.next();
        QtVariantProperty *property = createProperty(CustomProperty,
                                                     it.value().type(),
                                                     it.key(),
                                                     mCustomPropertiesGroup);

        property->setValue(it.value());
        updatePropertyColor(it.key());
    }

    mUpdating = wasUpdating;
}

// If there are other objects selected check if their properties are equal. If not give them a gray color.
void PropertyBrowser::updatePropertyColor(const QString &name)
{
    QtVariantProperty *property = mNameToProperty.value(name);
    if (!property)
        return;

    QString propertyName = property->propertyName();
    QString propertyValue = property->valueText();

    const auto &objects = mDocument->currentObjects();

    // If one of the objects doesn't have this property then gray out the name and value.
    for (Object *obj : objects) {
        if (!obj->hasProperty(propertyName)) {
            property->setNameColor(Qt::gray);
            property->setValueColor(Qt::gray);
            return;
        }
    }

    // If one of the objects doesn't have the same property value then gray out the value.
    for (Object *obj : objects) {
        if (obj == mObject)
            continue;
        if (obj->property(propertyName) != propertyValue) {
            property->setValueColor(Qt::gray);
            return;
        }
    }

    property->setNameColor(Qt::black);
    property->setValueColor(Qt::black);
}

} // namespace Internal
} // namespace Tiled
