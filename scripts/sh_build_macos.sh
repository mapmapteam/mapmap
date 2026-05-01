#!/bin/bash
# Build MapMap on macOS using Homebrew Qt 5 and GStreamer.
#
# Run sh_install_deps_macos.sh first if dependencies are not yet installed.
#
# Usage:
#   ./scripts/sh_build_macos.sh

set -euo pipefail

# Change to the project root regardless of where the script is called from.
cd "$(dirname "$0")/.."

# ---------------------------------------------------------------------------
# Locate Homebrew Qt 5 and GStreamer
# ---------------------------------------------------------------------------
QT5_PREFIX="$(brew --prefix qt@5)"
GST_PREFIX="$(brew --prefix gstreamer)"

export PATH="${QT5_PREFIX}/bin:${PATH}"
export PKG_CONFIG_PATH="${GST_PREFIX}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

echo "qmake:      $(command -v qmake)"
echo "Qt version: $(qmake --version | tail -1)"
echo "GStreamer:  $(pkg-config --modversion gstreamer-1.0)"

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
qmake mapmap.pro
make -j"$(sysctl -n hw.logicalcpu)"
