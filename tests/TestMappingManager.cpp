/*
 * TestMappingManager.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestMappingManager.h"

#include "MappingManager.h"
#include "Shapes.h"

using namespace mmp;

namespace {

Source::ptr makeColor(const QString& name)
{
  Source::ptr color(new Color());
  color->setName(name);
  return color;
}

Layer::ptr makeColorLayer(Source::ptr source, const QString& name)
{
  MShape::ptr quad(new Quad(QPointF(0, 0), QPointF(1, 0), QPointF(1, 1), QPointF(0, 1)));
  Layer::ptr layer(new ColorLayer(source, quad));
  layer->setName(name);
  return layer;
}

} // namespace

void TestMappingManager::addAndFindSources()
{
  MappingManager manager;
  Source::ptr red  = makeColor("red");
  Source::ptr blue = makeColor("blue");

  QCOMPARE(manager.addSource(red),  red->getId());
  QCOMPARE(manager.addSource(blue), blue->getId());

  QCOMPARE(manager.nSources(), 2);
  QCOMPARE(manager.getSource(0), red);
  QCOMPARE(manager.getSource(1), blue);
  QCOMPARE(manager.getSourceById(blue->getId()), blue);
  QCOMPARE(manager.getSourceByName("red"), red);
  QCOMPARE(manager.getSourceIndex(blue), 1);
}

void TestMappingManager::addAndFindLayers()
{
  MappingManager manager;
  Source::ptr red = makeColor("red");
  manager.addSource(red);
  Layer::ptr layer = makeColorLayer(red, "wall");

  QCOMPARE(manager.addLayer(layer), layer->getId());
  QCOMPARE(manager.nLayers(), 1);
  QCOMPARE(manager.getLayerById(layer->getId()), layer);
  QCOMPARE(manager.getLayerByName("wall"), layer);
  QCOMPARE(manager.getSourceLayers(red).size(), 1);
}

void TestMappingManager::newLayersGoOnTop()
{
  MappingManager manager;
  Source::ptr red = makeColor("red");
  manager.addSource(red);
  Layer::ptr first  = makeColorLayer(red, "first");
  Layer::ptr second = makeColorLayer(red, "second");
  manager.addLayer(first);
  manager.addLayer(second);

  // ProjectReader relies on this: it adds layers in reverse file order.
  QCOMPARE(manager.getLayer(0), second);
  QCOMPARE(manager.getLayer(1), first);
}

void TestMappingManager::findByNamePattern()
{
  MappingManager manager;
  Source::ptr red   = makeColor("red");
  Source::ptr green = makeColor("green");
  Source::ptr dark  = makeColor("dark red");
  manager.addSource(red);
  manager.addSource(green);
  manager.addSource(dark);

  // Wildcard patterns, as sent by OSC (eg. /mapmap/source/name/opacity).
  QCOMPARE(manager.getSourcesByNameRegExp("red").size(), 1);
  QCOMPARE(manager.getSourcesByNameRegExp("*red").size(), 2);
  QCOMPARE(manager.getSourcesByNameRegExp("*").size(), 3);
  QCOMPARE(manager.getSourcesByNameRegExp("blue").size(), 0);

  manager.addLayer(makeColorLayer(red, "wall left"));
  manager.addLayer(makeColorLayer(red, "wall right"));
  manager.addLayer(makeColorLayer(green, "floor"));
  QCOMPARE(manager.getLayersByNameRegExp("wall*").size(), 2);
  QCOMPARE(manager.getLayersByNameRegExp("floor").size(), 1);
  QCOMPARE(manager.getLayersByNameRegExp("ceiling").size(), 0);
}

void TestMappingManager::removeLayer()
{
  MappingManager manager;
  Source::ptr red = makeColor("red");
  manager.addSource(red);
  Layer::ptr layer = makeColorLayer(red, "wall");
  manager.addLayer(layer);

  QVERIFY(manager.removeLayer(layer->getId()));
  QCOMPARE(manager.nLayers(), 0);
  QVERIFY(manager.getLayerById(layer->getId()).isNull());
  QCOMPARE(manager.nSources(), 1);

  QVERIFY(!manager.removeLayer(layer->getId()));
}

void TestMappingManager::removeSourceRemovesItsLayers()
{
  MappingManager manager;
  Source::ptr red   = makeColor("red");
  Source::ptr green = makeColor("green");
  manager.addSource(red);
  manager.addSource(green);
  manager.addLayer(makeColorLayer(red, "a"));
  manager.addLayer(makeColorLayer(red, "b"));
  Layer::ptr kept = makeColorLayer(green, "c");
  manager.addLayer(kept);

  QVERIFY(manager.removeSource(red->getId()));
  QCOMPARE(manager.nSources(), 1);
  QCOMPARE(manager.nLayers(), 1);
  QCOMPARE(manager.getLayer(0), kept);
  QVERIFY(manager.getSourceById(red->getId()).isNull());

  QVERIFY(!manager.removeSource(red->getId()));
}

void TestMappingManager::unknownIdsReturnNull()
{
  MappingManager manager;
  QVERIFY(manager.getSourceById(12345).isNull());
  QVERIFY(manager.getLayerById(12345).isNull());
  // Looking up an unknown id must not create an entry.
  QVERIFY(manager.getSourceById(12345).isNull());
  QCOMPARE(manager.nSources(), 0);
}

void TestMappingManager::clearAll()
{
  MappingManager manager;
  Source::ptr red = makeColor("red");
  manager.addSource(red);
  manager.addLayer(makeColorLayer(red, "wall"));

  manager.clearAll();
  QCOMPARE(manager.nSources(), 0);
  QCOMPARE(manager.nLayers(), 0);
  QVERIFY(manager.getSourceById(red->getId()).isNull());
}
