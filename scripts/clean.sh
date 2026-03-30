#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

rm -rf \
    "$ROOT_DIR/build" \
    "$ROOT_DIR/dist" \
    "$ROOT_DIR/release"

rm -f \
    "$ROOT_DIR/bincalc" \
    "$ROOT_DIR/bincalc_tests" \
    "$ROOT_DIR/tests.o" \
    "$ROOT_DIR/Makefile" \
    "$ROOT_DIR/Makefile.tests" \
    "$ROOT_DIR/moc_predefs.h" \
    "$ROOT_DIR/moc_bincalc.cpp" \
    "$ROOT_DIR/moc_bincalc.o" \
    "$ROOT_DIR/qrc_resources.cpp" \
    "$ROOT_DIR/qrc_resources.o" \
    "$ROOT_DIR/ui_bincalc.h" \
    "$ROOT_DIR/calculator_core.o" \
    "$ROOT_DIR/numeric_formats.o" \
    "$ROOT_DIR/main.o" \
    "$ROOT_DIR/bincalc.o"
