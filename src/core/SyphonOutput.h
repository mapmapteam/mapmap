/*
 * SyphonOutput.h
 *
 * (c) 2026 Alexandre Quessy -- alexandre(@)quessy(.)net
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

#ifndef SYPHON_OUTPUT_H_
#define SYPHON_OUTPUT_H_

#include <QtGlobal>

// Syphon is a macOS-only inter-application video-sharing framework.
#ifdef HAVE_SYPHON

#include <QString>

namespace mmp {

// Objective-C++ implementation (SyphonOpenGLServer wrapper), defined in
// SyphonServerImpl.mm. Kept opaque so this header stays pure C++.
class SyphonServerImpl;

/**
 * Publishes MapMap's rendered output composition as a Syphon server, so other
 * macOS applications can receive it (a "virtual projector"). Opt-in: nothing is
 * published until setEnabled(true).
 *
 * publishCurrentFramebuffer() must be called from the output canvas's GL
 * context while it is current (e.g. inside QPainter::beginNativePainting()),
 * right after the clean composition has been rendered.
 */
class SyphonOutput
{
public:
  SyphonOutput();
  ~SyphonOutput();

  void setEnabled(bool on);
  bool isEnabled() const { return _enabled; }

  /// Human-readable server name shown to Syphon clients (default "MapMap").
  void setServerName(const QString& name);
  QString serverName() const { return _serverName; }

  /**
   * Publishes the contents of the currently-bound framebuffer (queried from GL,
   * along with the viewport size). Must be called from the output canvas's GL
   * context while current — inside QPainter::beginNativePainting() — right after
   * the composition is rendered. No-op unless enabled. Safe to call every frame.
   */
  void publishCurrentFramebuffer();

private:
  bool _enabled;
  QString _serverName;
  SyphonServerImpl* _impl;
};

}

#endif // HAVE_SYPHON

#endif /* SYPHON_OUTPUT_H_ */
