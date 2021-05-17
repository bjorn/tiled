#include "changecomponents.h"

#include "object.h"
#include "document.h"

#include <QCoreApplication>

using namespace Tiled;

static Properties objectTypeProperties(const QString &name)
{
    const ObjectType *type = nullptr;
    for (const ObjectType &t : Object::objectTypes()) {
        if (t.name.compare(name) == 0) {
            type = &t;
            break;
        }
    }

    return type ? type->defaultProperties : Properties {};
}

AddComponent::AddComponent(Document *document,
                           QList<Object *> objects,
                           const QString &name,
                           QUndoCommand *parent)
    : QUndoCommand(parent)
    , mDocument(document)
    , mObjects(objects)
    , mName(name)
    , mProperties(objectTypeProperties(mName))
{
    QString caption = QCoreApplication::translate("Undo Commands", "Add Component (%1)");
    caption = caption.arg(mName);
    setText(caption);
}

void AddComponent::undo()
{
    mDocument->removeComponent(mObjects, mName);
}

void AddComponent::redo()
{
    mDocument->addComponent(mObjects, mName, mProperties);
}


RemoveComponent::RemoveComponent(Document *document,
                                 QList<Object *> objects,
                                 const QString &componentName,
                                 QUndoCommand *parent)
    : QUndoCommand(parent)
    , mDocument(document)
    , mObjects(objects)
    , mComponentName(componentName)
{
    QString caption = QCoreApplication::translate("Undo Commands", "Remove Component (%1)");
    caption = caption.arg(mComponentName);
    setText(caption);
}

void RemoveComponent::undo()
{
    for (int i = 0; i < mObjects.size(); i++) {
        mDocument->addComponent({ mObjects.at(i) }, mComponentName, mProperties.at(i));
    }
}

void RemoveComponent::redo()
{
    mProperties.reserve(mObjects.size());
    for (int i = 0; i < mObjects.size(); i++) {
        mProperties.append(mObjects[i]->componentProperties(mComponentName));
    }

    mDocument->removeComponent(mObjects, mComponentName);
}


SetComponentProperty::SetComponentProperty(Document *document,
                                           QList<Object *> objects,
                                           const QString &componentName,
                                           const QString &propertyName,
                                           QVariant value,
                                           QUndoCommand *parent)
    : QUndoCommand(parent)
    , mDocument(document)
    , mObjects(objects)
    , mComponentName(componentName)
    , mPropertyName(propertyName)
    , mNewValue(value)
{
    setText(QCoreApplication::translate("Undo Commands", "Set Property"));

    mOldValues.reserve(objects.size());

    for (int i = 0; i < objects.size(); i++) {
        Object *object = objects.at(i);
        Properties &props = object->componentProperties(componentName);
        mOldValues.append(props[mPropertyName]);
    }
}

void SetComponentProperty::undo()
{
    for (int i = 0; i < mObjects.size(); i++) {
        Object *object = mObjects.at(i);
        QVariant oldValue = mOldValues.at(i);
        mDocument->setComponentProperty(object, mComponentName, mPropertyName, oldValue);
    }
}

void SetComponentProperty::redo()
{
    for (int i = 0; i < mObjects.size(); i++) {
        Object *object = mObjects.at(i);
        mDocument->setComponentProperty(object, mComponentName, mPropertyName, mNewValue);
    }
}
