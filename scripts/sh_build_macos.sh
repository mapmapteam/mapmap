#!/bin/bash
# Build MapMap on macOS using Homebrew Qt 6.
#
# Run scripts/sh_install_deps_macos.sh first if dependencies are not yet installed.
#
# Usage:
#   ./scripts/sh_build_macos.sh

set -euo pipefail

# Change to the project root regardless of where the script is called from.
cd "$(dirname "$0")/.."

# ---------------------------------------------------------------------------
# Locate Homebrew Qt 6
# ---------------------------------------------------------------------------
QT_PREFIX="$(brew --prefix qt)"

export PATH="${QT_PREFIX}/bin:${PATH}"

echo "qmake:      $(command -v qmake)"
echo "Qt version: $(qmake --version | tail -1)"

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
qmake mapmap.pro
make -j"$(sysctl -n hw.logicalcpu)"
