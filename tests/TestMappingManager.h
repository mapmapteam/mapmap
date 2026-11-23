/*
 * TestMappingManager.h
 *
 * Unit tests for MappingManager: adding, removing and finding sources and
 * layers by id and by name pattern (the lookup used by OSC control).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TEST_MAPPING_MANAGER_H_
#define TEST_MAPPING_MANAGER_H_

#include <QtTest/QtTest>

class TestMappingManager : public QObject
{
  Q_OBJECT

private slots:
  void addAndFindSources();
  void addAndFindLayers();
  void newLayersGoOnTop();
  void findByNamePattern();
  void removeLayer();
  void removeSourceRemovesItsLayers();
  void unknownIdsReturnNull();
  void clearAll();
  void outputResolutionPresets();
};

#endif /* TEST_MAPPING_MANAGER_H_ */
