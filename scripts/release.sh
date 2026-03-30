#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build/release}"
STAGE_DIR="${STAGE_DIR:-$ROOT_DIR/dist/stage}"
ARTIFACT_DIR="${ARTIFACT_DIR:-$ROOT_DIR/dist}"

jobs() {
    if command -v getconf >/dev/null 2>&1; then
        getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4
    else
        echo 4
    fi
}

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j"$(jobs)"
ctest --test-dir "$BUILD_DIR" --output-on-failure

rm -rf "$STAGE_DIR"
cmake --install "$BUILD_DIR" --prefix "$STAGE_DIR"

case "$(uname -s)" in
    Darwin)
        if command -v macdeployqt >/dev/null 2>&1; then
            macdeployqt "$STAGE_DIR/bincalc.app"
        else
            echo "warning: macdeployqt not found; app bundle will not be self-contained" >&2
        fi
        ;;
    Linux)
        if command -v linuxdeployqt >/dev/null 2>&1; then
            linuxdeployqt "$STAGE_DIR/bin/bincalc" -unsupported-allow-new-glibc
        else
            echo "warning: linuxdeployqt not found; archive will require compatible system Qt libraries" >&2
        fi
        ;;
esac

cmake --build "$BUILD_DIR" --target package

mkdir -p "$ARTIFACT_DIR"
echo "staged release in $STAGE_DIR"
echo "packaged artifacts in $BUILD_DIR"
