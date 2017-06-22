/*
 * JSON Tiled Plugin
 * Copyright 2017, Chase Warrington <spacechase0.and.cat@gmail.com>
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

#include "tbinplugin.h"

#include "tbin/Map.hpp"
#include "map.h"
#include "tile.h"

#include <map>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QDebug>
#include <sstream>

namespace
{
    void doProperties( Tiled::Object* obj, const tbin::Properties& props )
    {
        for ( const std::pair< std::string, tbin::PropertyValue >& prop : props )
        {
            switch ( prop.second.type )
            {
                case tbin::PropertyValue::String:
                    obj->setProperty( prop.first.c_str(), QVariant( prop.second.dataStr.c_str() ) );
                    break;

                case tbin::PropertyValue::Bool:
                    obj->setProperty( prop.first.c_str(), QVariant( prop.second.data.b ) );
                    break;

                case tbin::PropertyValue::Float:
                    obj->setProperty( prop.first.c_str(), QVariant( prop.second.data.f ) );
                    break;

                case tbin::PropertyValue::Integer:
                    obj->setProperty( prop.first.c_str(), QVariant( prop.second.data.i ) );
                    break;
            }
        }
    }
}

namespace Tbin {

void TbinPlugin::initialize()
{
    addObject(new TbinMapFormat(this));
}


TbinMapFormat::TbinMapFormat(QObject *parent)
{
}

Tiled::Map *TbinMapFormat::read(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        mError = tr("Could not open file for reading.");
        return nullptr;
    }

    QByteArray contents = file.readAll();
    std::istringstream ss( std::string( contents.constData(), contents.length() ) );

    tbin::Map tmap;
    Tiled::Map* map;
    try
    {
        tmap.loadFromStream( ss );
        map = new Tiled::Map( Tiled::Map::Orthogonal, tmap.layers[ 0 ].layerSize.x, tmap.layers[ 0 ].layerSize.y, tmap.layers[ 0 ].tileSize.x, tmap.layers[ 0 ].tileSize.y );

        int currFirstGid = 1;
        std::map< std::string, int > gids;
        for ( const tbin::TileSheet& ttilesheet : tmap.tilesheets )
        {
            if ( ttilesheet.spacing.x != ttilesheet.spacing.y )
                throw std::invalid_argument( "Tilesheet must have equal spacings" );
            if ( ttilesheet.margin.x != ttilesheet.margin.y )
                throw std::invalid_argument( "Tilesheet must have equal margins" );

            auto tilesheet = Tiled::Tileset::create( ttilesheet.id.c_str(), ttilesheet.tileSize.x, ttilesheet.tileSize.y, ttilesheet.spacing.x, ttilesheet.margin.x );
            tilesheet->setImageSource( ttilesheet.image.c_str() );
            doProperties( tilesheet.data(), ttilesheet.props );

            QList<Tiled::Tile*> tiles;
            for ( int i = 0; i < ttilesheet.sheetSize.x * ttilesheet.sheetSize.y; ++i )
            {
                tiles.append( new Tiled::Tile( i, tilesheet.data() ) );
            }
            tilesheet->addTiles( tiles );

            map->addTileset( tilesheet );
            gids.insert( std::make_pair( ttilesheet.id, currFirstGid ) );
            currFirstGid += ttilesheet.sheetSize.x * ttilesheet.sheetSize.y;
        }
        for ( const tbin::Layer& tlayer : tmap.layers )
        {

        }
    }
    catch ( std::exception& e )
    {
        mError = QString("Exception: ") + e.what();
    }

    return map;
}

bool TbinMapFormat::write(const Tiled::Map *map, const QString &fileName)
{
    // ...

    return false;
}

QString TbinMapFormat::nameFilter() const
{
    return tr("Tbin map files (*.tbin)");
}

QString TbinMapFormat::shortName() const
{
    return QLatin1String("tbin");
}

bool TbinMapFormat::supportsFile(const QString &fileName) const
{
    // ...

    return false;
}

QString TbinMapFormat::errorString() const
{
    return mError;
}

} // namespace Tbin
