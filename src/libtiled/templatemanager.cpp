/*
 * templatemanager.cpp
 * Copyright 2017, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2017, Mohamed Thabet <thabetx@gmail.com>
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

#include "templatemanager.h"

#include <memory>

#include "objecttemplate.h"
#include "objecttemplateformat.h"

using namespace Tiled;

TemplateManager *TemplateManager::mInstance;

TemplateManager *TemplateManager::instance()
{
    if (!mInstance)
        mInstance = new TemplateManager;

    return mInstance;
}

void TemplateManager::deleteInstance()
{
    delete mInstance;
    mInstance = nullptr;
}

TemplateManager::TemplateManager(QObject *parent)
    : QObject(parent),
      mWatcher(new FileSystemWatcher(this))
{
    connect(mWatcher, &FileSystemWatcher::fileChanged,
            this, &TemplateManager::fileChanged);
}

TemplateManager::~TemplateManager()
{
    qDeleteAll(mObjectTemplates);
    delete mWatcher;
}

ObjectTemplate *TemplateManager::loadObjectTemplate(const QString &fileName, QString *error)
{
    ObjectTemplate *objectTemplate = findObjectTemplate(fileName);

    if (!objectTemplate) {
        auto newTemplate = readObjectTemplate(fileName, error);

        if (newTemplate) {
            mWatcher->addPath(fileName);
        } else {
            // This instance will not have an object. It is used to detect broken
            // template references.
            newTemplate = std::make_unique<ObjectTemplate>(fileName);
        }

        objectTemplate = newTemplate.get();
        mObjectTemplates.insert(fileName, newTemplate.release());
    }

    return objectTemplate;
}

void TemplateManager::fileChanged(const QString &fileName)
{
    ObjectTemplate *objectTemplate = findObjectTemplate(fileName);

    // Most likely the file was removed.
    if (!objectTemplate)
        return;

    emit objectTemplateChanged(objectTemplate);
}
