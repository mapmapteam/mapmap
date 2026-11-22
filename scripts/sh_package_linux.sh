#!/bin/bash
# Package MapMap for GNU/Linux: a .tar.gz archive that holds the program, its
# icon, the license and a note on how to run it.
#
# The archive does not contain the Qt libraries: it runs on the distribution
# and version it was built on, once the Qt 6 packages listed in INSTALL.md are
# installed. The note inside the archive says which system that is.
#
# Usage:
#   ./scripts/sh_package_linux.sh [options]
#
# Options:
#   --skip-build               Package the mapmap program already built at the
#                              root of the repository (it must be a release
#                              build). Without this option, the script builds
#                              the last commit in a clean copy of the sources.
#   --output-directory <dir>   Where to write the archive (default: dist).
#   --help                     Show this help.

set -euo pipefail

# Change to the project root regardless of where the script is called from.
cd "$(dirname "$0")/.."
PROJECT_ROOT="$(pwd)"

PROGRAM_NAME="mapmap"
BUILD_DIRECTORY="${PROJECT_ROOT}/build/package-linux"

skip_build=false
output_directory="${PROJECT_ROOT}/dist"

show_help() {
    # Print the comment block at the top of this file, without the "# " prefix.
    sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'
}

while [ $# -gt 0 ]; do
    case "$1" in
        --skip-build)
            skip_build=true
            ;;
        --output-directory)
            [ $# -ge 2 ] || { echo "Error: --output-directory needs a value." >&2; exit 2; }
            output_directory="$2"
            shift
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            echo "Error: unknown option: $1" >&2
            show_help >&2
            exit 2
            ;;
    esac
    shift
done

# Read a "NAME = value" assignment from mapmap.pro.
read_project_variable() {
    sed -n "s/^$1[[:space:]]*=[[:space:]]*//p" "${PROJECT_ROOT}/mapmap.pro" | head -1
}

# Name of the distribution this archive is built on, e.g. "Ubuntu 24.04.1 LTS".
read_distribution_name() {
    if [ -r /etc/os-release ]; then
        sed -n 's/^PRETTY_NAME="\{0,1\}\([^"]*\)"\{0,1\}$/\1/p' /etc/os-release
    else
        uname -sr
    fi
}

# Copy the committed files to a clean directory. MapMap does not build outside
# of its source tree, and building a copy leaves a developer build alone.
export_committed_sources() {
    local destination="$1"
    rm -rf "${destination}"
    mkdir -p "${destination}"
    echo "Building commit $(git -C "${PROJECT_ROOT}" rev-parse --short HEAD)"
    if [ -n "$(git -C "${PROJECT_ROOT}" status --porcelain)" ]; then
        echo "Warning: uncommitted changes are not part of this build." >&2
    fi
    git -C "${PROJECT_ROOT}" archive HEAD | tar -x -C "${destination}"
}

# ---------------------------------------------------------------------------
# Locate the tools
# ---------------------------------------------------------------------------
if command -v qmake6 >/dev/null 2>&1; then
    qmake_command="qmake6"
elif command -v qmake >/dev/null 2>&1; then
    qmake_command="qmake"
else
    echo "Error: qmake6 was not found. Install the packages listed in INSTALL.md." >&2
    exit 1
fi

display_version="$(read_project_variable MAPMAP_VERSION)"
[ -n "${display_version}" ] || {
    echo "Error: could not read MAPMAP_VERSION from mapmap.pro." >&2
    exit 1
}
architecture="$(uname -m)"
package_name="${PROGRAM_NAME}-${display_version}-linux-${architecture}"

echo "Packaging MapMap ${display_version} for GNU/Linux (${architecture})"

# ---------------------------------------------------------------------------
# Build (release, from a clean copy of the committed files)
# ---------------------------------------------------------------------------
if [ "${skip_build}" = true ]; then
    built_program="${PROJECT_ROOT}/${PROGRAM_NAME}"
else
    source_directory="${BUILD_DIRECTORY}/source"
    export_committed_sources "${source_directory}"
    (
        cd "${source_directory}"
        "${qmake_command}" mapmap.pro CONFIG+=release
        make -j"$(nproc)"
    )
    built_program="${source_directory}/${PROGRAM_NAME}"
fi

[ -x "${built_program}" ] || {
    echo "Error: ${built_program} does not exist or cannot be run." >&2
    exit 1
}

# ---------------------------------------------------------------------------
# Stage the files of the archive
# ---------------------------------------------------------------------------
stage_directory="${BUILD_DIRECTORY}/stage"
package_directory="${stage_directory}/${package_name}"
rm -rf "${stage_directory}"
mkdir -p "${package_directory}"

cp "${built_program}" "${package_directory}/${PROGRAM_NAME}"
cp "${PROJECT_ROOT}/resources/app_icons/mapmap.png" "${package_directory}/mapmap.png"
cp "${PROJECT_ROOT}/LICENSE" "${package_directory}/LICENSE"
cp "${PROJECT_ROOT}/README.md" "${package_directory}/README.md"
cp "${PROJECT_ROOT}/CHANGELOG.md" "${package_directory}/CHANGELOG.md"

{
    echo "MapMap ${display_version} for GNU/Linux (${architecture})"
    echo
    echo "Built on: $(read_distribution_name)"
    echo
    echo "This archive holds the program only, not the Qt libraries. It runs on the"
    echo "system named above, once Qt 6 is installed. On Ubuntu:"
    echo
    echo "    sudo apt-get install -y qt6-base-dev qt6-multimedia-dev \\"
    echo "        libqt6opengl6-dev libqt6openglwidgets6"
    echo
    echo "The test cards of MapMap are SVG images: they also need the SVG image"
    echo "plugin of Qt 6, which comes with the Qt 6 SVG package of the distribution."
    echo
    echo "Then start MapMap from this folder:"
    echo
    echo "    ./mapmap"
    echo
    echo "On another distribution or version, build MapMap from its source code:"
    echo "see INSTALL.md at https://github.com/mapmapteam/mapmap"
    echo
    echo "Qt libraries this program was linked with:"
    echo
    ldd "${built_program}" | sed -n 's/^[[:space:]]*\(libQt6[^ ]*\).*/    \1/p' | sort
} > "${package_directory}/INSTALL.txt"

# ---------------------------------------------------------------------------
# Create the archive
# ---------------------------------------------------------------------------
mkdir -p "${output_directory}"
archive="${output_directory}/${package_name}.tar.gz"
tar -czf "${archive}" -C "${stage_directory}" "${package_name}"

echo
echo "Archive: ${archive}"
sha256sum "${archive}"
