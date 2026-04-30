#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
#  build.sh
#
#  Builds the C++ engine to WebAssembly using Emscripten, then outputs the
#  glue .js and .wasm files to web/src/wasm/ where Vite can serve them.
#
#  Prerequisites:
#    - emsdk installed and activated (run setup_emsdk.sh first if needed)
#    - cmake >= 3.20
#
#  Usage:
#    ./build.sh          # debug build
#    ./build.sh release  # optimized build
# ─────────────────────────────────────────────────────────────────────────────

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CPP_DIR="$SCRIPT_DIR/cpp"
BUILD_DIR="$SCRIPT_DIR/build"
BUILD_TYPE="${1:-Debug}"

# ── Activate emsdk if not already active ────────────────────────────────────
if ! command -v emcc &> /dev/null; then
    EMSDK_DIR="$HOME/emsdk"
    if [[ -f "$EMSDK_DIR/emsdk_env.sh" ]]; then
        source "$EMSDK_DIR/emsdk_env.sh" > /dev/null
    else
        echo "[build.sh] ERROR: emcc not found. Run setup_emsdk.sh first."
        exit 1
    fi
fi

echo "[build.sh] Using emcc: $(which emcc)"
echo "[build.sh] Build type: $BUILD_TYPE"

# ── Configure with CMake ─────────────────────────────────────────────────────
mkdir -p "$BUILD_DIR"

emcmake cmake \
    -S "$CPP_DIR" \
    -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# ── Build ────────────────────────────────────────────────────────────────────
emmake make -C "$BUILD_DIR" -j"$(nproc)"

echo ""
echo "[build.sh] Build complete."
echo "[build.sh] Output: web/src/wasm/projection_engine.js + .wasm"
echo "[build.sh] Start the dev server: cd web && npm run dev"
