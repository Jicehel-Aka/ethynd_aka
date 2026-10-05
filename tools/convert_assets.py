#!/usr/bin/env python3
"""Convertisseur d'assets Ethynd -> formats binaires pour la Gamebuino AKA.

Usage :
    python3 convert_assets.py <dossier Ethynd> <dossier sortie> [--tile 16|32]
                              [--audio] [--sfx-format wav|mp3]

--tile = taille d'AFFICHAGE des tuiles et sprites (16, 24 ou 32 px).
  32 : echelle 1, champ de vision 10x7,5 tuiles sur 320x240
  24 : echelle 0,75, champ de vision 13,3x10 tuiles (240 = 10 x 24)
  16 : echelle 0,5, champ de vision 20x15 tuiles (comme l'original)
La logique de jeu (collisions, vitesses, hitboxes) reste en unites de 32 px,
comme dans Ethynd : les masques de collision sont toujours calcules sur la tuile
32x32 d'origine ; seul l'affichage est mis a l'echelle (position * tile / 32).

Sorties (little-endian, pixels 16 bits, couleur cle 0xF81F = transparent ; ordre BGR565
natif de l'AKA par defaut, --order rgb pour du RGB565 standard) :
    tiles.bin       en-tete + N tuiles compactees (seulement celles utilisees)
    collide.bin     un masque de collision 1 bit/pixel par tuile (pixel-perfect)
    anim.bin        table tuile -> tuile suivante (animations de tuiles)
    maps/<nom>.map  en-tete + 4 couches uint16 (0xFFFF = vide)
    sprites.bin     tous les sprites + index
    menu/<nom>.raw  ecrans 320x240 RGB565
    manifest.json   index lisible (noms de sprites, tailles, etc.)
    audio/          musiques en mp3 (mono), effets en wav ou mp3 (avec --audio)
"""
import argparse
import glob
import json
import os
import struct
import subprocess
import sys
import types
import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)

from PIL import Image

KEY = 0xF81F          # RGB565 magenta = transparent
EMPTY = 0xFFFF        # tuile vide dans les cartes
ALPHA_MIN = 128       # seuil alpha (identique a pygame.mask.from_surface : > 127)


def load_constants(src):
    """Importe constantes_tuiles.py sans avoir besoin de pygame."""
    if "pygame" not in sys.modules:
        sys.modules["pygame"] = types.ModuleType("pygame")
    sys.path.insert(0, src)
    from constantes import constantes_tuiles as ct  # noqa
    return set(ct.collisions), dict(ct.animations)


ORDER = "bgr"   # "bgr" = ordre natif du framebuffer AKA ; "rgb" = RGB565 standard (tests PC / SDL)


def rgb565(r, g, b):
    """RGB565 avec arrondi au plus proche (et non troncature : evite l'assombrissement).

    gb_graphics (lcd_color_rgb) range le ROUGE dans les bits BAS et le BLEU dans les
    bits HAUTS : l'ordre natif de l'AKA est donc BGR565, pas le RGB565 standard.
    """
    r5 = (r * 31 + 127) // 255
    g6 = (g * 63 + 127) // 255
    b5 = (b * 31 + 127) // 255
    if ORDER == "bgr":
        return r5 | (g6 << 5) | (b5 << 11)
    return (r5 << 11) | (g6 << 5) | b5


def scale(img, factor):
    if factor == 1:
        return img
    w, h = img.size
    return img.resize((max(1, round(w * factor)), max(1, round(h * factor))),
                      Image.BOX)


def to_565(img):
    """RGBA -> octets RGB565 ; alpha < seuil -> couleur cle."""
    img = img.convert("RGBA")
    out = bytearray()
    for r, g, b, a in img.getdata():
        px = KEY if a < ALPHA_MIN else rgb565(r, g, b)
        if a >= ALPHA_MIN and px == KEY:   # evite un vrai pixel = cle
            px = KEY ^ 0x0001
        out += struct.pack("<H", px)
    return bytes(out)


def alpha_mask_bits(img):
    """Masque 1 bit/pixel (ligne par ligne, poids fort d'abord)."""
    img = img.convert("RGBA")
    w, h = img.size
    px = img.load()
    bits = bytearray()
    for y in range(h):
        row = 0
        n = 0
        for x in range(w):
            row = (row << 1) | (1 if px[x, y][3] >= ALPHA_MIN else 0)
            n += 1
            if n == 8:
                bits.append(row)
                row, n = 0, 0
        if n:
            bits.append(row << (8 - n))
    return bytes(bits)


def read_csv(path):
    with open(path) as f:
        return [[int(v) for v in l.strip().split(",")] for l in f if l.strip()]


def convert_tiles_and_maps(src, dst, tile, collisions, animations):
    factor = tile / 32
    maps = {}
    used = set()
    for path in sorted(glob.glob(os.path.join(src, "maps", "*_0.csv"))):
        name = os.path.basename(path)[:-6]
        layers = [read_csv(os.path.join(src, "maps", f"{name}_{i}.csv"))
                  for i in range(4)]
        maps[name] = layers
        for layer in layers:
            for row in layer:
                used.update(v for v in row if v >= 0)

    # fermeture sur les animations (une tuile animee peut mener a d'autres)
    todo = list(used)
    while todo:
        t = todo.pop()
        nxt = animations.get(str(t))
        if nxt is not None and int(nxt) not in used:
            used.add(int(nxt))
            todo.append(int(nxt))

    ids = sorted(used)
    remap = {old: new for new, old in enumerate(ids)}

    tileset = Image.open(os.path.join(src, "images", "tuiles", "tileset.png"))
    tileset = tileset.convert("RGBA")
    cols = tileset.size[0] // 32

    tiles_bin = bytearray()
    collide_bin = bytearray()
    for old in ids:
        x, y = (old % cols) * 32, (old // cols) * 32
        full = tileset.crop((x, y, x + 32, y + 32))
        tiles_bin += to_565(scale(full, factor))          # image d'affichage
        if str(old) in collisions:
            collide_bin += alpha_mask_bits(full)          # masque en 32x32 (logique)
        else:
            collide_bin += bytes(128)                     # tuile sans collision
    mask_bytes = 128

    with open(os.path.join(dst, "tiles.bin"), "wb") as f:
        f.write(b"ETIL" + struct.pack("<HHI", tile, len(ids), 0))
        f.write(tiles_bin)
    with open(os.path.join(dst, "collide.bin"), "wb") as f:
        f.write(b"ECOL" + struct.pack("<HHI", 32, len(ids), mask_bytes))
        f.write(collide_bin)

    anim = [EMPTY] * len(ids)
    for a, b in animations.items():
        a, b = int(a), int(b)
        if a in remap and b in remap:
            anim[remap[a]] = remap[b]
    with open(os.path.join(dst, "anim.bin"), "wb") as f:
        f.write(b"EANI" + struct.pack("<I", len(anim)))
        f.write(struct.pack(f"<{len(anim)}H", *anim))

    os.makedirs(os.path.join(dst, "maps"), exist_ok=True)
    info = {}
    for name, layers in maps.items():
        h, w = len(layers[0]), len(layers[0][0])
        with open(os.path.join(dst, "maps", name + ".map"), "wb") as f:
            f.write(b"EMAP" + struct.pack("<HHHH", w, h, 4, tile))
            for layer in layers:
                for row in layer:
                    f.write(struct.pack(
                        f"<{w}H", *[EMPTY if v < 0 else remap[v] for v in row]))
        info[name] = {"w": w, "h": h}
    return {"tiles_used": len(ids), "tile_px": tile, "mask_bytes": mask_bytes,
            "maps": info}


def convert_sprites(src, dst, tile):
    factor = tile / 32
    files = sorted(glob.glob(os.path.join(src, "images", "sprites", "*.png")))
    files += sorted(glob.glob(os.path.join(src, "images", "objets", "*.png")))
    index, blob = [], bytearray()
    for p in files:
        name = os.path.splitext(os.path.basename(p))[0]
        img = scale(Image.open(p), factor)
        data = to_565(img)
        index.append({"name": name, "w": img.size[0], "h": img.size[1],
                      "offset": len(blob)})
        blob += data
    with open(os.path.join(dst, "sprites.bin"), "wb") as f:
        f.write(b"ESPR" + struct.pack("<I", len(index)))
        for e in index:   # nom 24 octets + w + h + offset
            f.write(e["name"].encode()[:23].ljust(24, b"\0"))
            f.write(struct.pack("<HHI", e["w"], e["h"], e["offset"]))
        f.write(blob)
    return {"sprites": len(index), "sprite_index": index}


BG = (47, 47, 47)       # fond des ecrans d'origine
PANEL = (79, 79, 79)    # boutons et panneau de l'aide


def patch_menu(name, im):
    """Efface les textes qui parlent des touches du PC (J, a, q, Echap, X) : le jeu les
    redessine a l'execution (traduits, adaptes a la console). Coordonnees en 640x480."""
    from PIL import ImageDraw
    d = ImageDraw.Draw(im)
    if name == "menu":
        d.rectangle((50, 305, 286, 328), fill=PANEL)       # "(appuyez sur j)"
        d.rectangle((353, 305, 589, 328), fill=PANEL)      # "(appuyez sur a)"
        d.rectangle((200, 376, 440, 462), fill=BG)         # bouton "Quitter" (pas de quitter sur AKA)
    elif name == "aide":
        d.rectangle((53, 295, 360, 449), fill=PANEL)       # "X : Attaquer", "Echap : Quitter"
        d.rectangle((0, 452, 640, 478), fill=BG)           # "Appuyez sur Echap pour revenir au menu"
    elif name == "mort":
        d.rectangle((110, 398, 530, 428), fill=BG)         # "Appuyez sur Echap pour quitter"
    return im


def convert_menus(src, dst):
    os.makedirs(os.path.join(dst, "menu"), exist_ok=True)
    n = 0
    for p in sorted(glob.glob(os.path.join(src, "images", "menu", "*.png"))):
        name = os.path.splitext(os.path.basename(p))[0]
        img = patch_menu(name, Image.open(p).convert("RGB")).resize((320, 240), Image.BOX)
        raw = bytearray()
        for r, g, b in img.getdata():
            raw += struct.pack("<H", rgb565(r, g, b))
        name = os.path.splitext(os.path.basename(p))[0]
        with open(os.path.join(dst, "menu", name + ".raw"), "wb") as f:
            f.write(raw)
        n += 1
    return {"menus": n}


# marche.ogg d'origine : 52 s de pas (~4,6 Mo en WAV 44,1 kHz) rejoues du debut a chaque fois.
# On n'en garde que deux pas reguliers (0,70 s chacun) coupes aux creux de silence entre
# deux pas : le son se rejoue en boucle sans a-coup (le jeu relance le son quand il se termine).
# Choisis par analyse des pas (detection de transitoires, meilleur segment parmi 70 pas).
WALK_LOOP = (4.33, 1.40)      # (debut en s, duree en s)
FADE = 0.008                  # fondu d'entree/sortie de 8 ms contre les clics


def ffmpeg(args):
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error"] + args, check=True)


def convert_audio(src, dst, sfx_format):
    out = os.path.join(dst, "audio")
    os.makedirs(out, exist_ok=True)
    done = []
    # musiques : mp3 mono 22,05 kHz 48 kbit/s
    for p in sorted(glob.glob(os.path.join(src, "son", "*.ogg"))):
        name = os.path.splitext(os.path.basename(p))[0]
        short = os.path.getsize(p) < 64 * 1024     # petit son d'interface
        target = os.path.join(out, name + (".wav" if short and sfx_format == "wav" else ".mp3"))
        convert_one(p, target)
        done.append(os.path.basename(target))
    # effets
    for p in sorted(glob.glob(os.path.join(src, "son", "*", "*.ogg"))):
        name = os.path.splitext(os.path.basename(p))[0]
        sub = os.path.basename(os.path.dirname(p))
        target = os.path.join(out, f"{sub}_{name}.{sfx_format}")
        convert_one(p, target, WALK_LOOP if (sub, name) == ("joueur", "marche") else None)
        done.append(os.path.basename(target))
    return {"audio": done}


def convert_one(p, target, trim=None):
    """trim = (debut, duree) en secondes : extrait + fondus de FADE."""
    t_in, t_af = [], []
    if trim:
        # -ss/-t AVANT -i : decalage precis et horloge remise a zero (les fondus en dependent)
        start, dur = trim
        t_in = ["-ss", str(start), "-t", str(dur)]
        t_af = ["-af", "afade=t=in:d=%g,afade=t=out:st=%g:d=%g" % (FADE, dur - FADE, FADE)]
    if target.endswith(".mp3"):
        # 22,05 kHz mono : la piste MP3 reechantillonne vers GB_AUDIO_SAMPLE_RATE
        ffmpeg(t_in + ["-i", p] + t_af + ["-ac", "1", "-ar", "22050",
                "-c:a", "libmp3lame", "-b:a", "48k", target])
    else:
        # gb_audio_track_wav : mono, 16 bits, 44100 Hz (frequence du mixeur, pas
        # de reechantillonnage) et chunk 'data' juste apres 'fmt ' (44 octets
        # d'en-tete). ffmpeg ajoute un chunk LIST : on ecrit donc l'en-tete.
        rate = 44100
        pcm = subprocess.run(
            ["ffmpeg", "-y", "-loglevel", "error"] + t_in + ["-i", p] + t_af +
            ["-ac", "1", "-ar", str(rate), "-f", "s16le", "-"],
            check=True, capture_output=True).stdout
        with open(target, "wb") as f:
            f.write(b"RIFF" + struct.pack("<I", 36 + len(pcm)) + b"WAVE")
            f.write(b"fmt " + struct.pack("<IHHIIHH", 16, 1, 1, rate,
                                          rate * 2, 2, 16))
            f.write(b"data" + struct.pack("<I", len(pcm)))
            f.write(pcm)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("--tile", type=int, choices=(16, 24, 32), default=24)
    ap.add_argument("--audio", action="store_true")
    ap.add_argument("--sfx-format", choices=("wav", "mp3"), default="wav")
    ap.add_argument("--order", choices=("bgr", "rgb"), default="bgr",
                    help="ordre des bits de couleur : bgr = natif AKA (defaut), rgb = standard")
    a = ap.parse_args()
    global ORDER
    ORDER = a.order
    os.makedirs(a.dst, exist_ok=True)

    coll, anim = load_constants(a.src)
    manifest = {}
    manifest.update(convert_tiles_and_maps(a.src, a.dst, a.tile, coll, anim))
    manifest.update(convert_sprites(a.src, a.dst, a.tile))
    manifest.update(convert_menus(a.src, a.dst))
    if a.audio:
        manifest.update(convert_audio(a.src, a.dst, a.sfx_format))
    with open(os.path.join(a.dst, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=1)
    print("OK :", {k: v for k, v in manifest.items()
                   if k not in ("sprite_index", "audio", "maps")})


if __name__ == "__main__":
    main()
