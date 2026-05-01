#!/bin/bash
# Install build dependencies for MapMap on macOS using Homebrew.
#
# Qt5 is installed via Homebrew. GStreamer is installed via the official
# GStreamer macOS packages (pkg-config .pc files) so that qmake's
# CONFIG += link_pkgconfig / PKGCONFIG += gstreamer-1.0 ... works.
#
# Usage:
#   chmod +x sh_install_deps_macos.sh
#   ./sh_install_deps_macos.sh

set -euo pipefail

# ---------------------------------------------------------------------------
# Homebrew
# ---------------------------------------------------------------------------
if ! command -v brew &>/dev/null; then
    echo "Homebrew not found. Installing..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

# ---------------------------------------------------------------------------
# pkg-config (needed so qmake can locate GStreamer via PKGCONFIG)
# ---------------------------------------------------------------------------
brew install pkg-config

# ---------------------------------------------------------------------------
# Qt 5
# Modules used: core gui widgets opengl xml network multimedia
# ---------------------------------------------------------------------------
brew install qt@5

# Make Qt 5 tools (qmake, lrelease, …) available on PATH for this session.
QT5_PREFIX="$(brew --prefix qt@5)"
export PATH="${QT5_PREFIX}/bin:${PATH}"
echo "Qt 5 prefix: ${QT5_PREFIX}"
echo "qmake: $(command -v qmake)"
qmake --version

# ---------------------------------------------------------------------------
# GStreamer 1.0
# Required pkg-config packages:
#   gstreamer-1.0  gstreamer-base-1.0  gstreamer-app-1.0  gstreamer-pbutils-1.0
# ---------------------------------------------------------------------------
brew install \
    gstreamer \
    gst-plugins-base \
    gst-plugins-good \
    gst-plugins-bad \
    gst-plugins-ugly \
    gst-libav

# Expose GStreamer .pc files to pkg-config.
GST_PREFIX="$(brew --prefix gstreamer)"
export PKG_CONFIG_PATH="${GST_PREFIX}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
echo "GStreamer prefix: ${GST_PREFIX}"
echo "GStreamer version: $(pkg-config --modversion gstreamer-1.0)"

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
cat <<EOF

All dependencies installed successfully.

To build MapMap, run:

    ./scripts/sh_build_macos.sh

EOF
