#!/bin/bash
# Package MapMap for macOS: a disk image (.dmg) that holds MapMap.app with the
# Qt libraries it needs, so it runs on a Mac where Qt is not installed.
#
# Run scripts/sh_install_deps_macos.sh first if Qt 6 is not yet installed.
#
# The app gets an ad-hoc signature: it runs, but macOS warns that the developer
# cannot be verified. For a public release, use scripts/sh_release_macos.sh.
#
# Usage:
#   ./scripts/sh_package_macos.sh [options]
#
# Options:
#   --skip-build               Package the MapMap.app already built at the root
#                              of the repository (it must be a release build).
#                              Without this option, the script builds the
#                              last commit in a clean copy of the sources.
#   --output-directory <dir>   Where to write the disk image (default: dist).
#   --qt-directory <dir>       Build and package with this Qt, for example
#                              ~/Qt/6.11.3/macos. Without it, the Qt whose qmake
#                              is found first is used, then that of Homebrew.
#   --help                     Show this help.
#
# Use the Qt of the Qt Company (see https://www.qt.io or aqtinstall) for a disk
# image that goes on other computers: its libraries run on older versions of
# macOS, and on both Apple Silicon and Intel. Those of Homebrew only run on the
# version of macOS and the type of processor of the machine they were installed
# on.

set -euo pipefail

# Change to the project root regardless of where the script is called from.
cd "$(dirname "$0")/.."
PROJECT_ROOT="$(pwd)"

APP_NAME="MapMap"
BUILD_DIRECTORY="${PROJECT_ROOT}/build/package-macos"
# Signing identity "-" asks codesign for an ad-hoc signature.
AD_HOC_IDENTITY="-"
# Where a library copied into the bundle is loaded from at run time.
BUNDLED_LIBRARY_PREFIX="@executable_path/../Frameworks"
# Libraries loaded from these folders belong to the build machine (Homebrew).
# The folder of Qt is added once it is known.
BUILD_MACHINE_LIBRARY_PATTERN="^(/opt/homebrew|/usr/local)/"
# Each pass can copy libraries that need others in turn; a handful is plenty.
MAXIMUM_RELOCATION_PASSES=10
# Qt plugin that reads SVG images, and the Qt library it needs.
SVG_IMAGE_PLUGIN="imageformats/libqsvg.dylib"
SVG_FRAMEWORK="QtSvg"
# hdiutil sometimes fails with "Resource busy", when the system still uses the
# temporary disk it works with: how many times to try, and how long to wait.
DISK_IMAGE_MAXIMUM_ATTEMPTS=5
DISK_IMAGE_RETRY_DELAY_SECONDS=10

skip_build=false
output_directory="${PROJECT_ROOT}/dist"
qt_directory=""

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
        --qt-directory)
            [ $# -ge 2 ] || { echo "Error: --qt-directory needs a value." >&2; exit 2; }
            qt_directory="$2"
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
if [ -n "${qt_directory}" ]; then
    [ -x "${qt_directory}/bin/qmake" ] || {
        echo "Error: there is no bin/qmake in ${qt_directory}." >&2
        exit 1
    }
    export PATH="${qt_directory}/bin:${PATH}"
elif ! command -v qmake >/dev/null 2>&1 && command -v brew >/dev/null 2>&1; then
    export PATH="$(brew --prefix qt)/bin:${PATH}"
fi

for tool in qmake macdeployqt codesign hdiutil lipo; do
    command -v "${tool}" >/dev/null 2>&1 || {
        echo "Error: ${tool} was not found. Run scripts/sh_install_deps_macos.sh first." >&2
        exit 1
    }
done

display_version="$(read_project_variable MAPMAP_VERSION)"
numeric_version="$(read_project_variable VERSION)"
[ -n "${display_version}" ] && [ -n "${numeric_version}" ] || {
    echo "Error: could not read VERSION and MAPMAP_VERSION from mapmap.pro." >&2
    exit 1
}
qt_prefix="$(qmake -query QT_INSTALL_PREFIX)"
qt_libraries="$(qmake -query QT_INSTALL_LIBS)"
BUILD_MACHINE_LIBRARY_PATTERN="^(/opt/homebrew|/usr/local|${qt_prefix})/"
# Types of processor that Qt was built for, and so that MapMap can be built for.
architectures="$(lipo -archs "${qt_libraries}/QtCore.framework/Versions/A/QtCore")"

echo "Packaging ${APP_NAME} ${display_version} for macOS (${architectures})"
echo "Qt $(qmake -query QT_VERSION) in ${qt_prefix}"

# ---------------------------------------------------------------------------
# Build (release, from a clean copy of the committed files)
# ---------------------------------------------------------------------------
if [ "${skip_build}" = true ]; then
    built_app="${PROJECT_ROOT}/${APP_NAME}.app"
else
    source_directory="${BUILD_DIRECTORY}/source"
    export_committed_sources "${source_directory}"
    (
        cd "${source_directory}"
        qmake mapmap.pro CONFIG+=release "QMAKE_APPLE_DEVICE_ARCHS=${architectures}"
        make -j"$(sysctl -n hw.logicalcpu)"
    )
    built_app="${source_directory}/${APP_NAME}.app"
fi

[ -d "${built_app}" ] || {
    echo "Error: ${built_app} does not exist." >&2
    exit 1
}

# ---------------------------------------------------------------------------
# Stage a copy of the app and add the Qt libraries and plugins to it
# ---------------------------------------------------------------------------
stage_directory="${BUILD_DIRECTORY}/stage"
staged_app="${stage_directory}/${APP_NAME}.app"
rm -rf "${stage_directory}"
mkdir -p "${stage_directory}"
cp -R "${built_app}" "${staged_app}"
frameworks_directory="${staged_app}/Contents/Frameworks"

# List the programs and libraries inside the staged app. The library of a
# framework has no telling name or permission: ask what each file is.
list_binaries() {
    find "${staged_app}/Contents" -type f | while IFS= read -r candidate; do
        if file -b "${candidate}" | grep -q 'Mach-O'; then
            echo "${candidate}"
        fi
    done
}

# Print where, inside the bundle, a library of the build machine is loaded
# from at run time. A framework keeps its inner path
# (QtGui.framework/Versions/A/QtGui); a plain library keeps only its name.
bundled_reference_for() {
    case "$1" in
        *.framework/*)
            echo "${BUNDLED_LIBRARY_PREFIX}/$(echo "$1" | sed 's#^.*/\([^/]*\.framework/\)#\1#')"
            ;;
        *)
            echo "${BUNDLED_LIBRARY_PREFIX}/$(basename "$1")"
            ;;
    esac
}

# List the libraries that a program or library of the bundle loads from the
# build machine (Homebrew folders) instead of from the bundle. Its own
# identity, which otool prints first for a library, is not a dependency.
list_outside_references() {
    local own_identity
    own_identity="$(otool -D "$1" 2>/dev/null | sed -n '2p')"
    otool -L "$1" 2>/dev/null | awk 'NR > 1 { print $1 }' \
        | grep -E "${BUILD_MACHINE_LIBRARY_PATTERN}" \
        | grep -v -x -F "${own_identity:-none}" || true
}

macdeployqt "${staged_app}" -verbose=1

# MapMap draws its test cards and its logo from SVG files, through the SVG
# image plugin of Qt. With Qt from Homebrew, macdeployqt copies neither that
# plugin nor the QtSvg library it needs: add both.
staged_svg_plugin="${staged_app}/Contents/PlugIns/${SVG_IMAGE_PLUGIN}"
if [ ! -e "${staged_svg_plugin}" ]; then
    cp "$(qmake -query QT_INSTALL_PLUGINS)/${SVG_IMAGE_PLUGIN}" "${staged_svg_plugin}"
    chmod u+w "${staged_svg_plugin}"
fi
if [ ! -d "${frameworks_directory}/${SVG_FRAMEWORK}.framework" ]; then
    # Homebrew reaches the framework through a symbolic link: copy the real
    # folder, so the links inside the framework are kept as they are.
    svg_framework_source="$(cd "$(qmake -query QT_INSTALL_LIBS)/${SVG_FRAMEWORK}.framework" && pwd -P)"
    cp -R "${svg_framework_source}" "${frameworks_directory}/"
    chmod -R u+w "${frameworks_directory}/${SVG_FRAMEWORK}.framework"
    # Header files are of no use at run time.
    rm -rf "${frameworks_directory}/${SVG_FRAMEWORK}.framework/Headers" \
           "${frameworks_directory}/${SVG_FRAMEWORK}.framework/Versions/A/Headers"
    install_name_tool \
        -id "${BUNDLED_LIBRARY_PREFIX}/${SVG_FRAMEWORK}.framework/Versions/A/${SVG_FRAMEWORK}" \
        "${frameworks_directory}/${SVG_FRAMEWORK}.framework/Versions/A/${SVG_FRAMEWORK}"
fi

# The build leaves the libraries folder of the build machine in the search
# paths of the program. Remove it: only the bundle may be searched.
while IFS= read -r binary; do
    otool -l "${binary}" 2>/dev/null \
        | awk '$1 == "cmd" { is_rpath = ($2 == "LC_RPATH") } is_rpath && $1 == "path" { print $2 }' \
        | { grep -E "${BUILD_MACHINE_LIBRARY_PATTERN}" || true; } \
        | while IFS= read -r search_path; do
            install_name_tool -delete_rpath "${search_path}" "${binary}"
        done
done < <(list_binaries)

# A copied library can still name itself by its path on the build machine.
# Nothing loads it by that name, but give it the name it has in the bundle.
while IFS= read -r binary; do
    own_identity="$(otool -D "${binary}" 2>/dev/null | sed -n '2p')"
    if echo "${own_identity}" | grep -q -E "${BUILD_MACHINE_LIBRARY_PATTERN}"; then
        install_name_tool -id "$(bundled_reference_for "${own_identity}")" "${binary}"
    fi
done < <(list_binaries)

# Some programs and libraries of the bundle still load libraries from the
# build machine. Make every such reference point inside the bundle, copying a
# plain library in when it is not there yet. A copied library can need others
# in turn, hence the passes.
pass=0
while [ "${pass}" -lt "${MAXIMUM_RELOCATION_PASSES}" ]; do
    pass=$((pass + 1))
    relocated_something=false
    while IFS= read -r binary; do
        while IFS= read -r reference; do
            bundled_reference="$(bundled_reference_for "${reference}")"
            bundled_file="${frameworks_directory}/${bundled_reference#"${BUNDLED_LIBRARY_PREFIX}/"}"
            if [ ! -e "${bundled_file}" ]; then
                case "${reference}" in
                    *.framework/*)
                        echo "Error: ${binary} needs ${reference}," >&2
                        echo "a framework that is not in the bundle." >&2
                        exit 1
                        ;;
                esac
                cp "${reference}" "${bundled_file}"
                chmod u+w "${bundled_file}"
                install_name_tool -id "${bundled_reference}" "${bundled_file}"
            fi
            install_name_tool -change "${reference}" "${bundled_reference}" "${binary}"
            relocated_something=true
        done < <(list_outside_references "${binary}")
    done < <(list_binaries)
    [ "${relocated_something}" = true ] || break
done

# Stamp the version of this release in the bundle: the one the Finder shows
# (1.0.0-beta.1), and the internal one, which stays numeric (1.0.0).
info_plist="${staged_app}/Contents/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString ${display_version}" "${info_plist}"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion ${numeric_version}" "${info_plist}"

# ---------------------------------------------------------------------------
# Sign: required on Apple Silicon, where unsigned code does not run
# ---------------------------------------------------------------------------
codesign --force --deep --sign "${AD_HOC_IDENTITY}" "${staged_app}"
codesign --verify --deep --strict "${staged_app}"

# Nothing in the bundle may still point to the libraries of the build machine.
while IFS= read -r binary; do
    remaining_references="$(list_outside_references "${binary}")"
    if [ -n "${remaining_references}" ]; then
        echo "Error: ${binary} still loads libraries from outside the bundle:" >&2
        echo "${remaining_references}" >&2
        exit 1
    fi
done < <(list_binaries)

# ---------------------------------------------------------------------------
# Create the disk image
# ---------------------------------------------------------------------------
ln -s /Applications "${stage_directory}/Applications"
cp "${PROJECT_ROOT}/LICENSE" "${stage_directory}/LICENSE.txt"
cp "${PROJECT_ROOT}/README.md" "${stage_directory}/README.md"

mkdir -p "${output_directory}"
case "$(lipo -archs "${staged_app}/Contents/MacOS/${APP_NAME}")" in
    *\ *) architecture_name="universal" ;;
    *) architecture_name="$(lipo -archs "${staged_app}/Contents/MacOS/${APP_NAME}")" ;;
esac
disk_image="${output_directory}/${APP_NAME}-${display_version}-macos-${architecture_name}.dmg"
attempt=1
until hdiutil create \
    -volname "${APP_NAME} ${display_version}" \
    -srcfolder "${stage_directory}" \
    -ov -format UDZO \
    "${disk_image}"
do
    if [ "${attempt}" -ge "${DISK_IMAGE_MAXIMUM_ATTEMPTS}" ]; then
        echo "Error: hdiutil failed ${attempt} times; no disk image was made." >&2
        exit 1
    fi
    echo "hdiutil failed (attempt ${attempt}); trying again in ${DISK_IMAGE_RETRY_DELAY_SECONDS} seconds." >&2
    sleep "${DISK_IMAGE_RETRY_DELAY_SECONDS}"
    attempt=$((attempt + 1))
done

echo
echo "Disk image: ${disk_image}"
shasum -a 256 "${disk_image}"
echo "Ad-hoc signature only: macOS will warn that the developer cannot be verified."
echo "For a public release, use scripts/sh_release_macos.sh."
