# DoomRPG-RE

![image](https://github.com/Erick194/DoomRPG-RE/assets/41172072/258e99d9-b122-4cbe-8659-2fd0f4105068)<br />
https://www.doomworld.com/forum/topic/129997

## Español
Doom RPG ingeniería inversa por [GEC]<br />
Creado por Erick Vásquez García.

Versión actual 0.2.2

Requiere CMake para crear el proyecto.<br />
Requisitos para el projecto:
  * SDL2
  * SDL2-Mixer
  * Zlib
  * FluidSynth

Configuración por defecto de las teclas.

Move Forward: Up<br />
Move Backward: Down<br />
Move Left: A<br />
Move Right: D<br />
Turn Left: Left<br />
Turn Right: Right<br />
Atk/Talk/Use: Return<br />
Next Weapon: Z<br />
Prev Weapon: X<br />
Pass Turn: C<br />
Automap: Tab<br />
Menu Open/Back: Escape<br />

Trucos originales del juego:

Versión J2ME/BREW:<br />
Abres menu e ingresa los siguientes numeros.<br />
3666 -> Abre el menú debug.<br />
43629 -> Da al jugador maximo de salud y armadura.<br />
4332 -> Da al jugador todas las llaves, items y armas.<br />
3366 -> Inicia el testeo de velocidad, "Benchmark".<br />

## English
Doom RPG Reverse Engineering By [GEC]<br />
Created by Erick Vásquez García.

Current version 0.2.2

You need CMake to make the project.<br />
What you need for the project is:
  * SDL2
  * SDL2-Mixer
  * Zlib
  * FluidSynth

Default key configuration:

Move Forward: Up<br />
Move Backward: Down<br />
Move Left: A<br />
Move Right: D<br />
Turn Left: Left<br />
Turn Right: Right<br />
Atk/Talk/Use: Return<br />
Next Weapon: Z<br />
Prev Weapon: X<br />
Pass Turn: C<br />
Automap: Tab<br />
Menu Open/Back: Escape<br />

Original game cheat codes:

J2ME/BREW Version:<br />
3666 -> Opens debug menu.<br />
43629 -> Gives max health and armor to the player.<br />
4332 -> Gives all keys, items and weapons to the player.<br />
3366 -> Starts speed test "Benchmark".<br />

## Web (WebAssembly) build

This fork adds a browser build via [Emscripten](https://emscripten.org). The C
engine is compiled as-is; all web-only changes are guarded behind
`#ifdef __EMSCRIPTEN__`, so the native CMake build is unaffected. FluidSynth
(which can't run in a browser) is replaced by a self-contained
[TinySoundFont](https://github.com/schellingb/TinySoundFont) MIDI player, and
saves persist in the browser via IndexedDB.

### Game data is NOT included

This repository does **not** ship the game's `DoomRPG.zip` or the `gm.sf2`
soundfont — you must supply your own from legally obtained game data, exactly as
the native build requires. Place them in `data/`:

* **`data/DoomRPG.zip`** — built from the original BREW resource archive
  `doomrpg.bar` with the upstream **`BarToZip.exe`** tool. On macOS,
  `tools/make-data.sh` automates this (it runs BarToZip under Wine and needs
  `zlib.dll` beside the exe). You can also reuse the `DoomRPG.zip` from the
  native PC port.
* **`data/gm.sf2`** — any General MIDI soundfont (the GEC PC-port release ships one).

> A J2ME `DoomRPG.jar` is **not** a valid data source: its texture/map data uses
> a different (smaller) layout than this reverse-engineering expects, so it won't
> render. Use the BREW `.bar` → BarToZip path. (`tools/jar2zip.py` can build a
> *bootable* archive from a jar to smoke-test the engine, but its 3D art is wrong.)

### Build & run

```sh
# 1. Install the Emscripten SDK:
#    https://emscripten.org/docs/getting_started/downloads.html
# 2. Provide game data (your own doomrpg.bar + a gm.sf2):
./tools/make-data.sh        # -> data/DoomRPG.zip + data/gm.sf2
# 3. Build the web app:
./build-web.sh              # -> build-web/DoomRPG.{html,js,wasm,data}
# 4. Serve it (any static server; no COOP/COEP needed):
cd build-web && python3 -m http.server 8765
#    then open http://localhost:8765/DoomRPG.html
```

See [`web/README.md`](web/README.md) for the full list of changes and details.
