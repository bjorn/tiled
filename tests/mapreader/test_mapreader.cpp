#include "map.h"
#include "mapobject.h"
#include "objectgroup.h"
#include "tilelayer.h"
#include "mapreader.h"

#include <QtTest/QtTest>

using namespace Tiled;

class test_MapReader : public QObject
{
    Q_OBJECT

private slots:
    void loadMap();
};

void test_MapReader::loadMap()
{
    MapReader reader;
    auto map = reader.readMap(":maps/mapobject.tmx");

    // TODO: Also test tilesets (internal and external), properties and tile
    // layer data.

    QVERIFY(map.get());
    QCOMPARE(map->layerCount(), 2);
    QCOMPARE(map->width(), 100);
    QCOMPARE(map->height(), 80);
    QCOMPARE(map->tileWidth(), 32);
    QCOMPARE(map->tileHeight(), 32);

    TileLayer *tileLayer = dynamic_cast<TileLayer*>(map->layerAt(0));

    QVERIFY(tileLayer);
    QCOMPARE(tileLayer->width(), 100);
    QCOMPARE(tileLayer->height(), 80);

    ObjectGroup *objectGroup = dynamic_cast<ObjectGroup*>(map->layerAt(1));

    QVERIFY(objectGroup);
    QCOMPARE(objectGroup->name(), QLatin1String("Objects"));
    QCOMPARE(objectGroup->objects().count(), 1);

    MapObject *mapObject = objectGroup->objects().at(0);

    QCOMPARE(mapObject->name(), QLatin1String("Some object"));
    QCOMPARE(mapObject->type(), QLatin1String("WARP"));
    QCOMPARE(mapObject->x(), qreal(200));
    QCOMPARE(mapObject->y(), qreal(200));
    QCOMPARE(mapObject->width(), qreal(128));
    QCOMPARE(mapObject->height(), qreal(64));
}

QTEST_MAIN(test_MapReader)
#include "test_mapreader.moc"
