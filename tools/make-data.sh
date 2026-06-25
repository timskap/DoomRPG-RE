#!/usr/bin/env bash
#
# make-data.sh -- produce the data/ archive the web build preloads:
#   data/DoomRPG.zip  (the engine's game data, in GEC/BREW format)
#   data/gm.sf2       (General MIDI soundfont for music)
#
# DoomRPG.zip is generated from the original BREW resource archive
# (doomrpg.bar) using the upstream BarToZip.exe tool, run under Wine.
# BarToZip hardcodes its input name "doomrpg.bar" and output "DoomRPG.zip",
# and dynamically links zlib.dll -- both must sit next to the exe.
#
# Tested on Apple Silicon macOS with the game-porting-toolkit Wine
# (/usr/local/bin/wine64). Override the paths below via env vars as needed.
#
set -euo pipefail

PROJECT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BAR="${DOOMRPG_BAR:-/Users/tim/Downloads/doomrpg-2/doomrpg.bar}"
BARTOZIP="${BARTOZIP_EXE:-/Users/tim/Downloads/DoomRPG/BarToZip.exe}"
ZLIB_DLL="${ZLIB_DLL:-/Users/tim/Downloads/DoomRPG/zlib.dll}"
SF2="${DOOMRPG_SF2:-/Users/tim/Downloads/DoomRPG/gm.sf2}"
WINE="${WINE:-/usr/local/bin/wine64}"

mkdir -p "$PROJECT/data"

# --- gm.sf2 ---------------------------------------------------------------
if [[ -f "$SF2" ]]; then
  cp "$SF2" "$PROJECT/data/gm.sf2"
  echo "data/gm.sf2 <- $SF2"
else
  echo "WARNING: soundfont $SF2 not found (music will be disabled)."
fi

# --- DoomRPG.zip via BarToZip.exe ----------------------------------------
if [[ ! -f "$BAR" ]]; then
  echo "ERROR: BREW archive not found: $BAR (set DOOMRPG_BAR)" >&2; exit 1
fi
if ! command -v "$WINE" >/dev/null 2>&1; then
  echo "ERROR: wine not found at $WINE (set WINE)" >&2; exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cp "$BARTOZIP" "$WORK/BarToZip.exe"
cp "$ZLIB_DLL" "$WORK/zlib.dll"
cp "$BAR"      "$WORK/doomrpg.bar"

export WINEPREFIX="$WORK/.wineprefix"
export WINEDLLOVERRIDES="mscoree,mshtml="
export WINEDEBUG=-all

( cd "$WORK" && "$WINE" BarToZip.exe >/dev/null 2>&1 ) || true

if [[ -f "$WORK/DoomRPG.zip" ]]; then
  cp "$WORK/DoomRPG.zip" "$PROJECT/data/DoomRPG.zip"
  echo "data/DoomRPG.zip <- BarToZip($BAR)  ($(wc -c < "$PROJECT/data/DoomRPG.zip") bytes)"
else
  echo "ERROR: BarToZip did not produce DoomRPG.zip (check Wine setup)." >&2; exit 1
fi
