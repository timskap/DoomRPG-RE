#!/usr/bin/env python3
"""
jar2zip.py -- build the DoomRPG.zip data archive expected by DoomRPG-RE from the
original J2ME DoomRPG.jar.

The J2ME jar already contains the core game data under the exact names the C code
reads (entities.db, *.bin texel/palette/shape blobs, *.bsp maps). The differences
this script reconciles:
  * images are PNG in the jar, but the code loads BMP (SDL_LoadBMP) -> convert to
    8-bit paletted BMP, compositing transparency onto the magenta color-key the
    engine uses (255,0,255).
  * the engine loads a few UI images by fixed name (p/q/j.bmp) and particle gib
    sheets (gibs_16/24.bmp). The gib sheets are not in the jar -> generate
    transparent stubs.
  * the engine needs a menu backdrop map menu.bsp, absent from the jar -> reuse
    intro.bsp.
  * audio: the engine preloads ~95 sounds named by numeric resource id
    (NNN.wav / NNN.mid). The jar's audio is named differently and is MIDI-only.
    The 3 music tracks (resource ids 5039/5040/5043, the largest jar MIDIs) are
    mapped through; the remaining SFX are emitted as short silent WAVs so the
    game boots (SFX can be authored later).

Usage: jar2zip.py <DoomRPG.jar> <out/DoomRPG.zip>
"""
import io
import os
import sys
import wave
import shutil
import zipfile
import tempfile

from PIL import Image

MAGENTA = (255, 0, 255)

# Sound.c soundTable[] (MAX_AUDIOFILES = 95). Indices 0, 1, 3 are MIDI.
SOUND_TABLE = [
    5039, 5040, 5042, 5043, 5044, 5045, 5046, 5047, 5048, 5049, 5050,
    5051, 5052, 5053, 5054, 5055, 5057, 5058, 5059, 5060, 5061, 5062,
    5063, 5064, 5065, 5066, 5067, 5068, 5069, 5070, 5071, 5072, 5073,
    5074, 5076, 5077, 5078, 5079, 5080, 5081, 5082, 5083, 5084, 5085,
    5086, 5087, 5088, 5089, 5090, 5091, 5092, 5093, 5094, 5095, 5096,
    5097, 5098, 5099, 5100, 5101, 5102, 5103, 5104, 5105, 5106, 5107,
    5108, 5109, 5110, 5111, 5112, 5113, 5114, 5115, 5116, 5117, 5118,
    5119, 5120, 5121, 5122, 5123, 5124, 5125, 5126, 5127, 5128, 5129,
    5130, 5131, 5133, 5134, 5136, 5137, 5138,
]
MIDI_INDICES = {0, 1, 3}

# The engine loads 15 letter-named images (a-g, j-q) that correspond 1:1 to the
# jar's letter-named PNGs (a.png font, b cursor, c..f space intro, g legals,
# j logo, k status bar, l hud faces, m icon sheet, n/o arrows, p hand, q arrows).
# Every jar PNG is converted to <name>.bmp automatically.
#
# These names are GEC PC-port additions absent from the J2ME jar; substitute the
# closest original (low-res) image so the engine always has a valid file to load.
# At the default 320x240 resolution the small originals are the ones drawn.
SUBSTITUTE = {
    "bar_lg.bmp": "k.png",                  # large status bar -> small status bar
    "larger_font.bmp": "a.png",             # large font -> small font
    "larger HUD faces.bmp": "l.png",        # (note the space in the name)
    "larger_HUD_icon_sheet.bmp": "m.png",
}


def png_to_bmp(png_path):
    """PNG (8-bit colormap) -> 8-bit paletted BMP bytes, transparency -> magenta."""
    im = Image.open(png_path)
    if "transparency" in im.info or im.mode in ("RGBA", "LA", "PA"):
        rgba = im.convert("RGBA")
        bg = Image.new("RGBA", rgba.size, MAGENTA + (255,))
        bg.paste(rgba, (0, 0), rgba)
        im = bg.convert("RGB").quantize(colors=256)
    elif im.mode != "P":
        im = im.convert("P", palette=Image.ADAPTIVE, colors=256)
    buf = io.BytesIO()
    im.save(buf, format="BMP")
    return buf.getvalue()


def stub_bmp(w, h):
    """A transparent (all magenta) 8-bit paletted BMP of the given size."""
    im = Image.new("P", (w, h), 0)
    pal = list(MAGENTA)
    pal += [0, 0, 0] * 255
    im.putpalette(pal)
    buf = io.BytesIO()
    im.save(buf, format="BMP")
    return buf.getvalue()


def silent_wav():
    """A short valid silent 16-bit PCM WAV."""
    buf = io.BytesIO()
    w = wave.open(buf, "wb")
    w.setnchannels(1)
    w.setsampwidth(2)
    w.setframerate(11025)
    w.writeframes(b"\x00\x00" * 1024)
    w.close()
    return buf.getvalue()


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)
    jar_path, out_path = sys.argv[1], sys.argv[2]

    tmp = tempfile.mkdtemp(prefix="jar2zip_")
    try:
        with zipfile.ZipFile(jar_path) as jz:
            jz.extractall(tmp)

        def jar(name):
            return os.path.join(tmp, name)

        entries = {}  # archive name -> bytes

        # 1. Core binary data + maps, copied verbatim.
        verbatim = ["entities.db", "bitshapes.bin", "mappings.bin", "palettes.bin",
                    "sintable.bin", "stexels.bin", "wtexels.bin"]
        for f in os.listdir(tmp):
            if f.endswith(".bsp") or f in verbatim:
                with open(jar(f), "rb") as fh:
                    entries[f] = fh.read()

        # menu backdrop map (a GEC PC-port addition, absent from the J2ME jar)
        # -> reuse the smallest available map as a stand-in backdrop.
        if "menu.bsp" not in entries:
            cand = ["items.bsp", "reactor.bsp", "junction.bsp", "intro.bsp"]
            for c in cand:
                if c in entries:
                    entries["menu.bsp"] = entries[c]
                    print("menu.bsp <- %s (substitute backdrop)" % c)
                    break

        # 2. Every jar PNG -> 8-bit paletted BMP (a.png -> a.bmp, ...).
        for f in os.listdir(tmp):
            if f.endswith(".png"):
                entries[f[:-4] + ".bmp"] = png_to_bmp(jar(f))

        # GEC PC-port image additions absent from the jar -> closest original.
        for dst, src in SUBSTITUTE.items():
            if os.path.exists(jar(src)):
                entries[dst] = png_to_bmp(jar(src))

        # particle gib sheets (not in the jar) -> transparent stubs.
        entries["gibs_16.bmp"] = stub_bmp(256, 16)
        entries["gibs_24.bmp"] = stub_bmp(256, 24)

        # 3. Audio. Map the 3 largest jar MIDIs to the 3 music resource ids;
        #    emit silent WAVs for the rest.
        jar_mids = sorted(
            [f for f in os.listdir(tmp) if f.endswith(".mid")],
            key=lambda f: os.path.getsize(jar(f)), reverse=True,
        )
        music_ids = [SOUND_TABLE[i] for i in sorted(MIDI_INDICES)]  # 5039,5040,5043
        sil = silent_wav()
        for idx, rid in enumerate(SOUND_TABLE):
            if idx in MIDI_INDICES:
                slot = sorted(MIDI_INDICES).index(idx)
                src = jar_mids[slot] if slot < len(jar_mids) else None
                if src:
                    with open(jar(src), "rb") as fh:
                        entries["%03d.mid" % rid] = fh.read()
                else:
                    entries["%03d.mid" % rid] = sil  # fallback (won't loop nicely)
            else:
                entries["%03d.wav" % rid] = sil

        # 4. Write the archive (deflate; the engine handles store + deflate).
        os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
        with zipfile.ZipFile(out_path, "w", zipfile.ZIP_DEFLATED) as oz:
            for name in sorted(entries):
                oz.writestr(name, entries[name])

        bsp = sum(1 for n in entries if n.endswith(".bsp"))
        wav = sum(1 for n in entries if n.endswith(".wav"))
        mid = sum(1 for n in entries if n.endswith(".mid"))
        bmp = sum(1 for n in entries if n.endswith(".bmp"))
        print("Wrote %s: %d entries (%d bsp, %d bmp, %d wav, %d mid, music=%s)" %
              (out_path, len(entries), bsp, bmp, wav, mid, music_ids))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    main()
