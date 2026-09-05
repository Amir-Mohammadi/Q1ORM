#!/usr/bin/env bash
# ============================================
#           Q1ORM Release Builder
# ============================================
set -uo pipefail

die() { printf '%s\n' "$*" >&2; pause_exit 1; }
pause_exit() {
    local code="${1:-0}"
    if [[ -t 0 ]]; then
        read -r -p "Press Enter to continue..." _ || true
    fi
    exit "$code"
}

echo "============================================"
echo "          Q1ORM Release Builder"
echo "============================================"

# ---- Project paths (auto-derived from this script's location) ----
PROJECT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$PROJECT/build"
RELEASE="$PROJECT/Releases/Release-0.1"

# ---- Auto-detect Qt (QTDIR -> newest <root>/<ver>/gcc_64 -> qmake on PATH) ----
QT=""
if [[ -n "${QTDIR:-}" && -x "$QTDIR/bin/qmake" ]]; then
    QT="$QTDIR"
fi

if [[ -z "$QT" ]]; then
    for root in "$HOME/Qt" /opt/Qt /usr/local/Qt; do
        [[ -d "$root" ]] || continue
        while IFS= read -r dir; do
            for kit in gcc_64 macos clang_64; do
                if [[ -z "$QT" && -d "$dir/$kit" ]]; then
                    QT="$dir/$kit"
                fi
            done
        done < <(find "$root" -mindepth 1 -maxdepth 1 -type d | sort -Vr)
        [[ -n "$QT" ]] && break
    done
fi

if [[ -z "$QT" ]]; then
    for qm in qmake6 qmake; do
        if command -v "$qm" >/dev/null 2>&1; then
            QT="$("$qm" -query QT_INSTALL_PREFIX)"
            break
        fi
    done
fi

[[ -n "$QT" ]] || echo "WARNING: Qt not found. Set QTDIR or run from a Qt-enabled shell."

QT_BIN=""
[[ -n "$QT" ]] && QT_BIN="$QT/bin"

# Linux/macOS equivalent of windeployqt: optional, only used if present.
DEPLOYQT=""
for cand in "${QT_BIN:+$QT_BIN/macdeployqt}" "$(command -v linuxdeployqt 2>/dev/null || true)"; do
    if [[ -n "$cand" && -x "$cand" ]]; then
        DEPLOYQT="$cand"
        break
    fi
done

# ---- Parallel jobs ----
if command -v nproc >/dev/null 2>&1; then
    JOBS="$(nproc)"
elif command -v sysctl1; then
    JOBS; then
    JOBS="$(sysctl -n hw.ncpu)"
else
    JOBS=4
fi

echo
echo "Project : $PROJECT"
echo "Qt      : ${QT:-<not found>}"
echo "Build   : $BUILD"
echo "Jobs    : $JOBS"
echo

echo "[1/6] Configuring project..."
cmake_args=(-S "$PROJECT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release)
[[ -n "$QT" ]] && cmake_args+=(-DCMAKE_PREFIX_PATH="$QT")
cmake "${cmake_args[@]}" || die "CONFIGURE FAILED!"

echo
echo "[2/6] Building library..."
cmake --build "$BUILD" --target Src --config Release -j "$JOBS" || die "LIBRARY BUILD FAILED!"

echo
echo "[3/6] Building tools..."
cmake --build "$BUILD" --target ExampleProjectModifier --config Release -j "$JOBS" \
    || die "EXAMPLE PROJECT MODIFIER BUILD FAILED!"

echo
echo "[4/6] Building examples (all registered example projects)..."
cmake --build "$BUILD" --config Release -j "$JOBS" || die "EXAMPLES BUILD FAILED!"

echo
echo "[5/6] Copying fresh libQ1ORM.so next to released example executables..."
LIB=""
for cand in \
    "$BUILD/src/libQ1ORM.so" \
    "$BUILD/src/Release/libQ1ORM.so" \
    "$BUILD/src/libQ1ORM.dylib" \
    "$BUILD/src/Release/libQ1ORM.dylib"; do
    if [[ -f "$cand" ]]; then
        LIB="$cand"
        break
    fi
done

if [[ -n "$LIB" && -d "$BUILD/Examples" ]]; then
    for exdir in "$BUILD/Examples"/*/; do
        [[ -d "$exdir" ]] || continue
        target="$exdir"
        [[ -d "$exdir/Release" ]] && target="$exdir/Release"
        cp -f "$LIB" "$target/$(basename "$LIB")"
    done
else
    echo "WARNING: Q1ORM shared library or Examples directory not found - skipping copy."
fi

echo
echo "[6/6] Syncing Qt Creator build trees (Debug and Release)..."
for pattern in "Debug" "Release"; do
    tree=""
    while IFS= read -r dir; do
        tree="$dir"
    done < <(find "$BUILD" -maxdepth 1 -type d -name "Desktop_Qt_*_$pattern" 2>/dev/null | sort)

    if [[ -n "$tree" ]]; then
        if [[ -f "$tree/CMakeCache.txt" ]]; then
            echo "Building all in Qt Creator $pattern tree..."
            cmake --build "$tree" -j "$JOBS" || die "QT $pattern BUILD FAILED!"
        else
            echo "INFO: No Qt Creator $pattern tree configured yet - skipping."
        fi
    else
        echo "INFO: No Qt Creator $pattern tree found - skipping."
    fi
done

echo
echo "Installing library and examples..."
cmake --install "$BUILD" --config Release --prefix "$RELEASE" || die "INSTALL FAILED!"

echo
echo "Verifying release..."
[[ -d "$RELEASE" ]] || die "RELEASE DIRECTORY NOT FOUND!"

echo
echo "Deploying Qt runtime..."
if [[ -n "$DEPLOYQT" ]]; then
    APP=""
    for cand in "$RELEASE/bin/UnitTestExample" "$RELEASE/bin/UnitTestExample.app"; do
        [[ -e "$cand" ]] && APP="$cand" && break
    done
    if [[ -n "$APP" ]]; then
        "$DEPLOYQT" "$APP" || die "QT RUNTIME DEPLOY FAILED!"
    else
        echo "WARNING: UnitTestExample not found in release bin. Skipping Qt runtime deploy."
    fi
else
    echo "WARNING: No deploy tool (linuxdeployqt/macdeployqt) found. Skipping Qt runtime deploy."
fi

echo
echo "============================================"
echo "    RELEASE CREATED SUCCESSFULLY"
echo "============================================"
echo
echo "Release folder:"
echo "$RELEASE"
echo
echo "Contents of bin:"
if [[ -d "$RELEASE/bin" ]]; then
    ls -1 "$RELEASE/bin"
else
    echo "WARNING: bin directory not found."
fi

echo
pause_exit 0
