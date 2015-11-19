#include <QDebug>
#include <QGuiApplication>
#include <QCommandLineParser>

#include "mapformat.h"
#include "mapreader.h"
#include "mapwriter.h"

using namespace Tiled;

template <typename Format>
Format *findFormat(QString const &fileName) {
  FormatHelper<Format> formatHelper(FileFormat::ReadWrite, "");
  foreach (Format *format, formatHelper.formats()) {
    qDebug() << "Checking if " << format->nameFilter() << " supports " << fileName;
    if (format->supportsFile(fileName))
      return format;
  }
  return nullptr;
}

Map *readMap(QString const &fileName) {
  MapFormat *format = findFormat<MapFormat>(fileName);
  if (format) {
    qDebug() << "Reading " << format->nameFilter();
    return format->read(fileName);
  } else {
    qDebug() << "Reading TMX format";
    MapReader reader;
    return reader.readMap(fileName);
  }
}

void writeMap(Tiled::Map *map, QString const &fileName) {
  MapFormat *format = findFormat<MapFormat>(fileName);
  if (format) {
    qDebug() << "Writing " << format->nameFilter();
    format->write(map, fileName);
  } else {
    qDebug() << "Writing TMX format";
    MapWriter writer;
    writer.writeMap(map, fileName);
  }
}

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  QGuiApplication::setApplicationName("tmxtojson");
  QGuiApplication::setApplicationVersion("0.1");

  QCommandLineParser parser;
  parser.setApplicationDescription("Convert .tmx files to .json map files");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addPositionalArgument("source", QCoreApplication::translate("tmxtojson", ".tmx file to read"));
  parser.addPositionalArgument("destination", QCoreApplication::translate("tmxtojson", ".json file to write"));

  parser.process(app);

  QStringList const args = parser.positionalArguments();

  PluginManager::instance()->loadPlugins();
  foreach (LoadedPlugin plugin, PluginManager::instance()->plugins()) {
    qDebug() << "Loaded plugin " << plugin.fileName;
  }

  Tiled::Map *map = readMap(args.at(0));
  writeMap(map, args.at(1));

  return 0;
}
