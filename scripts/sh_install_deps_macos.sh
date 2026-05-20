#!/bin/bash
# Install build dependencies for MapMap on macOS using Homebrew.
#
# Qt 6 is used as the media backend (QMediaPlayer, QVideoSink, QCamera).
# GStreamer is not required.
#
# Usage:
#   chmod +x scripts/sh_install_deps_macos.sh
#   ./scripts/sh_install_deps_macos.sh

set -euo pipefail

# ---------------------------------------------------------------------------
# Homebrew
# ---------------------------------------------------------------------------
if ! command -v brew &>/dev/null; then
    echo "Homebrew not found. Installing..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

# ---------------------------------------------------------------------------
# Qt 6
# Modules used: core gui widgets opengl openglwidgets xml network multimedia
#               multimediawidgets
# ---------------------------------------------------------------------------
brew install qt

# Make Qt 6 tools (qmake, lrelease, …) available on PATH for this session.
QT_PREFIX="$(brew --prefix qt)"
export PATH="${QT_PREFIX}/bin:${PATH}"
echo "Qt prefix:  ${QT_PREFIX}"
echo "qmake:      $(command -v qmake)"
qmake --version

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
cat <<EOF

All dependencies installed successfully.

To build MapMap, run:

    ./scripts/sh_build_macos.sh

EOF
