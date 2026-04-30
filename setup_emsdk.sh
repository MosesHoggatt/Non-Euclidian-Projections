#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
#  setup_emsdk.sh
#
#  Downloads, installs, and activates the Emscripten SDK.
#  Only needs to be run once per machine.
# ─────────────────────────────────────────────────────────────────────────────

set -euo pipefail

EMSDK_DIR="$HOME/emsdk"
EMSDK_VERSION="latest"

if [[ -d "$EMSDK_DIR" ]]; then
    echo "[setup_emsdk.sh] emsdk already present at $EMSDK_DIR"
else
    echo "[setup_emsdk.sh] Cloning emsdk..."
    git clone https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
fi

cd "$EMSDK_DIR"

echo "[setup_emsdk.sh] Installing Emscripten $EMSDK_VERSION..."
./emsdk install "$EMSDK_VERSION"

echo "[setup_emsdk.sh] Activating..."
./emsdk activate "$EMSDK_VERSION"

echo ""
echo "[setup_emsdk.sh] Done. Add this to your shell config to persist:"
echo "    source \$HOME/emsdk/emsdk_env.sh"
echo ""
echo "Or just run ./build.sh — it sources emsdk_env.sh automatically."
