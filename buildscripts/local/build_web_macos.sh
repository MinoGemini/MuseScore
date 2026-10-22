#!/usr/bin/env bash
# Build the experimental MuseScore web app (app-web) on macOS.
#
# Requirements (versions match the upstream Wasm CI):
#   - emsdk 3.1.70        (EMSDK_DIR, default ~/emsdk)
#   - Qt 6.9.2 for Wasm + matching macOS host Qt (QT_DIR, default ~/Qt/6.9.2)
#   - cmake, ninja
#
# QT_WASM_ARCH selects the Qt build: wasm_multithread (default) is required for
# playback, because the audio worker runs on its own thread. The page must then
# be served with COOP/COEP headers (see "serve").
#
# Usage: buildscripts/local/build_web_macos.sh [configure|build|serve]
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
EMSDK_DIR="${EMSDK_DIR:-$HOME/emsdk}"
QT_DIR="${QT_DIR:-$HOME/Qt/6.9.2}"
QT_WASM_ARCH="${QT_WASM_ARCH:-wasm_multithread}"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build.web-$QT_WASM_ARCH}"
PORT="${PORT:-8000}"
STEP="${1:-build}"

# shellcheck disable=SC1091
source "$EMSDK_DIR/emsdk_env.sh" >/dev/null

configure() {
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -GNinja \
        -DCMAKE_TOOLCHAIN_FILE="$QT_DIR/$QT_WASM_ARCH/lib/cmake/Qt6/qt.toolchain.cmake" \
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
    # Qt Wasm apps must be served over HTTP, and threads (SharedArrayBuffer)
    # need cross-origin isolation. Open http://127.0.0.1:$PORT/test.html afterwards.
    cd "$BUILD_DIR/public_html"
    python3 - "$PORT" <<'EOF'
import functools, http.server, sys

class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {**http.server.SimpleHTTPRequestHandler.extensions_map,
                      ".wasm": "application/wasm", ".js": "text/javascript"}

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()

port = int(sys.argv[1])
print(f"Serving on http://127.0.0.1:{port}/")
http.server.ThreadingHTTPServer(("127.0.0.1", port), functools.partial(Handler, directory=".")).serve_forever()
EOF
    ;;
*)
    echo "Unknown step: $STEP (expected configure, build or serve)" >&2
    exit 1
    ;;
esac
