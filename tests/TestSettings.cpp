/*
 * TestSettings.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "TestSettings.h"

#include <QColor>

#include "Source.h"

using namespace mmp;

void TestSettings::opacityIsClamped()
{
  Color source(QColor(Qt::red));
  source.setOpacity(5.0f);
  QCOMPARE(source.getOpacity(), 1.0f);
  source.setOpacity(-2.0f);
  QCOMPARE(source.getOpacity(), 0.0f);
}
