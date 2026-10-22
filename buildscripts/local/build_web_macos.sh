#!/usr/bin/env bash
# Build the experimental MuseScore web app (app-web) on macOS.
#
# Requirements (versions match the upstream Wasm CI):
#   - emsdk 3.1.70        (EMSDK_DIR, default ~/emsdk)
#   - Qt 6.9.2 wasm_singlethread + matching macOS host Qt (QT_DIR, default ~/Qt/6.9.2)
#   - cmake, ninja
#
# Usage: buildscripts/local/build_web_macos.sh [configure|build|serve]
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
EMSDK_DIR="${EMSDK_DIR:-$HOME/emsdk}"
QT_DIR="${QT_DIR:-$HOME/Qt/6.9.2}"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build.web}"
STEP="${1:-build}"

# shellcheck disable=SC1091
source "$EMSDK_DIR/emsdk_env.sh" >/dev/null

configure() {
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -GNinja \
        -DCMAKE_TOOLCHAIN_FILE="$QT_DIR/wasm_singlethread/lib/cmake/Qt6/qt.toolchain.cmake" \
        -DQT_HOST_PATH="$QT_DIR/macos" \
        -DCMAKE_BUILD_TYPE=Release \
        -DMUSESCORE_BUILD_CONFIGURATION=app-web \
        -DMUSE_APP_BUILD_MODE=dev \
        -DMUSESCORE_REVISION="$(git -C "$ROOT_DIR" rev-parse --short=7 HEAD)"
}

case "$STEP" in
configure)
    configure
    ;;
build)
    [ -f "$BUILD_DIR/build.ninja" ] || configure
    cmake --build "$BUILD_DIR"
    ;;
serve)
    # Qt Wasm apps must be served over HTTP, not opened as file:// URLs.
    # Open http://127.0.0.1:8000/test.html (or viewer.html) afterwards.
    cd "$BUILD_DIR/public_html"
    python3 -m http.server --bind 127.0.0.1 8000
    ;;
*)
    echo "Unknown step: $STEP (expected configure, build or serve)" >&2
    exit 1
    ;;
esac
