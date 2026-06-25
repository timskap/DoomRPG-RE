# Doom RPG — Web (WebAssembly) port

This builds DoomRPG-RE for the browser with [Emscripten](https://emscripten.org).
The C engine is compiled unchanged except for small, `#ifdef __EMSCRIPTEN__`-guarded
adaptations, so the native CMake build is unaffected.

## Quick start

```sh
# 1. one-time: install the Emscripten SDK and activate it
#    (https://emscripten.org/docs/getting_started/downloads.html)

# 2. build the game data archive (data/DoomRPG.zip + data/gm.sf2)
./tools/make-data.sh

# 3. build the web app  ->  build-web/DoomRPG.{html,js,wasm,data}
./build-web.sh

# 4. serve it (a static server is enough; single-threaded, no COOP/COEP needed)
cd build-web && python3 -m http.server 8765
#    open http://localhost:8765/DoomRPG.html
```

Controls: arrows move/turn, `A`/`D` strafe, `Enter` attack/use/talk, `Z`/`X`
weapon, `C` pass turn, `Tab` automap, `Esc` menu/back. Click the canvas to
capture the mouse for look/turn. Saves persist in the browser (IndexedDB).

## Game data

The engine reads a `DoomRPG.zip` of GEC/BREW-format resources (texels, maps,
entities, BMP images, WAV/MIDI). `tools/make-data.sh` produces it by running the
upstream **BarToZip.exe** against the original BREW archive `doomrpg.bar`, under
Wine. BarToZip hardcodes the input name `doomrpg.bar`, output `DoomRPG.zip`, and
needs `zlib.dll` beside the exe. Override locations with the env vars documented
at the top of `make-data.sh`.

> Note: a J2ME `DoomRPG.jar` is **not** a valid source — its texture/map data
> uses a different (smaller) layout than this reverse-engineering expects. The
> helper `tools/jar2zip.py` can build a *bootable* archive from a jar to smoke-test
> the engine, but its 3D art will be wrong; use the BarToZip/`.bar` path for a
> correct build.

## What the port changes (all guarded for `__EMSCRIPTEN__`)

| Concern | Native | Web |
|---|---|---|
| Main loop | blocking `while` | `emscripten_set_main_loop` (`src/Main.c`) |
| Music | FluidSynth (glib + own audio thread) | **TinySoundFont** via `Mix_HookMusic` (`src/MidiPlayer.{c,h}`, `third_party/tsf.h`+`tml.h`) |
| Saves | files in cwd | IDBFS mounted at `/save`, `chdir`'d into; `FS.syncfs` after each save (`src/Web.{c,h}`, `web/shell.html`) |
| Fatal errors | `SDL_ShowMessageBox`+`exit` | DOM overlay + cancel loop (`DoomRPG_Error`) |
| `SDL_Init` | `SDL_INIT_EVERYTHING` | minus `HAPTIC` (unsupported) |
| Assets/soundfont | relative names | absolute `/DoomRPG.zip`, `/gm.sf2` (preloaded into MEMFS) |

Two pre-existing portability bugs that MSVC tolerated but clang/wasm reject were
also fixed: a K&R declaration `Sound_getFromResourceID(resourceID)` and the
engine's `SDL_CloseAudio` (which collided with SDL2's own symbol; renamed
`DoomRPG_CloseAudio`).

## Layout

```
build-web.sh          emcc build script (-> build-web/)
web/shell.html        HTML shell: canvas, IDBFS mount, click-to-start
web/README.md         this file
tools/make-data.sh    BarToZip(.bar) -> data/DoomRPG.zip + gm.sf2
tools/jar2zip.py      J2ME jar -> bootable (art-incorrect) archive (smoke test)
third_party/tsf.h     TinySoundFont (SF2 synth)
third_party/tml.h     TinyMidiLoader (MIDI parser)
src/MidiPlayer.{c,h}  FluidSynth-API shim backed by TSF+TML
src/Web.{c,h}         browser helpers (IDBFS sync, error overlay)
```
