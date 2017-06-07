/*
 * templategroup.h
 * Copyright 2017, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2017, Mohamed Thabet <thabetx@gmail.com>
 *
 * This file is part of libtiled.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "tiled_global.h"

#include "objecttemplate.h"

namespace Tiled {

class MapObject;
class TemplateGroupFormat;

class TILEDSHARED_EXPORT TemplateGroup
{
public:
    TemplateGroup();
    TemplateGroup(QString name);
    ~TemplateGroup();

    const QList<ObjectTemplate*> &templates() const;
    void addTemplate(ObjectTemplate *objectTemplate);

    const QVector<SharedTileset> &tilesets() const;
    bool addTileset(const SharedTileset &tileset);

    const QString &name() const;
    void setName(const QString &name);

    void setFormat(TemplateGroupFormat *format);
    TemplateGroupFormat *format() const;

private:
    QList<ObjectTemplate*> mTemplates;
    QVector<SharedTileset> mTilesets;
    TemplateGroupFormat *mFormat;
    QString mName;
};

inline const QList<ObjectTemplate*> &TemplateGroup::templates() const
{ return mTemplates; }

inline const QVector<SharedTileset> &TemplateGroup::tilesets() const
{ return mTilesets; }

inline void TemplateGroup::setName(const QString &name)
{ mName = name; }

inline const QString &TemplateGroup::name() const
{ return mName; }

inline void TemplateGroup::setFormat(TemplateGroupFormat *format)
{ mFormat = format; }

inline TemplateGroupFormat *TemplateGroup::format() const
{ return mFormat; }

} // namespace Tiled
