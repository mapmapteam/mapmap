/*
 * FrameStats.cpp
 *
 * (c) 2026 MapMap contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FrameStats.h"
#include "MM.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QMutexLocker>
#include <QStringList>
#include <QTimer>

Q_LOGGING_CATEGORY(lcFrameStats, "mapmap.framestats")

namespace mmp {

namespace {

// Time between two lines of the log, in milliseconds.
const int LOG_INTERVAL_MS = 1000;

// Decimals of the times shown in milliseconds.
const int MILLISECOND_DECIMALS = 2;

// Names of the steps in the summary lines, in the order of FrameStats::Stage.
const char* const STAGE_NAMES[FrameStats::STAGE_COUNT] = {
  "prepare", "upload", "render", "draw", "interval"
};

// Every live instance, in order of creation.
QMutex& _registryMutex()
{
  static QMutex mutex;
  return mutex;
}

QList<FrameStats*>& _registry()
{
  static QList<FrameStats*> registry;
  return registry;
}

QString _milliseconds(qint64 ns)
{
  return QString::number(qreal(ns) / MM::NANOSECONDS_PER_MILLISECOND, 'f', MILLISECOND_DECIMALS);
}

}

std::atomic<bool> FrameStats::_enabled(false);
std::atomic<bool> FrameStats::_synchronous(false);

bool FrameStats::Summary::isEmpty() const
{
  if (dropped > 0)
    return false;
  for (const StageSummary& stage : stages)
    if (stage.count > 0)
      return false;
  return true;
}

void FrameStats::Ring::add(qint64 atNs, qint64 durationNs)
{
  samples[next] = Sample{atNs, durationNs};
  next = (next + 1) % SAMPLE_CAPACITY;
  if (size < SAMPLE_CAPACITY)
    size++;
}

FrameStats::StageSummary FrameStats::Ring::summary(qint64 fromNs, qint64 toNs) const
{
  StageSummary result;
  qint64 totalNs = 0;
  for (int i = 0; i < size; i++)
  {
    const Sample& sample = samples[i];
    if (sample.atNs <= fromNs || sample.atNs > toNs)
      continue;
    result.count++;
    totalNs += sample.durationNs;
    result.maxNs = qMax(result.maxNs, sample.durationNs);
  }
  if (result.count > 0)
    result.meanNs = totalNs / result.count;
  return result;
}

FrameStats::FrameStats(const QString& name)
  : _name(name)
{
  QMutexLocker locker(&_registryMutex());
  _registry().append(this);
}

FrameStats::~FrameStats()
{
  QMutexLocker locker(&_registryMutex());
  _registry().removeOne(this);
}

void FrameStats::setEnabled(bool enabled)
{
  _enabled.store(enabled, std::memory_order_relaxed);
}

void FrameStats::setSynchronous(bool synchronous)
{
  _synchronous.store(synchronous, std::memory_order_relaxed);
}

qint64 FrameStats::now()
{
  // Started on first use (the initialization of a local static is thread-safe).
  static const QElapsedTimer clock = [] {
    QElapsedTimer timer;
    timer.start();
    return timer;
  }();
  return clock.nsecsElapsed();
}

void FrameStats::setName(const QString& name)
{
  QMutexLocker locker(&_mutex);
  _name = name;
}

QString FrameStats::name() const
{
  QMutexLocker locker(&_mutex);
  return _name;
}

void FrameStats::setPath(const QString& path)
{
  QMutexLocker locker(&_mutex);
  _path = path;
}

void FrameStats::record(Stage stage, qint64 durationNs, qint64 atNs)
{
  QMutexLocker locker(&_mutex);
  _stages[stage].add(atNs, durationNs);
}

void FrameStats::recordDropped(qint64 atNs)
{
  QMutexLocker locker(&_mutex);
  _dropped.add(atNs, 0);
}

FrameStats::Summary FrameStats::summary(qint64 atNs) const
{
  const qint64 fromNs = atNs - WINDOW_NS;
  QMutexLocker locker(&_mutex);
  Summary result;
  result.name = _name;
  result.path = _path;
  for (int stage = 0; stage < STAGE_COUNT; stage++)
    result.stages[stage] = _stages[stage].summary(fromNs, atNs);
  result.dropped = _dropped.summary(fromNs, atNs).count;
  return result;
}

QList<FrameStats::Summary> FrameStats::summaries(qint64 atNs)
{
  QMutexLocker locker(&_registryMutex());
  QList<Summary> result;
  for (const FrameStats* stats : std::as_const(_registry()))
    result.append(stats->summary(atNs));
  return result;
}

QString FrameStats::summaryLine(const Summary& summary)
{
  QStringList parts;
  for (int stage = 0; stage < STAGE_COUNT; stage++)
  {
    const StageSummary& s = summary.stages[stage];
    if (s.count == 0)
      continue;
    parts << QString("%1 %2 ms, max %3 ms, %4/s")
             .arg(STAGE_NAMES[stage], _milliseconds(s.meanNs), _milliseconds(s.maxNs))
             .arg(s.count);
  }
  if (summary.dropped > 0)
    parts << QString("dropped %1/s").arg(summary.dropped);

  const QString title = summary.path.isEmpty()
      ? summary.name
      : QString("%1 (%2)").arg(summary.name, summary.path);
  return title + ": " + parts.join("; ");
}

void FrameStats::logSummaries()
{
  for (const Summary& summary : summaries())
    if (!summary.isEmpty())
      qCInfo(lcFrameStats).noquote() << summaryLine(summary);
}

void FrameStats::startLogging()
{
  QTimer* timer = new QTimer(QCoreApplication::instance());
  QObject::connect(timer, &QTimer::timeout, &FrameStats::logSummaries);
  timer->start(LOG_INTERVAL_MS);
}

}
