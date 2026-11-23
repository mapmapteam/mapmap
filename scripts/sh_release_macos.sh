#!/bin/bash
# Make the macOS disk image of a public release of MapMap, signed with a
# "Developer ID Application" certificate. For a quick disk image to try, use
# scripts/sh_package_macos.sh: this script stops at the first doubt.
#
# Nothing is sent to Apple unless --submit-to-apple is given, and nothing is
# ever published. Signing asks Apple's time server for the date; it receives a
# fingerprint of the files, not the files.
#
# A release is made in two steps, with a manual test of the disk image between:
#   ./scripts/sh_release_macos.sh --qt-directory <dir>
#   ./scripts/sh_release_macos.sh --submit-to-apple <disk image> --notary-profile <name>
#
# Options to make the disk image:
#   --qt-directory <dir>       Qt of the Qt Company to build with, for example
#                              ~/Qt/6.11.3/macos. Required.
#   --sign-identity <name>     "Developer ID Application" identity to sign with.
#                              Needed only when the keychain holds several.
#   --output-directory <dir>   Where to write the disk image (default: dist).
#   --rehearsal                Try the script without the certificate: sign ad
#                              hoc, without Apple's time server, and report the
#                              problems of the release as warnings. The result
#                              is not for distribution and may not start.
#
# Options to have the disk image notarized by Apple:
#   --submit-to-apple <file>   Send this disk image, made by this script, to
#                              Apple for notarization, after a confirmation
#                              typed on the keyboard, then attach Apple's
#                              approval to it.
#   --notary-profile <name>    Keychain profile made with
#                              "xcrun notarytool store-credentials <name>".
#
#   --help                     Show this help.

set -euo pipefail

# Keep relative paths given by the user meaningful, then change to the project
# root regardless of where the script is called from.
CALLER_DIRECTORY="$(pwd)"
cd "$(dirname "$0")/.."
PROJECT_ROOT="$(pwd)"

APP_NAME="MapMap"
IDENTITY_PREFIX="Developer ID Application:"
AD_HOC_IDENTITY="-"
ENTITLEMENTS="${PROJECT_ROOT}/resources/macOS/MapMap.entitlements"
BUILD_DIRECTORY="${PROJECT_ROOT}/build/release-macos"
# Where scripts/sh_package_macos.sh leaves the app and the files that go in the
# disk image with it.
PACKAGE_STAGE_DIRECTORY="${PROJECT_ROOT}/build/package-macos/stage"
# A release runs on both Intel and Apple Silicon.
RELEASE_ARCHITECTURES="x86_64 arm64"
# Entitlement of development builds, which notarization refuses.
DEBUG_ENTITLEMENT="com.apple.security.get-task-allow"
# hdiutil sometimes fails with "Resource busy": how many times to try, and how
# long to wait.
DISK_IMAGE_MAXIMUM_ATTEMPTS=5
DISK_IMAGE_RETRY_DELAY_SECONDS=10

mount_point=""
notary_profile=""
output_directory="${PROJECT_ROOT}/dist"
problem_count=0
qt_directory=""
rehearsal=false
sign_identity=""
submitted_image=""
team_identifier=""

show_help() {
    # Print the comment block at the top of this file, without the "# " prefix.
    sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'
}

# Stop with an error message.
fail() {
    echo "Error: $1" >&2
    exit 1
}

# Report a problem that forbids a release. A rehearsal goes on.
report_problem() {
    if [ "${rehearsal}" = true ]; then
        echo "Warning (would stop a release): $1" >&2
        problem_count=$((problem_count + 1))
    else
        fail "$1"
    fi
}

# Detach the disk image that is being checked, whatever happens.
clean_up() {
    if [ -n "${mount_point}" ] && [ -d "${mount_point}" ]; then
        hdiutil detach "${mount_point}" -quiet || true
        rmdir "${mount_point}" 2>/dev/null || true
    fi
}
trap clean_up EXIT

# Print a path given on the command line as an absolute path.
absolute_path() {
    case "$1" in
        /*) echo "$1" ;;
        *) echo "${CALLER_DIRECTORY}/$1" ;;
    esac
}

print_step() {
    echo
    echo "==> $1"
}

require_tools() {
    local tool
    for tool in "$@"; do
        command -v "${tool}" >/dev/null 2>&1 || fail "${tool} was not found."
    done
}

# Read a "NAME = value" assignment from mapmap.pro.
read_project_variable() {
    sed -n "s/^$1[[:space:]]*=[[:space:]]*//p" "${PROJECT_ROOT}/mapmap.pro" | head -1
}

# In this script, "grep -q" reads a variable and never a pipe: it stops reading
# at the first match, which makes the command before it fail (pipefail).

# Print what codesign knows about a signed file. codesign writes to stderr.
signature_details() {
    codesign --display --verbose=4 "$1" 2>&1
}

# Succeed when version $1 is newer than version $2 (numbers and dots only).
version_is_newer() {
    awk -v first="$1" -v second="$2" 'BEGIN {
        first_count = split(first, first_parts, ".")
        second_count = split(second, second_parts, ".")
        count = first_count > second_count ? first_count : second_count
        for (i = 1; i <= count; i++) {
            if (first_parts[i] + 0 > second_parts[i] + 0) exit 0
            if (first_parts[i] + 0 < second_parts[i] + 0) exit 1
        }
        exit 1
    }'
}

# ---------------------------------------------------------------------------
# Checks made before building
# ---------------------------------------------------------------------------

check_sources() {
    local released_version
    [ -z "$(git status --porcelain)" ] \
        || report_problem "there are uncommitted changes."

    # The version at the top of the release notes is the one being released.
    released_version="$(sed -n 's/^## \([^ ]*\) - .*$/\1/p' CHANGELOG.md | head -1)"
    [ "${released_version}" = "${display_version}" ] \
        || report_problem "mapmap.pro says ${display_version}, the top of CHANGELOG.md says ${released_version:-nothing}."
}

# Choose the identity to sign with, and read its team from the certificate.
find_identity() {
    local identities identity_count
    if [ "${rehearsal}" = true ]; then
        sign_identity="${AD_HOC_IDENTITY}"
        return
    fi
    # find-identity -v only lists certificates that are valid and have a key.
    identities="$(security find-identity -v -p codesigning \
        | sed -n "s/^.*\"\(${IDENTITY_PREFIX} .*\)\"$/\1/p" | sort -u)"
    if [ -z "${sign_identity}" ]; then
        identity_count="$(echo "${identities}" | grep -c . || true)"
        [ "${identity_count}" -ne 0 ] \
            || fail "no valid \"${IDENTITY_PREFIX}\" certificate in the keychain."
        [ "${identity_count}" -eq 1 ] \
            || fail "several certificates, choose one with --sign-identity:
${identities}"
        sign_identity="${identities}"
    else
        grep -q -x -F "${sign_identity}" <<< "${identities}" \
            || fail "\"${sign_identity}\" is not a valid certificate of the keychain."
    fi
    team_identifier="$(security find-certificate -c "${sign_identity}" -p \
        | openssl x509 -noout -subject \
        | sed -n 's/^.*OU *= *\([A-Z0-9]*\).*$/\1/p')"
    [ -n "${team_identifier}" ] || fail "could not read the team of ${sign_identity}."
}

# ---------------------------------------------------------------------------
# Signature
# ---------------------------------------------------------------------------

# List the programs and libraries inside the app.
list_binaries() {
    find "${app}/Contents" -type f | while IFS= read -r candidate; do
        case "$(file -b "${candidate}")" in
            *Mach-O*) echo "${candidate}" ;;
        esac
    done
}

# Sign one file or bundle. The hardened runtime and a secure timestamp are
# required for notarization. More arguments are passed to codesign.
sign() {
    local target="$1"
    local timestamp="--timestamp"
    shift
    if [ "${rehearsal}" = true ]; then
        timestamp="--timestamp=none"
    fi
    codesign --force --options runtime "${timestamp}" \
        --sign "${sign_identity}" "$@" "${target}"
}

# Sign from the inside out, as Apple asks: a signature covers the signatures of
# what it holds. Only the app itself gets the entitlements.
sign_app() {
    local binary framework
    while IFS= read -r binary; do
        case "${binary}" in
            *.framework/*) ;;
            "${main_program}") ;;
            *) sign "${binary}" ;;
        esac
    done < <(list_binaries)
    while IFS= read -r framework; do
        sign "${framework}"
    done < <(find "${app}/Contents" -type d -name '*.framework' -depth)
    sign "${app}" --entitlements "${ENTITLEMENTS}"
}

# ---------------------------------------------------------------------------
# Checks made on the signed app
# ---------------------------------------------------------------------------

# Check who signed a file, and how. $2 is the name to use in messages.
check_signature() {
    local details
    details="$(signature_details "$1")"
    if [ "${rehearsal}" = false ]; then
        grep -q -x -F "Authority=${sign_identity}" <<< "${details}" \
            || fail "$2 is not signed by ${sign_identity}."
        grep -q -x -F "TeamIdentifier=${team_identifier}" <<< "${details}" \
            || fail "$2 is not signed by team ${team_identifier}."
        grep -q '^Timestamp=' <<< "${details}" \
            || fail "$2 has no secure timestamp."
    fi
}

check_binary() {
    local binary="$1"
    local name="${binary#"${app}/"}"
    local architecture architectures minimum_version outside_libraries
    check_signature "${binary}" "${name}"
    grep -q '^CodeDirectory .*flags=.*runtime' <<< "$(signature_details "${binary}")" \
        || fail "${name} does not use the hardened runtime."

    architectures="$(lipo -archs "${binary}")"
    for architecture in ${RELEASE_ARCHITECTURES}; do
        case " ${architectures} " in
            *" ${architecture} "*) ;;
            *) report_problem "${name} is not built for ${architecture}." ;;
        esac
    done

    while IFS= read -r minimum_version; do
        if version_is_newer "${minimum_version}" "${announced_minimum_version}"; then
            report_problem "${name} needs macOS ${minimum_version}, Info.plist announces ${announced_minimum_version}."
        fi
    done < <(otool -arch all -l "${binary}" | awk '
        $1 == "cmd" { in_version = ($2 == "LC_BUILD_VERSION" || $2 == "LC_VERSION_MIN_MACOSX") }
        in_version && ($1 == "minos" || $1 == "version") { print $2; in_version = 0 }' | sort -u)

    # Only the libraries of the app and of the system may be loaded. otool
    # indents them, unlike the name of the file.
    outside_libraries="$(otool -arch all -L "${binary}" \
        | awk '/^\t/ && $1 ~ /^\// && $1 !~ /^\/(System|usr\/lib)\// { print $1 }')"
    [ -z "${outside_libraries}" ] \
        || fail "${name} loads a library from outside the app: ${outside_libraries}"
}

check_app() {
    local binary entitlements
    codesign --verify --deep --strict "${app}" || fail "the signature of the app is not valid."
    announced_minimum_version="$(/usr/libexec/PlistBuddy -c 'Print :LSMinimumSystemVersion' \
        "${app}/Contents/Info.plist")"
    while IFS= read -r binary; do
        check_binary "${binary}"
        checked_count=$((checked_count + 1))
    done < <(list_binaries)

    entitlements="$(codesign --display --entitlements - --xml "${app}" 2>/dev/null)"
    [ "$(echo "${entitlements}" | plutil -convert json -o - -)" = "$(plutil -convert json -o - "${ENTITLEMENTS}")" ] \
        || fail "the app does not have exactly the entitlements of ${ENTITLEMENTS}."
    case "${entitlements}" in
        *"${DEBUG_ENTITLEMENT}"*) fail "the app has the ${DEBUG_ENTITLEMENT} entitlement." ;;
    esac
}

# ---------------------------------------------------------------------------
# Disk image
# ---------------------------------------------------------------------------

create_disk_image() {
    local attempt=1
    until hdiutil create \
        -volname "${APP_NAME} ${display_version}" \
        -srcfolder "${PACKAGE_STAGE_DIRECTORY}" \
        -format UDZO \
        -quiet \
        "${disk_image}"
    do
        [ "${attempt}" -lt "${DISK_IMAGE_MAXIMUM_ATTEMPTS}" ] \
            || fail "hdiutil failed ${attempt} times; no disk image was made."
        echo "hdiutil failed (attempt ${attempt}); trying again in ${DISK_IMAGE_RETRY_DELAY_SECONDS} seconds." >&2
        sleep "${DISK_IMAGE_RETRY_DELAY_SECONDS}"
        rm -f "${disk_image}"
        attempt=$((attempt + 1))
    done
}

# Check the disk image, and the app in it as a user would get it.
check_disk_image() {
    hdiutil verify -quiet "${disk_image}" || fail "${disk_image} is damaged."
    codesign --verify --strict "${disk_image}" || fail "the signature of ${disk_image} is not valid."
    check_signature "${disk_image}" "${disk_image}"
    mount_point="$(mktemp -d)"
    hdiutil attach -quiet -readonly -nobrowse -mountpoint "${mount_point}" "${disk_image}"
    codesign --verify --deep --strict "${mount_point}/${APP_NAME}.app" \
        || fail "the signature of the app in the disk image is not valid."
    hdiutil detach -quiet "${mount_point}"
    rmdir "${mount_point}" 2>/dev/null || true
    mount_point=""
}

# Fingerprint of a file.
sha256_of() {
    shasum -a 256 "$1" | awk '{ print $1 }'
}

write_report() {
    {
        echo "${APP_NAME} ${display_version} for macOS"
        echo "Commit: ${commit}"
        echo "Qt: $("${qt_directory}/bin/qmake" -query QT_VERSION)"
        echo "Signed by: ${sign_identity}"
        echo "Oldest macOS: ${announced_minimum_version}"
        echo "Programs and libraries checked: ${checked_count}"
        echo "Problems ignored (rehearsal): ${problem_count}"
        echo "SHA-256: $(sha256_of "${disk_image}")"
        echo "Sent to Apple: no"
    } > "${disk_image}.report.txt"
    cat "${disk_image}.report.txt"
}

make_release() {
    local suffix=""
    [ -n "${qt_directory}" ] || fail "--qt-directory is required."
    [ -x "${qt_directory}/bin/qmake" ] || fail "there is no bin/qmake in ${qt_directory}."
    require_tools codesign file git hdiutil lipo openssl otool plutil security shasum

    display_version="$(read_project_variable MAPMAP_VERSION)"
    [ -n "${display_version}" ] || fail "could not read MAPMAP_VERSION from mapmap.pro."
    commit="$(git rev-parse HEAD)"
    if [ "${rehearsal}" = true ]; then
        suffix="-rehearsal"
    fi
    disk_image="${output_directory}/${APP_NAME}-${display_version}-macos-universal${suffix}.dmg"
    # A released file is never replaced by another one of the same name. That
    # of a rehearsal is worth nothing.
    if [ "${rehearsal}" = true ]; then
        rm -f "${disk_image}"
    elif [ -e "${disk_image}" ]; then
        fail "${disk_image} already exists."
    fi

    print_step "Checking the sources, commit ${commit}"
    check_sources
    find_identity

    print_step "Building, with no certificate"
    bash scripts/sh_package_macos.sh \
        --qt-directory "${qt_directory}" --output-directory "${BUILD_DIRECTORY}/unsigned"
    [ "$(git rev-parse HEAD)" = "${commit}" ] || fail "another commit was checked out during the build."
    app="${PACKAGE_STAGE_DIRECTORY}/${APP_NAME}.app"
    main_program="${app}/Contents/MacOS/${APP_NAME}"

    print_step "Signing the app: ${sign_identity}"
    sign_app

    print_step "Checking the app"
    checked_count=0
    check_app

    print_step "Making the disk image"
    mkdir -p "${output_directory}"
    create_disk_image
    sign "${disk_image}"
    check_disk_image

    print_step "Done, nothing was sent to Apple"
    write_report
    echo
    echo "Disk image: ${disk_image}"
    if [ "${rehearsal}" = true ]; then
        echo "Rehearsal: do not distribute this disk image."
    else
        echo "Try it by hand (a camera, a video with sound) before --submit-to-apple."
    fi
}

# ---------------------------------------------------------------------------
# Notarization
# ---------------------------------------------------------------------------

submit_to_apple() {
    local details expected_answer answer report result status submission_identifier
    require_tools codesign hdiutil plutil shasum spctl xcrun
    [ -f "${submitted_image}" ] || fail "${submitted_image} does not exist."
    [ -n "${notary_profile}" ] || fail "--notary-profile is required to submit to Apple."
    report="${submitted_image}.report.txt"
    [ -f "${report}" ] || fail "${report} is missing: was this disk image made by this script?"

    codesign --verify --strict "${submitted_image}" \
        || fail "the signature of ${submitted_image} is not valid."
    details="$(signature_details "${submitted_image}")"
    grep -q "^Authority=${IDENTITY_PREFIX} " <<< "${details}" \
        || fail "${submitted_image} is not signed with a \"${IDENTITY_PREFIX}\" certificate."
    grep -q -x -F "SHA-256: $(sha256_of "${submitted_image}")" "${report}" \
        || fail "${submitted_image} is not the file that ${report} describes."
    grep -q -x -F "Sent to Apple: no" "${report}" \
        || fail "${submitted_image} was already sent to Apple."

    cat "${report}"
    echo
    echo "This sends ${submitted_image} to Apple."
    # Only a person at the keyboard may confirm.
    [ -t 0 ] || fail "the confirmation must be typed in a terminal."
    expected_answer="$(basename "${submitted_image}")"
    printf 'Type the name of the file (%s) to send it, anything else to stop: ' "${expected_answer}"
    IFS= read -r answer
    [ "${answer}" = "${expected_answer}" ] || fail "nothing was sent."

    print_step "Waiting for Apple"
    result="${submitted_image}.notarization.plist"
    xcrun notarytool submit "${submitted_image}" \
        --keychain-profile "${notary_profile}" --wait --output-format plist > "${result}" \
        || fail "the submission failed, see ${result}."
    status="$(plutil -extract status raw -o - "${result}")"
    submission_identifier="$(plutil -extract id raw -o - "${result}")"
    xcrun notarytool log "${submission_identifier}" \
        --keychain-profile "${notary_profile}" "${submitted_image}.notarization-log.json" \
        || echo "Warning: could not get the log of Apple." >&2
    [ "${status}" = "Accepted" ] \
        || fail "Apple answered \"${status}\", see ${submitted_image}.notarization-log.json."

    print_step "Attaching the approval of Apple"
    xcrun stapler staple "${submitted_image}"
    xcrun stapler validate "${submitted_image}"
    spctl --assess --type open --context context:primary-signature --verbose=2 "${submitted_image}" \
        || fail "macOS does not accept ${submitted_image}."

    # The approval changes the file, hence its fingerprint.
    grep -v -e '^SHA-256: ' -e '^Sent to Apple: ' "${report}" > "${report}.new"
    {
        echo "SHA-256: $(sha256_of "${submitted_image}")"
        echo "Sent to Apple: yes, accepted (${submission_identifier})"
    } >> "${report}.new"
    mv "${report}.new" "${report}"
    print_step "Notarized; nothing was published"
    cat "${report}"
}

while [ $# -gt 0 ]; do
    case "$1" in
        --notary-profile|--output-directory|--qt-directory|--sign-identity|--submit-to-apple)
            if [ $# -lt 2 ]; then
                echo "Error: $1 needs a value." >&2
                exit 2
            fi
            case "$1" in
                --notary-profile) notary_profile="$2" ;;
                --output-directory) output_directory="$(absolute_path "$2")" ;;
                --qt-directory) qt_directory="$(absolute_path "$2")" ;;
                --sign-identity) sign_identity="$2" ;;
                --submit-to-apple) submitted_image="$(absolute_path "$2")" ;;
            esac
            shift
            ;;
        --rehearsal)
            rehearsal=true
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

if [ -n "${submitted_image}" ]; then
    [ "${rehearsal}" = false ] || fail "--rehearsal cannot be combined with --submit-to-apple."
    submit_to_apple
else
    make_release
fi
