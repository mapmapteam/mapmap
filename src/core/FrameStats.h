/*
 * FrameStats.h
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

#ifndef FRAME_STATS_H_
#define FRAME_STATS_H_

#include <QLoggingCategory>
#include <QMutex>
#include <QList>
#include <QString>
#include <array>
#include <atomic>

Q_DECLARE_LOGGING_CATEGORY(lcFrameStats)

namespace mmp {

/**
 * Times each step of the path of an image, from the decoded video frame to
 * the screen, for one video source or one view.
 *
 * Off by default (see setEnabled()): callers test isEnabled() before reading
 * the clock, so the measure costs a single test of a boolean when it is off.
 *
 * Each step keeps its latest samples in a ring buffer of fixed size: no
 * allocation per frame. Recording and reading are safe from any thread (the
 * preparation of a video frame is timed on the decoder's thread).
 *
 * Every live instance is listed by summaries(), which gives the mean and the
 * maximum of each step over the last second. logSummaries() writes them to
 * the log, under the "mapmap.framestats" category.
 */
class FrameStats
{
public:
  /// The steps of the image path.
  enum Stage {
    /// Video frame work on the decoder's thread (copy, or conversion to RGB).
    PREPARE,
    /// Copy of a new frame to the graphics card.
    UPLOAD,
    /// Painting of a view, from start to end.
    DRAW,
    /// Time between two paintings of a view.
    INTERVAL,
    STAGE_COUNT
  };

  /// Number of samples kept for each step: more than one second of frames,
  /// even on a 240 Hz screen.
  static constexpr int SAMPLE_CAPACITY = 256;

  /// Length of the window of the summaries, in nanoseconds.
  static constexpr qint64 WINDOW_NS = 1000000000;

  /// Mean and maximum of one step over the window.
  struct StageSummary {
    int count = 0;
    qint64 meanNs = 0;
    qint64 maxNs = 0;
  };

  /// Summary of one source or view over the window.
  struct Summary {
    QString name;
    /// How the frames are handled, eg. "YUV on the GPU, NV12" (may be empty).
    QString path;
    StageSummary stages[STAGE_COUNT];
    /// Decoded frames replaced by a newer one before they were uploaded.
    int dropped = 0;

    /// Returns true if nothing was recorded during the window.
    bool isEmpty() const;
  };

  /// Creates a measure named after a source or a view, listed by summaries().
  explicit FrameStats(const QString& name = QString());
  ~FrameStats();

  FrameStats(const FrameStats&) = delete;
  FrameStats& operator=(const FrameStats&) = delete;

  /// Turns the measure on or off for every source and view.
  static void setEnabled(bool enabled);
  /// Returns true if the measure is on.
  static bool isEnabled() { return _enabled.load(std::memory_order_relaxed); }

  /// Turns the synchronous measure on or off: callers wait for the graphics
  /// card to finish (glFinish()) before stopping the clock, so each step gets
  /// its real cost. It slows everything down.
  static void setSynchronous(bool synchronous);
  /// Returns true if the synchronous measure is on.
  static bool isSynchronous() { return _synchronous.load(std::memory_order_relaxed); }

  /// Returns the time of a monotonic clock, in nanoseconds.
  static qint64 now();

  /// Sets the name shown in the summaries.
  void setName(const QString& name);
  /// Returns the name shown in the summaries.
  QString name() const;

  /// Sets how the frames are handled. Call it only when it changes: it may
  /// allocate.
  void setPath(const QString& path);

  /// Records that a step lasted durationNs, at time atNs.
  void record(Stage stage, qint64 durationNs, qint64 atNs = now());

  /// Records a decoded frame replaced by a newer one before its upload.
  void recordDropped(qint64 atNs = now());

  /// Returns the summary of the window that ends at atNs.
  Summary summary(qint64 atNs = now()) const;

  /// Returns the summaries of every live instance, in order of creation.
  static QList<Summary> summaries(qint64 atNs = now());

  /// Returns a summary as one line of text, leaving out the empty steps.
  static QString summaryLine(const Summary& summary);

  /// Writes one line per non-empty summary to the log.
  static void logSummaries();

  /// Calls logSummaries() once per second, from the event loop of the
  /// application.
  static void startLogging();

private:
  struct Sample {
    qint64 atNs;
    qint64 durationNs;
  };

  /// Latest samples of one step, the oldest overwritten first.
  struct Ring {
    std::array<Sample, SAMPLE_CAPACITY> samples;
    int next = 0;
    int size = 0;

    void add(qint64 atNs, qint64 durationNs);
    StageSummary summary(qint64 fromNs, qint64 toNs) const;
  };

  mutable QMutex _mutex;
  QString _name;
  QString _path;
  Ring _stages[STAGE_COUNT];
  Ring _dropped;

  static std::atomic<bool> _enabled;
  static std::atomic<bool> _synchronous;
};

}

#endif /* FRAME_STATS_H_ */
