#!/usr/bin/env bash
set -euo pipefail

die() {
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

PROJECT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

# Separate directory to avoid changing your existing build tree.
BUILD="${Q1ORM_BUILD_DIR:-$PROJECT/build-ubuntu}"
RELEASE="${Q1ORM_RELEASE_DIR:-$PROJECT/Releases/Release-0.1}"

JOBS="${CMAKE_BUILD_PARALLEL_LEVEL:-$(nproc)}"
QT="${QTDIR:-}"

command -v cmake >/dev/null 2>&1 || die "CMake is not installed."

echo "============================================"
echo "          Q1ORM Ubuntu Builder"
echo "============================================"
echo "Project : $PROJECT"
echo "Build   : $BUILD"
echo "Release : $RELEASE"

cmake_args=(
    -S "$PROJECT"
    -B "$BUILD"
    -DCMAKE_BUILD_TYPE=Release
    -DQ1ORM_BUILD_TOOLS=ON
    "-DQ1ORM_RELEASE_DIR=$RELEASE"
    "-DQ1ORM_RELEASE_SQL_DRIVERS=${Q1ORM_RELEASE_SQL_DRIVERS:-QMYSQL;QSQLITE}"
)

# If QTDIR is provided, use that Qt installation.
# Otherwise let find_package() in CMake locate Qt.
if [[ -n "$QT" ]]; then
    [[ -d "$QT" ]] || die "QTDIR does not exist: $QT"
    cmake_args+=("-DCMAKE_PREFIX_PATH=$QT")
    echo "Qt      : $QT"
else
    echo "Qt      : default CMake discovery"
fi

echo
echo "[1/5] Configuring..."
cmake "${cmake_args[@]}"

echo
echo "[2/5] Building library..."
# Your current build tree indicates that the library target is Src.
cmake --build "$BUILD" \
    --target Src \
    --config Release \
    --parallel "$JOBS"

echo
echo "[3/5] Building tools..."
cmake --build "$BUILD" \
    --target ExampleProjectModifier \
    --config Release \
    --parallel "$JOBS"

echo
echo "[4/5] Building all targets..."
cmake --build "$BUILD" \
    --config Release \
    --parallel "$JOBS"

echo
echo "[5/5] Installing..."
cmake --install "$BUILD" \
    --config Release \
    --prefix "$RELEASE"

echo
echo "Packaging Qt SQL plugins and native client dependencies..."
cmake --build "$BUILD" --target Q1ORMReleaseRuntime --config Release --parallel "$JOBS"

echo
echo "Installed files:"
find "$RELEASE" -maxdepth 2 \( -type f -o -type l \) -print

echo
echo "Build and installation completed."
echo "Qt SQL runtime libraries are bundled and verified in $RELEASE."
echo "Use bin/q1orm-run to launch your application with the bundled libraries."
