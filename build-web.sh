#!/usr/bin/env bash
#
# Build DoomRPG-RE for the web (Emscripten -> WebAssembly).
#
# Requires the emsdk (set EMSDK_DIR, default /Users/tim/Downloads/emsdk).
# Game data is preloaded from $DOOMRPG_DATA_DIR (default ./data):
#   - DoomRPG.zip  (built from the J2ME jar by tools/jar2zip.js)
#   - gm.sf2       (General MIDI soundfont for music)
# If a data file is missing the build still succeeds (useful for a compile
# check), but the game will not fully boot without DoomRPG.zip.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$SCRIPT_DIR/src"
THIRD_PARTY="$SCRIPT_DIR/third_party"
OUT="$SCRIPT_DIR/build-web"
SHELL_FILE="$SCRIPT_DIR/web/shell.html"
DATA_DIR="${DOOMRPG_DATA_DIR:-$SCRIPT_DIR/data}"
EMSDK="${EMSDK_DIR:-/Users/tim/Downloads/emsdk}"

if ! command -v emcc >/dev/null 2>&1; then
  # shellcheck disable=SC1091
  source "$EMSDK/emsdk_env.sh"
fi

mkdir -p "$OUT"

PRELOAD=()
if [[ -f "$DATA_DIR/DoomRPG.zip" ]]; then
  PRELOAD+=(--preload-file "$DATA_DIR/DoomRPG.zip@/DoomRPG.zip")
else
  echo "WARNING: $DATA_DIR/DoomRPG.zip not found - game will not boot."
fi
if [[ -f "$DATA_DIR/gm.sf2" ]]; then
  PRELOAD+=(--preload-file "$DATA_DIR/gm.sf2@/gm.sf2")
else
  echo "WARNING: $DATA_DIR/gm.sf2 not found - music disabled."
fi

echo "Compiling..."
emcc \
  "$SRC"/*.c \
  -I "$SRC" -I "$THIRD_PARTY" \
  -O3 \
  -Wno-invalid-source-encoding \
  -sUSE_SDL=2 -sUSE_SDL_MIXER=2 -sUSE_ZLIB=1 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sFORCE_FILESYSTEM=1 \
  -sEXIT_RUNTIME=0 \
  -sINVOKE_RUN=0 \
  -sEXPORTED_RUNTIME_METHODS=callMain,FS \
  -sEXPORTED_FUNCTIONS=_main,_Web_inGame,_Web_doAction \
  -lidbfs.js \
  "${PRELOAD[@]}" \
  --shell-file "$SHELL_FILE" \
  -o "$OUT/DoomRPG.html"

echo "Built: $OUT/DoomRPG.html"
