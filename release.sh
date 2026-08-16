#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
RELEASE_DIR="$SCRIPT_DIR/Releases/Release-0.1"

echo "============================================"
echo " Q1ORM Release Builder"
echo "============================================"

echo ""
echo "Project : $SCRIPT_DIR"
echo "Build   : $BUILD_DIR"
echo ""

echo ""
echo "[1/5] Configuring project..."
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR"

echo ""
echo "[2/5] Building library and examples..."
cmake --build "$BUILD_DIR" --target Src
cmake --build "$BUILD_DIR"

echo ""
echo "[3/5] Copying fresh library next to example executables..."
for exe_dir in "$BUILD_DIR"/Examples/*/; do
    if [ -d "$exe_dir" ]; then
        cp -f "$BUILD_DIR/src/libQ1ORM.so" "$exe_dir/" 2>/dev/null || true
    fi
done

echo ""
echo "[4/5] Installing into release folder..."
cmake --install "$BUILD_DIR" --prefix "$RELEASE_DIR"

echo ""
echo "[5/5] Release created: $RELEASE_DIR"

echo ""
echo "============================================"
echo " RELEASE CREATED SUCCESSFULLY"
echo "============================================"
echo ""
echo "Release folder: $RELEASE_DIR"
echo ""
echo "Folder structure:"
echo "  lib/          - Q1ORM.so, Q1ORM.a"
echo "  include/      - Q1ORM headers"
echo "  scripts/      - installer scripts"