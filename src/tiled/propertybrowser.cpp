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

#include "changeimagelayerproperties.h"
#include "changelayer.h"
#include "changemapobject.h"
#include "changemapproperty.h"
#include "changeobjectgroupproperties.h"
#include "changeproperties.h"
#include "changetile.h"
#include "changetileimagesource.h"
#include "changetileprobability.h"
#include "changewangcolordata.h"
#include "changewangsetdata.h"
#include "flipmapobjects.h"
#include "grouplayer.h"
#include "imagelayer.h"
#include "map.h"
#include "mapdocument.h"
#include "mapobject.h"
#include "movemapobject.h"
#include "objectgroup.h"
#include "objecttemplate.h"
#include "preferences.h"
#include "renamelayer.h"
#include "renameterrain.h"
#include "renamewangset.h"
#include "replacetileset.h"
#include "resizemapobject.h"
#include "rotatemapobject.h"
#include "terrain.h"
#include "tile.h"
#include "tilelayer.h"
#include "tilesetchanges.h"
#include "tilesetdocument.h"
#include "tilesetformat.h"
#include "tilesetmanager.h"
#include "tilesetterrainmodel.h"
#include "tilesetwangsetmodel.h"
#include "utils.h"
#include "varianteditorfactory.h"
#include "variantpropertymanager.h"
#include "wangcolormodel.h"
#include "wangset.h"
#include "invertyaxishelper.h"

#include <QtGroupPropertyManager>

#include <QCoreApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QMessageBox>

namespace Tiled {

PropertyBrowser::PropertyBrowser(QWidget *parent)
    : QtTreePropertyBrowser(parent)
    , mUpdating(false)
    , mMapObjectFlags(0)
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
    setAllowMultiSelection(true);

    retranslateUi();

    connect(mVariantManager, &QtVariantPropertyManager::valueChanged,
            this, &PropertyBrowser::valueChanged);

    connect(variantEditorFactory, &VariantEditorFactory::resetProperty,
            this, &PropertyBrowser::resetProperty);

    connect(Preferences::instance(), &Preferences::objectTypesChanged,
            this, &PropertyBrowser::objectTypesChanged);
    
    connect(Preferences::instance(), &Preferences::invertYAxisChanged,
            this, &PropertyBrowser::invertYAxisChanged);
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
        if (mTilesetDocument) {
            mTilesetDocument->terrainModel()->disconnect(this);
            mTilesetDocument->wangSetModel()->disconnect(this);
        }
    }

    mDocument = document;
    mMapDocument = mapDocument;
    mTilesetDocument = tilesetDocument;

    if (mapDocument) {
        connect(mapDocument, &MapDocument::mapChanged,
                this, &PropertyBrowser::mapChanged);
        connect(mapDocument, &MapDocument::objectsChanged,
                this, &PropertyBrowser::objectsChanged);
        connect(mapDocument, &MapDocument::objectsTypeChanged,
                this, &PropertyBrowser::objectsTypeChanged);
        connect(mapDocument, &MapDocument::layerChanged,
                this, &PropertyBrowser::layerChanged);
        connect(mapDocument, &MapDocument::objectGroupChanged,
                this, &PropertyBrowser::objectGroupChanged);
        connect(mapDocument, &MapDocument::imageLayerChanged,
                this, &PropertyBrowser::imageLayerChanged);
        connect(mapDocument, &MapDocument::tileTypeChanged,
                this, &PropertyBrowser::tileTypeChanged);

        connect(mapDocument, &MapDocument::selectedObjectsChanged,
                this, &PropertyBrowser::selectedObjectsChanged);
        connect(mapDocument, &MapDocument::selectedLayersChanged,
                this, &PropertyBrowser::selectedLayersChanged);
    }

    if (tilesetDocument) {
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
        connect(tilesetDocument, &TilesetDocument::tileTypeChanged,
                this, &PropertyBrowser::tileTypeChanged);

        connect(tilesetDocument, &TilesetDocument::selectedTilesChanged,
                this, &PropertyBrowser::selectedTilesChanged);

        TilesetTerrainModel *terrainModel = tilesetDocument->terrainModel();
        connect(terrainModel, &TilesetTerrainModel::terrainChanged,
                this, &PropertyBrowser::terrainChanged);

        TilesetWangSetModel *wangSetModel = tilesetDocument->wangSetModel();
        connect(wangSetModel, &TilesetWangSetModel::wangSetChanged,
                this, &PropertyBrowser::wangSetChanged);
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
    }
}

bool PropertyBrowser::isCustomPropertyItem(const QtBrowserItem *item) const
{
    return item && mPropertyToId[item->property()] == CustomProperty;
}

/**
 * Returns whether the given list of \a items are all custom properties.
 */
bool PropertyBrowser::allCustomPropertyItems(const QList<QtBrowserItem *> &items) const
{
    for (QtBrowserItem *item : items)
        if (mPropertyToId[item->property()] != CustomProperty)
            return false;

    return true;
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

QSize PropertyBrowser::sizeHint() const
{
    return Utils::dpiScaled(QSize(260, 100));
}

bool PropertyBrowser::event(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();

    if (event->type() == QEvent::ShortcutOverride) {
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Tab) {
            if (editedItem()) {
                event->accept();
                return true;
            }
        }
    }

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

void PropertyBrowser::layerChanged(Layer *layer)
{
    if (mObject == layer)
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
    if (mObject == tileset) {
        updateProperties();
        updateCustomProperties();   // Tileset may have been swapped
    }
}

void PropertyBrowser::tileChanged(Tile *tile)
{
    if (mObject == tile)
        updateProperties();
}

void PropertyBrowser::tileTypeChanged(Tile *tile)
{
    if (mObject == tile) {
        updateProperties();
        updateCustomProperties();
    } else if (mObject && mObject->typeId() == Object::MapObjectType) {
        auto mapObject = static_cast<MapObject*>(mObject);
        if (mapObject->cell().tile() == tile && mapObject->type().isEmpty())
            updateProperties();
    }
}

void PropertyBrowser::terrainChanged(Tileset *tileset, int index)
{
    if (mObject == tileset->terrain(index))
        updateProperties();
}

void PropertyBrowser::wangSetChanged(Tileset *tileset, int index)
{
    if (mObject == tileset->wangSet(index))
        updateProperties();
}

void PropertyBrowser::invertYAxisChanged()
{
    if (mObject && mObject->typeId() == Object::MapObjectType)
        updateProperties();
}

static QVariant predefinedPropertyValue(Object *object, const QString &name)
{
    QString objectType;

    switch (object->typeId()) {
    case Object::TileType:
        objectType = static_cast<Tile*>(object)->type();
        break;
    case Object::MapObjectType: {
        auto mapObject = static_cast<MapObject*>(object);
        objectType = mapObject->type();

        if (Tile *tile = mapObject->cell().tile()) {
            if (tile->hasProperty(name))
                return tile->property(name);

            if (objectType.isEmpty())
                objectType = tile->type();
        }
        break;
    }
    case Object::LayerType:
    case Object::MapType:
    case Object::TerrainType:
    case Object::TilesetType:
    case Object::WangSetType:
    case Object::WangColorType:
    case Object::ObjectTemplateType:
        break;
    }

    if (objectType.isEmpty())
        return QVariant();

    for (const ObjectType &type : Object::objectTypes()) {
        if (type.name == objectType)
            if (type.defaultProperties.contains(name))
                return type.defaultProperties.value(name);
    }

    return QVariant();
}

static bool anyObjectHasProperty(const QList<Object*> &objects, const QString &name)
{
    for (Object *obj : objects) {
        if (obj->hasProperty(name))
            return true;
    }
    return false;
}

static bool propertyValueAffected(Object *currentObject,
                                  Object *changedObject,
                                  const QString &propertyName)
{
    if (currentObject == changedObject)
        return true;

    // Changed property may be inherited
    if (currentObject && currentObject->typeId() == Object::MapObjectType && changedObject->typeId() == Object::TileType) {
        auto tile = static_cast<MapObject*>(currentObject)->cell().tile();
        if (tile == changedObject && !currentObject->hasProperty(propertyName))
            return true;
    }

    return false;
}

static bool objectPropertiesRelevant(Document *document, Object *object)
{
    auto currentObject = document->currentObject();
    if (!currentObject)
        return false;

    if (currentObject == object)
        return true;

    if (currentObject->typeId() == Object::MapObjectType)
        if (static_cast<MapObject*>(currentObject)->cell().tile() == object)
            return true;

    if (document->currentObjects().contains(object))
        return true;

    return false;
}

void PropertyBrowser::propertyAdded(Object *object, const QString &name)
{
    if (!objectPropertiesRelevant(mDocument, object))
        return;
    if (mNameToProperty.contains(name)) {
        if (propertyValueAffected(mObject, object, name))
            setCustomPropertyValue(mNameToProperty[name], object->property(name));
    } else {
        QVariant value;
        if (mObject->hasProperty(name))
            value = mObject->property(name);
        else
            value = predefinedPropertyValue(mObject, name);

        createCustomProperty(name, value);
    }
    updateCustomPropertyColor(name);
}

void PropertyBrowser::propertyRemoved(Object *object, const QString &name)
{
    auto property = mNameToProperty.value(name);
    if (!property)
        return;
    if (!objectPropertiesRelevant(mDocument, object))
        return;

    QVariant predefinedValue = predefinedPropertyValue(mObject, name);

    if (!predefinedValue.isValid() &&
            !anyObjectHasProperty(mDocument->currentObjects(), name)) {
        // It's not a predefined property and no selected object has this
        // property, so delete it.

        // First move up or down the currently selected item
        QtBrowserItem *item = currentItem();
        if (item && item->property() == property) {
            const QList<QtBrowserItem *> siblings = item->parent()->children();
            if (siblings.count() > 1) {
                int currentItemIndex = siblings.indexOf(item);
                if (item == siblings.last()) {
                    setCurrentItem(siblings.at(currentItemIndex - 1));
                } else {
                    setCurrentItem(siblings.at(currentItemIndex + 1));
                }
            }
        }

        deleteCustomProperty(property);
        return;
    }

    if (propertyValueAffected(mObject, object, name)) {
        // Property deleted from the current object, so reset the value.
        setCustomPropertyValue(property, predefinedValue);
    }

    updateCustomPropertyColor(name);
}

void PropertyBrowser::propertyChanged(Object *object, const QString &name)
{
    auto property = mNameToProperty[name];
    if (!property)
        return;

    if (propertyValueAffected(mObject, object, name))
        setCustomPropertyValue(property, object->property(name));

    if (mDocument->currentObjects().contains(object))
        updateCustomPropertyColor(name);
}

void PropertyBrowser::propertiesChanged(Object *object)
{
    if (objectPropertiesRelevant(mDocument, object))
        updateCustomProperties();
}

void PropertyBrowser::selectedObjectsChanged()
{
    updateCustomProperties();
}

void PropertyBrowser::selectedLayersChanged()
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
    case Object::MapType:               applyMapValue(id, val); break;
    case Object::MapObjectType:         applyMapObjectValue(id, val); break;
    case Object::LayerType:             applyLayerValue(id, val); break;
    case Object::TilesetType:           applyTilesetValue(id, val); break;
    case Object::TileType:              applyTileValue(id, val); break;
    case Object::TerrainType:           applyTerrainValue(id, val); break;
    case Object::WangSetType:           applyWangSetValue(id, val); break;
    case Object::WangColorType:         applyWangColorValue(id, val); break;
    case Object::ObjectTemplateType:    break;
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
            addProperty(OrientationProperty,
                        QtVariantPropertyManager::enumTypeId(),
                        tr("Orientation"),
                        groupProperty);

    orientationProperty->setAttribute(QLatin1String("enumNames"), mOrientationNames);

    addProperty(WidthProperty, QVariant::Int, tr("Width"), groupProperty)->setEnabled(false);
    addProperty(HeightProperty, QVariant::Int, tr("Height"), groupProperty)->setEnabled(false);
    addProperty(TileWidthProperty, QVariant::Int, tr("Tile Width"), groupProperty);
    addProperty(TileHeightProperty, QVariant::Int, tr("Tile Height"), groupProperty);
    addProperty(InfiniteProperty, QVariant::Bool, tr("Infinite"), groupProperty);

    addProperty(HexSideLengthProperty, QVariant::Int, tr("Tile Side Length (Hex)"), groupProperty);

    QtVariantProperty *staggerAxisProperty =
            addProperty(StaggerAxisProperty,
                        QtVariantPropertyManager::enumTypeId(),
                        tr("Stagger Axis"),
                        groupProperty);

    staggerAxisProperty->setAttribute(QLatin1String("enumNames"), mStaggerAxisNames);

    QtVariantProperty *staggerIndexProperty =
            addProperty(StaggerIndexProperty,
                        QtVariantPropertyManager::enumTypeId(),
                        tr("Stagger Index"),
                        groupProperty);

    staggerIndexProperty->setAttribute(QLatin1String("enumNames"), mStaggerIndexNames);

    QtVariantProperty *layerFormatProperty =
            addProperty(LayerFormatProperty,
                        QtVariantPropertyManager::enumTypeId(),
                        tr("Tile Layer Format"),
                        groupProperty);

    layerFormatProperty->setAttribute(QLatin1String("enumNames"), mLayerFormatNames);

    QtVariantProperty *renderOrderProperty =
            addProperty(RenderOrderProperty,
                        QtVariantPropertyManager::enumTypeId(),
                        tr("Tile Render Order"),
                        groupProperty);

    renderOrderProperty->setAttribute(QLatin1String("enumNames"), mRenderOrderNames);

    addProperty(BackgroundColorProperty, QVariant::Color, tr("Background Color"), groupProperty);
    addProperty(groupProperty);
}

static QStringList objectTypeNames()
{
    QStringList names;
    for (const ObjectType &type : Object::objectTypes())
        names.append(type.name);
    return names;
}

enum MapObjectFlags {
    ObjectHasDimensions = 0x1,
    ObjectHasTile = 0x2,
    ObjectIsText = 0x4
};

static int mapObjectFlags(const MapObject *mapObject)
{
    int flags = 0;
    if (mapObject->hasDimensions())
        flags |= ObjectHasDimensions;
    if (!mapObject->cell().isEmpty())
        flags |= ObjectHasTile;
    if (mapObject->shape() == MapObject::Text)
        flags |= ObjectIsText;
    return flags;
}

void PropertyBrowser::addMapObjectProperties()
{
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Object"));

    addProperty(IdProperty, QVariant::Int, tr("ID"), groupProperty)->setEnabled(false);
    addProperty(TemplateProperty, filePathTypeId(), tr("Template"), groupProperty)->setEnabled(false);
    addProperty(NameProperty, QVariant::String, tr("Name"), groupProperty);

    QtVariantProperty *typeProperty =
            addProperty(TypeProperty, QVariant::String, tr("Type"), groupProperty);
    typeProperty->setAttribute(QLatin1String("suggestions"), objectTypeNames());

    if (mMapDocument->allowHidingObjects())
        addProperty(VisibleProperty, QVariant::Bool, tr("Visible"), groupProperty);

    addProperty(XProperty, QVariant::Double, tr("X"), groupProperty);
    addProperty(YProperty, QVariant::Double, tr("Y"), groupProperty);

    auto mapObject = static_cast<const MapObject*>(mObject);
    mMapObjectFlags = mapObjectFlags(mapObject);

    if (mMapObjectFlags & ObjectHasDimensions) {
        addProperty(WidthProperty, QVariant::Double, tr("Width"), groupProperty);
        addProperty(HeightProperty, QVariant::Double, tr("Height"), groupProperty);
    }

    bool isPoint = mapObject->shape() == MapObject::Point;
    addProperty(RotationProperty, QVariant::Double, tr("Rotation"), groupProperty)->setEnabled(!isPoint);

    if (mMapObjectFlags & ObjectHasTile) {
        QtVariantProperty *flippingProperty =
                addProperty(FlippingProperty, VariantPropertyManager::flagTypeId(),
                               tr("Flipping"), groupProperty);

        flippingProperty->setAttribute(QLatin1String("flagNames"), mFlippingFlagNames);
    }

    if (mMapObjectFlags & ObjectIsText) {
        addProperty(TextProperty, QVariant::String, tr("Text"), groupProperty)->setAttribute(QLatin1String("multiline"), true);
        addProperty(TextAlignmentProperty, VariantPropertyManager::alignmentTypeId(), tr("Alignment"), groupProperty);
        addProperty(FontProperty, QVariant::Font, tr("Font"), groupProperty);
        addProperty(WordWrapProperty, QVariant::Bool, tr("Word Wrap"), groupProperty);
        addProperty(ColorProperty, QVariant::Color, tr("Color"), groupProperty);
    }

    addProperty(groupProperty);
}

void PropertyBrowser::addLayerProperties(QtProperty *parent)
{
    addProperty(IdProperty, QVariant::Int, tr("ID"), parent)->setEnabled(false);
    addProperty(NameProperty, QVariant::String, tr("Name"), parent);
    addProperty(VisibleProperty, QVariant::Bool, tr("Visible"), parent);
    addProperty(LockedProperty, QVariant::Bool, tr("Locked"), parent);

    QtVariantProperty *opacityProperty =
            addProperty(OpacityProperty, QVariant::Double, tr("Opacity"), parent);
    opacityProperty->setAttribute(QLatin1String("minimum"), 0.0);
    opacityProperty->setAttribute(QLatin1String("maximum"), 1.0);
    opacityProperty->setAttribute(QLatin1String("singleStep"), 0.1);

    addProperty(OffsetXProperty, QVariant::Double, tr("Horizontal Offset"), parent);
    addProperty(OffsetYProperty, QVariant::Double, tr("Vertical Offset"), parent);
}

void PropertyBrowser::addTileLayerProperties()
{
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Tile Layer"));
    addLayerProperties(groupProperty);
    addProperty(groupProperty);
}

void PropertyBrowser::addObjectGroupProperties()
{
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Object Layer"));
    addLayerProperties(groupProperty);

    addProperty(ColorProperty, QVariant::Color, tr("Color"), groupProperty);

    QtVariantProperty *drawOrderProperty =
            addProperty(DrawOrderProperty,
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

    QtVariantProperty *imageSourceProperty = addProperty(ImageSourceProperty,
                                                         filePathTypeId(),
                                                         tr("Image"), groupProperty);

    imageSourceProperty->setAttribute(QLatin1String("filter"),
                                      Utils::readableImageFormatsFilter());

    addProperty(ColorProperty, QVariant::Color, tr("Transparent Color"), groupProperty);

    addProperty(groupProperty);
}

void PropertyBrowser::addGroupLayerProperties()
{
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Group Layer"));
    addLayerProperties(groupProperty);
    addProperty(groupProperty);
}

void PropertyBrowser::addTilesetProperties()
{
    const Tileset *tileset = static_cast<const Tileset*>(mObject);

    QtProperty *groupProperty = mGroupManager->addProperty(tr("Tileset"));

    if (mMapDocument) {
        auto property = addProperty(FileNameProperty, filePathTypeId(), tr("Filename"), groupProperty);

        QString filter = QCoreApplication::translate("MainWindow", "All Files (*)");
        FormatHelper<TilesetFormat> helper(FileFormat::Read, filter);

        property->setAttribute(QStringLiteral("filter"), helper.filter());
    }

    QtVariantProperty *nameProperty = addProperty(NameProperty, QVariant::String, tr("Name"), groupProperty);
    nameProperty->setEnabled(mTilesetDocument);

    QtVariantProperty *tileOffsetProperty = addProperty(TileOffsetProperty, QVariant::Point, tr("Drawing Offset"), groupProperty);
    tileOffsetProperty->setEnabled(mTilesetDocument);

    QtVariantProperty *backgroundProperty = addProperty(BackgroundColorProperty, QVariant::Color, tr("Background Color"), groupProperty);
    backgroundProperty->setEnabled(mTilesetDocument);

    QtVariantProperty *orientationProperty =
            addProperty(OrientationProperty,
                        QtVariantPropertyManager::enumTypeId(),
                        tr("Orientation"),
                        groupProperty);

    orientationProperty->setAttribute(QLatin1String("enumNames"), mTilesetOrientationNames);

    QtVariantProperty *gridWidthProperty = addProperty(GridWidthProperty, QVariant::Int, tr("Grid Width"), groupProperty);
    gridWidthProperty->setEnabled(mTilesetDocument);
    gridWidthProperty->setAttribute(QLatin1String("minimum"), 1);
    QtVariantProperty *gridHeightProperty = addProperty(GridHeightProperty, QVariant::Int, tr("Grid Height"), groupProperty);
    gridHeightProperty->setEnabled(mTilesetDocument);
    gridHeightProperty->setAttribute(QLatin1String("minimum"), 1);

    QtVariantProperty *columnsProperty = addProperty(ColumnCountProperty, QVariant::Int, tr("Columns"), groupProperty);
    columnsProperty->setAttribute(QLatin1String("minimum"), 1);

    // Next properties we should add only for non 'Collection of Images' tilesets
    if (!tileset->isCollection()) {
        QtVariantProperty *parametersProperty =
                addProperty(TilesetImageParametersProperty, VariantPropertyManager::tilesetParametersTypeId(), tr("Image"), groupProperty);

        parametersProperty->setEnabled(mTilesetDocument);

        QtVariantProperty *imageSourceProperty = addProperty(ImageSourceProperty, QVariant::String, tr("Source"), parametersProperty);
        QtVariantProperty *tileWidthProperty = addProperty(TileWidthProperty, QVariant::Int, tr("Tile Width"), parametersProperty);
        QtVariantProperty *tileHeightProperty = addProperty(TileHeightProperty, QVariant::Int, tr("Tile Height"), parametersProperty);
        QtVariantProperty *marginProperty = addProperty(MarginProperty, QVariant::Int, tr("Margin"), parametersProperty);
        QtVariantProperty *spacingProperty = addProperty(SpacingProperty, QVariant::Int, tr("Spacing"), parametersProperty);
        QtVariantProperty *colorProperty = addProperty(ColorProperty, QVariant::Color, tr("Transparent Color"), parametersProperty);

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
    addProperty(IdProperty, QVariant::Int, tr("ID"), groupProperty)->setEnabled(false);

    QtVariantProperty *typeProperty =
            addProperty(TypeProperty, QVariant::String, tr("Type"), groupProperty);
    typeProperty->setAttribute(QLatin1String("suggestions"), objectTypeNames());
    typeProperty->setEnabled(mTilesetDocument);

    addProperty(WidthProperty, QVariant::Int, tr("Width"), groupProperty)->setEnabled(false);
    addProperty(HeightProperty, QVariant::Int, tr("Height"), groupProperty)->setEnabled(false);

    QtVariantProperty *probabilityProperty = addProperty(TileProbabilityProperty,
                                                         QVariant::Double,
                                                         tr("Probability"),
                                                         groupProperty);
    probabilityProperty->setAttribute(QLatin1String("decimals"), 3);
    probabilityProperty->setToolTip(tr("Relative chance this tile will be picked"));
    probabilityProperty->setEnabled(mTilesetDocument);

    const Tile *tile = static_cast<const Tile*>(mObject);
    if (!tile->imageSource().isEmpty()) {
        QtVariantProperty *imageSourceProperty = addProperty(ImageSourceProperty,
                                                             filePathTypeId(),
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
    QtVariantProperty *nameProperty = addProperty(NameProperty, QVariant::String, tr("Name"), groupProperty);
    nameProperty->setEnabled(mTilesetDocument);
    addProperty(groupProperty);
}

void PropertyBrowser::addWangSetProperties()
{
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Wang Set"));
    QtVariantProperty *nameProperty = addProperty(NameProperty, QVariant::String, tr("Name"), groupProperty);
    QtVariantProperty *edgeProperty = addProperty(EdgeCountProperty, QVariant::Int, tr("Edge Count"), groupProperty);
    QtVariantProperty *cornerProperty = addProperty(CornerCountProperty, QVariant::Int, tr("Corner Count"), groupProperty);

    edgeProperty->setAttribute(QLatin1String("minimum"), 1);
    edgeProperty->setAttribute(QLatin1String("maximum"), 15);
    cornerProperty->setAttribute(QLatin1String("minimum"), 1);
    cornerProperty->setAttribute(QLatin1String("maximum"), 15);

    nameProperty->setEnabled(mTilesetDocument);
    edgeProperty->setEnabled(mTilesetDocument);
    cornerProperty->setEnabled(mTilesetDocument);

    addProperty(groupProperty);
}

void PropertyBrowser::addWangColorProperties()
{
    QtProperty *groupProperty = mGroupManager->addProperty(tr("Wang Color"));
    QtVariantProperty *nameProperty = addProperty(NameProperty,
                                                  QVariant::String,
                                                  tr("Name"),
                                                  groupProperty);
    QtVariantProperty *colorProperty = addProperty(ColorProperty,
                                                   QVariant::Color,
                                                   tr("Color"),
                                                   groupProperty);
    QtVariantProperty *probabilityProperty = addProperty(WangColorProbabilityProperty,
                                                         QVariant::Double,
                                                         tr("Probability"),
                                                         groupProperty);

    probabilityProperty->setAttribute(QLatin1String("minimum"), 0.01);

    nameProperty->setEnabled(mTilesetDocument);
    colorProperty->setEnabled(mTilesetDocument);
    probabilityProperty->setEnabled(mTilesetDocument);

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
    case InfiniteProperty: {
        bool infinite = val.toInt();

        QUndoStack *undoStack = mDocument->undoStack();
        undoStack->beginMacro(tr("Change Infinite Property"));

        if (!infinite) {
            QRect mapBounds(QPoint(0, 0), mMapDocument->map()->size());

            LayerIterator iterator(mMapDocument->map());
            while (Layer *layer = iterator.next()) {
                if (TileLayer *tileLayer = dynamic_cast<TileLayer*>(layer))
                    mapBounds = mapBounds.united(tileLayer->bounds());
            }

            if (mapBounds.size() == QSize(0, 0))
                mapBounds.setSize(QSize(1, 1));

            mMapDocument->resizeMap(mapBounds.size(), -mapBounds.topLeft(), false);
        }

        undoStack->push(new ChangeMapProperty(mMapDocument, ChangeMapProperty::Infinite,
                                              val.toInt()));
        undoStack->endMacro();
        break;
    }
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
    case BackgroundColorProperty:
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
    default: {
        MapObject::Property property;

        switch (id) {
        case NameProperty:          property = MapObject::NameProperty; break;
        case TypeProperty:          property = MapObject::TypeProperty; break;
        case VisibleProperty:       property = MapObject::VisibleProperty; break;
        case TextProperty:          property = MapObject::TextProperty; break;
        case FontProperty:          property = MapObject::TextFontProperty; break;
        case TextAlignmentProperty: property = MapObject::TextAlignmentProperty; break;
        case WordWrapProperty:      property = MapObject::TextWordWrapProperty; break;
        case ColorProperty:         property = MapObject::TextColorProperty; break;
        default:
            return nullptr; // unrecognized property
        }

        command = new ChangeMapObject(mMapDocument, mapObject, property, val);
        break;
    }
    case XProperty: {
        const QPointF oldPos = mapObject->position();
        const QPointF newPos(val.toReal(), oldPos.y());
        command = new MoveMapObject(mMapDocument, mapObject, newPos, oldPos);
        break;
    }
    case YProperty: {
        const QPointF oldPos = mapObject->position();
        const QPointF newPos(oldPos.x(), InvertYAxisHelper(mMapDocument).pixelY(val.toReal()));
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
    case RotationProperty:
        if (mapObject->canRotate()) {
            const qreal newRotation = val.toDouble();
            const qreal oldRotation = mapObject->rotation();
            command = new RotateMapObject(mMapDocument, mapObject, newRotation, oldRotation);
        }
        break;
    case FlippingProperty: {
        const int flippingFlags = val.toInt();

        MapObjectCell mapObjectCell;
        mapObjectCell.object = mapObject;
        mapObjectCell.cell = mapObject->cell();
        mapObjectCell.cell.setFlippedHorizontally(flippingFlags & 1);
        mapObjectCell.cell.setFlippedVertically(flippingFlags & 2);

        command = new ChangeMapObjectCells(mMapDocument,
                                           QVector<MapObjectCell>() << mapObjectCell);

        command->setText(QCoreApplication::translate("Undo Commands",
                                                     "Flip %n Object(s)",
                                                     nullptr,
                                                     mMapDocument->selectedObjects().size()));
        break;
    }
    }

    return command;
}

void PropertyBrowser::applyMapObjectValue(PropertyId id, const QVariant &val)
{
    MapObject *mapObject = static_cast<MapObject*>(mObject);

    QUndoCommand *command = applyMapObjectValueTo(id, val, mapObject);
    if (!command)
        return;

    mDocument->undoStack()->beginMacro(command->text());
    mDocument->undoStack()->push(command);

    for (MapObject *obj : mMapDocument->selectedObjects()) {
        if (obj != mapObject) {
            if (QUndoCommand *cmd = applyMapObjectValueTo(id, val, obj))
                mDocument->undoStack()->push(cmd);
        }
    }

    mDocument->undoStack()->endMacro();
}

void PropertyBrowser::applyLayerValue(PropertyId id, const QVariant &val)
{
    Layer *currentLayer = static_cast<Layer*>(mObject);

    QUndoCommand *command = applyLayerValueTo(id, val, currentLayer);
    if (!command)
        return;

    mDocument->undoStack()->beginMacro(command->text());
    mDocument->undoStack()->push(command);

    for (Layer *layer : mMapDocument->selectedLayers()) {
        if (layer != currentLayer) {
            if (QUndoCommand *cmd = applyLayerValueTo(id, val, layer))
                mDocument->undoStack()->push(cmd);
        }
    }

    mDocument->undoStack()->endMacro();
}

QUndoCommand *PropertyBrowser::applyLayerValueTo(PropertyBrowser::PropertyId id, const QVariant &val, Layer *layer)
{
    QUndoCommand *command = nullptr;

    switch (id) {
    case NameProperty:
        command = new RenameLayer(mMapDocument, layer, val.toString());
        break;
    case VisibleProperty:
        command = new SetLayerVisible(mMapDocument, layer, val.toBool());
        break;
    case LockedProperty:
        command = new SetLayerLocked(mMapDocument, layer, val.toBool());
        break;
    case OpacityProperty:
        command = new SetLayerOpacity(mMapDocument, layer, val.toDouble());
        break;
    case OffsetXProperty:
    case OffsetYProperty: {
        QPointF offset = layer->offset();

        if (id == OffsetXProperty)
            offset.setX(val.toDouble());
        else
            offset.setY(val.toDouble());

        command = new SetLayerOffset(mMapDocument, layer, offset);
        break;
    }
    default:
        switch (layer->layerType()) {
        case Layer::TileLayerType:
            command = applyTileLayerValueTo(id, val, static_cast<TileLayer*>(layer));
            break;
        case Layer::ObjectGroupType:
            command = applyObjectGroupValueTo(id, val, static_cast<ObjectGroup*>(layer));
            break;
        case Layer::ImageLayerType:
            command = applyImageLayerValueTo(id, val, static_cast<ImageLayer*>(layer));
            break;
        case Layer::GroupLayerType:
            command = applyGroupLayerValueTo(id, val, static_cast<GroupLayer*>(layer));
            break;
        }
        break;
    }

    return command;
}

QUndoCommand *PropertyBrowser::applyTileLayerValueTo(PropertyId id, const QVariant &val, TileLayer *tileLayer)
{
    Q_UNUSED(id)
    Q_UNUSED(val)
    Q_UNUSED(tileLayer)

    return nullptr;
}

QUndoCommand *PropertyBrowser::applyObjectGroupValueTo(PropertyId id, const QVariant &val, ObjectGroup *objectGroup)
{
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

    return command;
}

QUndoCommand *PropertyBrowser::applyImageLayerValueTo(PropertyId id, const QVariant &val, ImageLayer *imageLayer)
{
    QUndoCommand *command = nullptr;

    switch (id) {
    case ImageSourceProperty: {
        const FilePath imageSource = val.value<FilePath>();
        const QColor &color = imageLayer->transparentColor();
        command = new ChangeImageLayerProperties(mMapDocument,
                                                 imageLayer,
                                                 color,
                                                 imageSource.url);
        break;
    }
    case ColorProperty: {
        const QColor color = val.value<QColor>();
        const QUrl &imageSource = imageLayer->imageSource();
        command = new ChangeImageLayerProperties(mMapDocument,
                                                 imageLayer,
                                                 color,
                                                 imageSource);
        break;
    }
    default:
        break;
    }

    return command;
}

QUndoCommand *PropertyBrowser::applyGroupLayerValueTo(PropertyId id, const QVariant &val, GroupLayer *groupLayer)
{
    Q_UNUSED(id)
    Q_UNUSED(val)
    Q_UNUSED(groupLayer)

    return nullptr;
}

void PropertyBrowser::applyTilesetValue(PropertyId id, const QVariant &val)
{
    Tileset *tileset = static_cast<Tileset*>(mObject);
    QUndoStack *undoStack = mDocument->undoStack();

    switch (id) {
    case FileNameProperty: {
        FilePath filePath = val.value<FilePath>();
        QString error;
        SharedTileset newTileset = TilesetManager::instance()->loadTileset(filePath.url.toLocalFile(), &error);
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
    case OrientationProperty: {
        Q_ASSERT(mTilesetDocument);
        auto orientation = static_cast<Tileset::Orientation>(val.toInt());
        undoStack->push(new ChangeTilesetOrientation(mTilesetDocument,
                                                     orientation));
        break;
    }
    case GridWidthProperty: {
        Q_ASSERT(mTilesetDocument);
        QSize gridSize = tileset->gridSize();
        gridSize.setWidth(val.toInt());
        undoStack->push(new ChangeTilesetGridSize(mTilesetDocument,
                                                  gridSize));
        break;
    }
    case GridHeightProperty: {
        Q_ASSERT(mTilesetDocument);
        QSize gridSize = tileset->gridSize();
        gridSize.setHeight(val.toInt());
        undoStack->push(new ChangeTilesetGridSize(mTilesetDocument,
                                                  gridSize));
        break;
    }
    case ColumnCountProperty:
        Q_ASSERT(mTilesetDocument);
        undoStack->push(new ChangeTilesetColumnCount(mTilesetDocument,
                                                     val.toInt()));
        break;
    case BackgroundColorProperty:
        Q_ASSERT(mTilesetDocument);
        undoStack->push(new ChangeTilesetBackgroundColor(mTilesetDocument,
                                                         val.value<QColor>()));
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
    case TypeProperty:
        undoStack->push(new ChangeTileType(mTilesetDocument,
                                           mTilesetDocument->selectedTiles(),
                                           val.toString()));
        break;
    case TileProbabilityProperty:
        undoStack->push(new ChangeTileProbability(mTilesetDocument,
                                                  mTilesetDocument->selectedTiles(),
                                                  val.toFloat()));
        break;
    case ImageSourceProperty: {
        const FilePath filePath = val.value<FilePath>();
        undoStack->push(new ChangeTileImageSource(mTilesetDocument,
                                                  tile, filePath.url));
        break;
    }
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

void PropertyBrowser::applyWangSetValue(PropertyId id, const QVariant &val)
{
    Q_ASSERT(mTilesetDocument);

    WangSet *wangSet = static_cast<WangSet*>(mObject);

    switch (id) {
    case NameProperty:
        mDocument->undoStack()->push(new RenameWangSet(mTilesetDocument,
                                                       wangSet,
                                                       val.toString()));
        break;
    case EdgeCountProperty:
        mDocument->undoStack()->push(new ChangeWangSetEdgeCount(mTilesetDocument,
                                                                wangSet,
                                                                val.toInt()));
        break;
    case CornerCountProperty:
        mDocument->undoStack()->push(new ChangeWangSetCornerCount(mTilesetDocument,
                                                                  wangSet,
                                                                  val.toInt()));
        break;
    default:
        break;
    }
}

void PropertyBrowser::applyWangColorValue(PropertyId id, const QVariant &val)
{
    Q_ASSERT(mTilesetDocument);

    WangColor *wangColor = static_cast<WangColor*>(mObject);

    switch (id) {
    case NameProperty:
        mDocument->undoStack()->push(new ChangeWangColorName(mTilesetDocument,
                                                             wangColor,
                                                             val.toString()));
        break;
    case ColorProperty:
        mDocument->undoStack()->push(new ChangeWangColorColor(mTilesetDocument,
                                                              wangColor,
                                                              val.value<QColor>()));
        break;
    case WangColorProbabilityProperty:
        mDocument->undoStack()->push(new ChangeWangColorProbability(mTilesetDocument,
                                                                    wangColor,
                                                                    val.toDouble()));
        break;
    default:
        break;
    }
}

/**
 * @warning This function does not add the property to the view.
 */
QtVariantProperty *PropertyBrowser::createProperty(PropertyId id, int type,
                                                   const QString &name)
{
    QtVariantProperty *property = mVariantManager->addProperty(type, name);
    if (!property) {
        // fall back to string property for unsupported property types
        property = mVariantManager->addProperty(QVariant::String, name);
    }

    if (type == QVariant::Bool)
        property->setAttribute(QLatin1String("textVisible"), false);
    if (type == QVariant::String && id == CustomProperty)
        property->setAttribute(QLatin1String("multiline"), true);
    if (type == QVariant::Double && id == CustomProperty)
        property->setAttribute(QLatin1String("decimals"), 9);

    mPropertyToId.insert(property, id);

    if (id != CustomProperty) {
        Q_ASSERT(!mIdToProperty.contains(id));
        mIdToProperty.insert(id, property);
    } else {
        Q_ASSERT(!mNameToProperty.contains(name));
        mNameToProperty.insert(name, property);
    }

    return property;
}

QtVariantProperty *PropertyBrowser::addProperty(PropertyId id, int type,
                                                const QString &name,
                                                QtProperty *parent)
{
    QtVariantProperty *property = createProperty(id, type, name);

    parent->addSubProperty(property);

    if (id == CustomProperty) {
        // Collapse custom color properties, to save space
        if (type == QVariant::Color)
            setExpanded(items(property).first(), false);

        if (mObject->isPartOfTileset())
            property->setEnabled(mTilesetDocument);
    }

    return property;
}

QtVariantProperty *PropertyBrowser::createCustomProperty(const QString &name, const QVariant &value)
{
    // Determine the property preceding the new property, if any
    const QList<QtProperty *> properties = mCustomPropertiesGroup->subProperties();
    QtProperty *precedingProperty = nullptr;
    for (int i = 0; i < properties.size(); ++i) {
        if (properties.at(i)->propertyName() < name)
            precedingProperty = properties.at(i);
        else
            break;
    }

    mUpdating = true;
    QtVariantProperty *property = createProperty(CustomProperty, value.userType(), name);
    property->setValue(value);
    mCustomPropertiesGroup->insertSubProperty(property, precedingProperty);

    // Collapse custom color properties, to save space
    if (value.type() == QVariant::Color)
        setExpanded(items(property).first(), false);

    mUpdating = false;
    return property;
}

void PropertyBrowser::deleteCustomProperty(QtVariantProperty *property)
{
    Q_ASSERT(mNameToProperty.contains(property->propertyName()));
    mNameToProperty.remove(property->propertyName());
    delete property;
}

void PropertyBrowser::setCustomPropertyValue(QtVariantProperty *property,
                                             const QVariant &value)
{
    if (value.userType() != property->valueType()) {
        // Re-creating the property is necessary to change its type
        const QString name = property->propertyName();
        const bool wasCurrent = currentItem() && currentItem()->property() == property;

        deleteCustomProperty(property);
        property = createCustomProperty(name, value);
        updateCustomPropertyColor(name);

        if (wasCurrent)
            setCurrentItem(items(property).first());
    } else {
        mUpdating = true;
        property->setValue(value);
        mUpdating = false;
    }
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
        case Layer::GroupLayerType:     addGroupLayerProperties();  break;
        }
        break;
    case Object::TilesetType:           addTilesetProperties(); break;
    case Object::TileType:              addTileProperties(); break;
    case Object::TerrainType:           addTerrainProperties(); break;
    case Object::WangSetType:           addWangSetProperties(); break;
    case Object::WangColorType:         addWangColorProperties(); break;
    case Object::ObjectTemplateType:    break;
    }

    // Make sure the color and font properties are collapsed, to save space
    if (QtProperty *colorProperty = mIdToProperty.value(ColorProperty))
        setExpanded(items(colorProperty).first(), false);
    if (QtProperty *colorProperty = mIdToProperty.value(BackgroundColorProperty))
        setExpanded(items(colorProperty).first(), false);
    if (QtProperty *fontProperty = mIdToProperty.value(FontProperty))
        setExpanded(items(fontProperty).first(), false);

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
        mIdToProperty[InfiniteProperty]->setValue(map->infinite());
        mIdToProperty[OrientationProperty]->setValue(map->orientation() - 1);
        mIdToProperty[HexSideLengthProperty]->setValue(map->hexSideLength());
        mIdToProperty[StaggerAxisProperty]->setValue(map->staggerAxis());
        mIdToProperty[StaggerIndexProperty]->setValue(map->staggerIndex());
        mIdToProperty[LayerFormatProperty]->setValue(map->layerDataFormat());
        mIdToProperty[RenderOrderProperty]->setValue(map->renderOrder());
        mIdToProperty[BackgroundColorProperty]->setValue(map->backgroundColor());
        break;
    }
    case Object::MapObjectType: {
        const MapObject *mapObject = static_cast<const MapObject*>(mObject);
        const int flags = mapObjectFlags(mapObject);

        if (mMapObjectFlags != flags) {
            removeProperties();
            addProperties();
            return;
        }

        const QString &type = mapObject->effectiveType();
        const auto typeColorGroup = mapObject->type().isEmpty() ? QPalette::Disabled
                                                                : QPalette::Active;

        FilePath templateFilePath;
        if (auto objectTemplate = mapObject->objectTemplate())
            templateFilePath.url = QUrl::fromLocalFile(objectTemplate->fileName());

        mIdToProperty[IdProperty]->setValue(mapObject->id());
        mIdToProperty[TemplateProperty]->setValue(QVariant::fromValue(templateFilePath));
        mIdToProperty[NameProperty]->setValue(mapObject->name());
        mIdToProperty[TypeProperty]->setValue(type);
        mIdToProperty[TypeProperty]->setValueColor(palette().color(typeColorGroup, QPalette::WindowText));
        if (auto visibleProperty = mIdToProperty[VisibleProperty])
            visibleProperty->setValue(mapObject->isVisible());
        mIdToProperty[XProperty]->setValue(mapObject->x());
        mIdToProperty[YProperty]->setValue(InvertYAxisHelper(mMapDocument).pixelY(mapObject->y()));

        if (flags & ObjectHasDimensions) {
            mIdToProperty[WidthProperty]->setValue(mapObject->width());
            mIdToProperty[HeightProperty]->setValue(mapObject->height());
        }

        mIdToProperty[RotationProperty]->setValue(mapObject->rotation());

        if (flags & ObjectHasTile) {
            int flippingFlags = 0;
            if (mapObject->cell().flippedHorizontally())
                flippingFlags |= 1;
            if (mapObject->cell().flippedVertically())
                flippingFlags |= 2;
            mIdToProperty[FlippingProperty]->setValue(flippingFlags);
        }

        if (flags & ObjectIsText) {
            const auto& textData = mapObject->textData();
            mIdToProperty[TextProperty]->setValue(textData.text);
            mIdToProperty[FontProperty]->setValue(textData.font);
            mIdToProperty[TextAlignmentProperty]->setValue(QVariant::fromValue(textData.alignment));
            mIdToProperty[WordWrapProperty]->setValue(textData.wordWrap);
            mIdToProperty[ColorProperty]->setValue(textData.color);
        }
        break;
    }
    case Object::LayerType: {
        const Layer *layer = static_cast<const Layer*>(mObject);

        mIdToProperty[IdProperty]->setValue(layer->id());
        mIdToProperty[NameProperty]->setValue(layer->name());
        mIdToProperty[VisibleProperty]->setValue(layer->isVisible());
        mIdToProperty[LockedProperty]->setValue(layer->isLocked());
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
        case Layer::ImageLayerType: {
            const ImageLayer *imageLayer = static_cast<const ImageLayer*>(layer);
            mIdToProperty[ImageSourceProperty]->setValue(QVariant::fromValue(FilePath { imageLayer->imageSource() }));
            mIdToProperty[ColorProperty]->setValue(imageLayer->transparentColor());
            break;
        }
        case Layer::GroupLayerType:
            break;
        }
        break;
    }
    case Object::TilesetType: {
        Tileset *tileset = static_cast<Tileset*>(mObject);

        if (QtVariantProperty *fileNameProperty = mIdToProperty.value(FileNameProperty))
            fileNameProperty->setValue(QVariant::fromValue(FilePath { QUrl::fromLocalFile(tileset->fileName()) }));

        mIdToProperty[BackgroundColorProperty]->setValue(tileset->backgroundColor());

        mIdToProperty[NameProperty]->setValue(tileset->name());
        mIdToProperty[TileOffsetProperty]->setValue(tileset->tileOffset());
        mIdToProperty[OrientationProperty]->setValue(tileset->orientation());
        mIdToProperty[GridWidthProperty]->setValue(tileset->gridSize().width());
        mIdToProperty[GridHeightProperty]->setValue(tileset->gridSize().height());
        mIdToProperty[ColumnCountProperty]->setValue(tileset->columnCount());
        mIdToProperty[ColumnCountProperty]->setEnabled(mTilesetDocument && tileset->isCollection());

        if (!tileset->isCollection()) {
            mIdToProperty[TilesetImageParametersProperty]->setValue(QVariant::fromValue(mTilesetDocument));
            mIdToProperty[ImageSourceProperty]->setValue(tileset->imageSource().toString(QUrl::PreferLocalFile));
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
        mIdToProperty[TypeProperty]->setValue(tile->type());
        mIdToProperty[WidthProperty]->setValue(tileSize.width());
        mIdToProperty[HeightProperty]->setValue(tileSize.height());
        mIdToProperty[TileProbabilityProperty]->setValue(tile->probability());
        if (QtVariantProperty *imageSourceProperty = mIdToProperty.value(ImageSourceProperty))
            imageSourceProperty->setValue(QVariant::fromValue(FilePath { tile->imageSource() }));
        break;
    }
    case Object::TerrainType: {
        const Terrain *terrain = static_cast<const Terrain*>(mObject);
        mIdToProperty[NameProperty]->setValue(terrain->name());
        break;
    }
    case Object::WangSetType: {
        const WangSet *wangSet = static_cast<const WangSet*>(mObject);
        mIdToProperty[NameProperty]->setValue(wangSet->name());
        mIdToProperty[EdgeCountProperty]->setValue(wangSet->edgeColorCount());
        mIdToProperty[CornerCountProperty]->setValue(wangSet->cornerColorCount());
        break;
    }
    case Object::WangColorType: {
        const WangColor *wangColor = static_cast<const WangColor*>(mObject);
        mIdToProperty[NameProperty]->setValue(wangColor->name());
        mIdToProperty[ColorProperty]->setValue(wangColor->color());
        mIdToProperty[WangColorProbabilityProperty]->setValue(wangColor->probability());
        break;
    }
    case Object::ObjectTemplateType:
        break;
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

    QString objectType;

    switch (mObject->typeId()) {
    case Object::TileType:
        objectType = static_cast<Tile*>(mObject)->type();
        break;
    case Object::MapObjectType: {
        auto mapObject = static_cast<MapObject*>(mObject);
        objectType = mapObject->type();

        // Inherit properties from the template
        if (const MapObject *templateObject = mapObject->templateObject()) {
            QMapIterator<QString,QVariant> it(templateObject->properties());
            while (it.hasNext()) {
                it.next();
                if (!mCombinedProperties.contains(it.key()))
                    mCombinedProperties.insert(it.key(), it.value());
            }
        }

        if (Tile *tile = mapObject->cell().tile()) {
            if (objectType.isEmpty())
                objectType = tile->type();

            // Inherit properties from the tile
            QMapIterator<QString,QVariant> it(tile->properties());
            while (it.hasNext()) {
                it.next();
                if (!mCombinedProperties.contains(it.key()))
                    mCombinedProperties.insert(it.key(), it.value());
            }
        }
        break;
    }
    case Object::LayerType:
    case Object::MapType:
    case Object::TerrainType:
    case Object::TilesetType:
    case Object::WangSetType:
    case Object::WangColorType:
    case Object::ObjectTemplateType:
        break;
    }

    if (!objectType.isEmpty()) {
        // Inherit properties from the object type
        for (const ObjectType &type : Object::objectTypes()) {
            if (type.name == objectType) {
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
        QtVariantProperty *property = addProperty(CustomProperty,
                                                  it.value().userType(),
                                                  it.key(),
                                                  mCustomPropertiesGroup);

        property->setValue(it.value());
        updateCustomPropertyColor(it.key());
    }

    mUpdating = wasUpdating;
}

// If there are other objects selected check if their properties are equal. If not give them a gray color.
void PropertyBrowser::updateCustomPropertyColor(const QString &name)
{
    QtVariantProperty *property = mNameToProperty.value(name);
    if (!property)
        return;
    if (!property->isEnabled())
        return;

    QString propertyName = property->propertyName();
    QString propertyValue = property->valueText();

    const auto &objects = mDocument->currentObjects();

    QColor textColor = palette().color(QPalette::Active, QPalette::WindowText);
    QColor disabledTextColor = palette().color(QPalette::Disabled, QPalette::WindowText);

    // If one of the objects doesn't have this property then gray out the name and value.
    for (Object *obj : objects) {
        if (!obj->hasProperty(propertyName)) {
            property->setNameColor(disabledTextColor);
            property->setValueColor(disabledTextColor);
            return;
        }
    }

    // If one of the objects doesn't have the same property value then gray out the value.
    for (Object *obj : objects) {
        if (obj == mObject)
            continue;
        if (obj->property(propertyName) != propertyValue) {
            property->setNameColor(textColor);
            property->setValueColor(disabledTextColor);
            return;
        }
    }

    property->setNameColor(textColor);
    property->setValueColor(textColor);
}

void PropertyBrowser::retranslateUi()
{
    mStaggerAxisNames.clear();
    mStaggerIndexNames.clear();
    mOrientationNames.clear();
    mTilesetOrientationNames.clear();
    mLayerFormatNames.clear();
    mRenderOrderNames.clear();
    mFlippingFlagNames.clear();
    mDrawOrderNames.clear();

    mStaggerAxisNames.append(tr("X"));
    mStaggerAxisNames.append(tr("Y"));

    mStaggerIndexNames.append(tr("Odd"));
    mStaggerIndexNames.append(tr("Even"));

    mOrientationNames.append(QCoreApplication::translate("Tiled::NewMapDialog", "Orthogonal"));
    mOrientationNames.append(QCoreApplication::translate("Tiled::NewMapDialog", "Isometric"));
    mOrientationNames.append(QCoreApplication::translate("Tiled::NewMapDialog", "Isometric (Staggered)"));
    mOrientationNames.append(QCoreApplication::translate("Tiled::NewMapDialog", "Hexagonal (Staggered)"));

    mTilesetOrientationNames.append(mOrientationNames.at(0));
    mTilesetOrientationNames.append(mOrientationNames.at(1));

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

    removeProperties();
    addProperties();
}

} // namespace Tiled
