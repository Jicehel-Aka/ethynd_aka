#!/usr/bin/env python3
"""Convertisseur d'assets Ethynd -> formats binaires pour la Gamebuino AKA.

Usage :
    python3 convert_assets.py <dossier Ethynd d'origine> <dossier sortie> [--world <dossier>]
                              [--tile 16|24|32] [--audio] [--sfx-format wav|mp3]
                              [--editor-out <dossier>]

Le dossier d'origine fournit les images, les sons et les constantes d'animation ; les CARTES,
les portes, les monstres, les PNJ, les objets et l'histoire viennent du dossier "world"
(format texte, voir world_io.py ; par defaut ../world).

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
    maps/<nom>.map  "EMA2" : 4 couches uint16 (0xFFFF = vide) + monstres, PNJ, portes, objets
    world.bin       "EWLD" : depart, cartes, drapeaux, objets, dialogues, objectifs
    sprites.bin     sprites, variantes de PNJ (sage, gardien), icones d'objets + index
    menu/<nom>.raw  ecrans 320x240 RGB565
    manifest.json   index lisible (noms de sprites, tailles, etc.)
    audio/          musiques en mp3 (mono), effets en wav ou mp3 (avec --audio)
"""
import argparse
import glob
import json
import os
import struct
import shutil
import subprocess
import sys
import types
import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import world_io as wio

from PIL import Image

KEY = 0xF81F          # RGB565 magenta = transparent
EMPTY = 0xFFFF        # tuile vide dans les cartes
ALPHA_MIN = 128       # seuil alpha (identique a pygame.mask.from_surface : > 127)


def load_python_constants(src):
    """Importe les constantes d'Ethynd sans avoir besoin de pygame."""
    pg = types.ModuleType("pygame")
    for k in ("K_UP", "K_LEFT", "K_DOWN", "K_RIGHT", "K_x"):
        setattr(pg, k, k)
    sys.modules["pygame"] = pg
    sys.path.insert(0, src)
    from constantes import constantes_entite as ce  # noqa
    from constantes import constantes_joueur as cj  # noqa
    return ce, cj


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



# ------------------------------------------------------------------ binaire : petits outils
def u8(v):  return struct.pack("<B", v)
def u16(v): return struct.pack("<H", v)
def s16(v): return struct.pack("<h", v)
def sstr(s):            # chaine courte : u8 longueur + octets UTF-8
    b = s.encode("utf-8")
    assert len(b) < 256, s
    return u8(len(b)) + b
def lstr(s):            # chaine longue : u16 longueur + octets UTF-8
    b = s.encode("utf-8")
    return u16(len(b)) + b


def flag_index(story, name):
    return story["flags"].index(name)


def parse_rules(rules, story):
    """'has:crystal?sage_thanks;talked_sage?sage_wait;sage_intro' -> [(kind, arg, dialogue)]."""
    dl = [d["id"] for d in story["dialogues"]]
    out = []
    if rules == "-":
        return out
    for rule in rules.split(";"):
        cond, _, dlg = rule.rpartition("?")
        if not cond:
            out.append((0, 0, dl.index(dlg)))
        elif cond.startswith("has:"):
            out.append((3, [i["id"] for i in story["items"]].index(cond[4:]), dl.index(dlg)))
        elif cond.startswith("!"):
            out.append((2, flag_index(story, cond[1:]), dl.index(dlg)))
        else:
            out.append((1, flag_index(story, cond), dl.index(dlg)))
    return out


def write_map_bin(path, m, world, story, tile, remap):
    names = world["maps"]
    item_ids = [i["id"] for i in story["items"]]
    b = bytearray(b"EMA2")
    b += struct.pack("<HHHH", m["w"], m["h"], 4, tile)
    b += u8(wio.MUSICS.index(m["music"])) + u8(0)
    for layer in m["layers"]:
        for row in layer:
            for v in row:
                b += u16(EMPTY if v < 0 else remap[v])
    b += u16(len(m["spawns"]))
    for s in m["spawns"]:
        b += sstr(s["type"]) + s16(s["x"]) + s16(s["y"]) + s16(s["vie"]) + s16(s["attaque"])
    b += u16(len(m["npcs"]))
    for s in m["npcs"]:
        rules = parse_rules(s["rules"], story)
        b += sstr(s["type"]) + s16(s["x"]) + s16(s["y"]) + u8(wio.DIRS.index(s["dir"])) + u8(len(rules))
        for kind, arg, dlg in rules:
            b += u8(kind) + s16(arg) + s16(dlg)
    b += u16(len(m["doors"]))
    for d in m["doors"]:
        req = -1 if d["requires"] == "-" else flag_index(story, d["requires"])
        deny = -1 if d["deny"] == "-" else [x["id"] for x in story["dialogues"]].index(d["deny"])
        b += s16(d["x"]) + s16(d["y"]) + s16(d["w"]) + s16(d["h"]) + u8(names.index(d["map"]))
        b += s16(d["dx"]) + s16(d["dy"]) + s16(req) + s16(deny)
    b += u16(len(m["items"]))
    for s in m["items"]:
        b += u8(item_ids.index(s["id"])) + s16(s["x"]) + s16(s["y"])
    with open(path, "wb") as f:
        f.write(b)


def write_world_bin(path, world, story):
    names = world["maps"]
    sm, sx, sy, sd = world["start"]
    b = bytearray(b"EWLD") + u16(1)
    b += u8(names.index(sm)) + s16(sx) + s16(sy) + u8(wio.DIRS.index(sd))
    b += u8(len(names))
    for n in names:
        b += sstr(n)
    b += u8(len(story["flags"]))
    b += u8(len(story["items"]))
    for it in story["items"]:
        b += sstr(it["icon"]) + u8(it["heal"]) + sstr(it["name"])
    item_ids = [i["id"] for i in story["items"]]
    b += u16(len(story["dialogues"]))
    for d in story["dialogues"]:
        b += u8(len(d["lines"]))
        for ln in d["lines"]:
            b += sstr(ln["speaker"] if ln["speaker"] != "-" else "") + sstr(ln["text"])
        acts = []
        for kind, args in d["actions"]:
            if kind == "set":
                acts.append((1, flag_index(story, args[0]), 0))
            elif kind == "give":
                acts.append((2, item_ids.index(args[0]), int(args[1]) if len(args) > 1 else 1))
            elif kind == "take":
                acts.append((3, item_ids.index(args[0]), int(args[1]) if len(args) > 1 else 1))
            elif kind == "win":
                acts.append((4, 0, 0))
            elif kind == "heal":
                acts.append((5, int(args[0]), 0))
        b += u8(len(acts))
        for t, a, c in acts:
            b += u8(t) + s16(a) + s16(c)
    objs = [o["id"] for o in story["objectives"]]
    b += u8(len(objs))
    for o in story["objectives"]:
        c = o["cond"].split(":")
        typ, s, p = 0, "", [0, 0, 0, 0]
        if c[0] == "flag":
            typ, p[0] = 0, flag_index(story, c[1])
        elif c[0] == "kill":
            typ, s, p[0] = 1, c[1], int(c[2])
        elif c[0] == "item":
            typ, p[0], p[1] = 2, item_ids.index(c[1]), int(c[2])
        elif c[0] == "map":
            typ, p[0] = 3, names.index(c[1])
        elif c[0] == "reach":
            typ, p = 4, [names.index(c[1]), int(c[2]), int(c[3]), int(c[4])]
        after = -1 if o["after"] == "-" else objs.index(o["after"])
        b += sstr(o["title"]) + struct.pack("<b", after) + u8(typ) + sstr(s)
        b += struct.pack("<hhhh", *p)
    with open(path, "wb") as f:
        f.write(b)


# ------------------------------------------------------------------ langues
def write_langs(dst, langs):
    """lang/<code>.bin : "ELNG", u16 n, n * (sstr cle, lstr texte), sans les cles 'aka.'.
    lang/<code>.json : cles 'aka.XXX' -> {"XXX": texte}, pour le menu systeme aka_runtime."""
    out = os.path.join(dst, "lang")
    os.makedirs(out, exist_ok=True)
    ref = langs[wio.DEFAULT_LANG]
    for code, d in langs.items():
        game_keys = sorted(k for k in d if not k.startswith(("aka.", "ed.")))
        b = bytearray(b"ELNG") + u16(len(game_keys))
        for k in game_keys:
            b += sstr(k) + lstr(d[k])
        with open(os.path.join(out, code + ".bin"), "wb") as f:
            f.write(b)
        aka = {k[4:]: v for k, v in d.items() if k.startswith("aka.")}
        for k, v in ref.items():       # menu systeme : repli sur le francais si cle absente
            if k.startswith("aka.") and k[4:] not in aka:
                aka[k[4:]] = v
        with open(os.path.join(out, code + ".json"), "w", encoding="utf-8") as f:
            json.dump(aka, f, ensure_ascii=False, indent=1)
        missing = [k for k in ref if k not in d]
        if missing:
            print("  attention : %s.txt n'a pas %d cle(s) (repli sur fr) : %s%s" % (
                code, len(missing), ", ".join(missing[:4]), "..." if len(missing) > 4 else ""))


# ------------------------------------------------------------------ tuiles, cartes, monde
def convert_world(src, dst, world_dir, tile, ce, editor_out):
    factor = tile / 32
    world = wio.parse_world(os.path.join(world_dir, "world.txt"))
    collisions, animations = wio.parse_tiles(os.path.join(world_dir, "tiles.txt"))
    story = wio.parse_story(os.path.join(world_dir, "story.txt"))
    langs = wio.load_langs(os.path.join(world_dir, "lang"))
    maps = {n: wio.parse_map(os.path.join(world_dir, "maps", n + ".txt")) for n in world["maps"]}
    monsters = list(ce.animation["monstre"].keys())
    errs = wio.validate(world, maps, story, langs=langs, entity_names=monsters)
    if errs:
        raise SystemExit("Erreurs dans le monde :\n  " + "\n  ".join(errs))

    used = set()
    for m in maps.values():
        for layer in m["layers"]:
            for row in layer:
                used.update(v for v in row if v >= 0)
    todo = list(used)
    while todo:                      # une tuile animee mene a d'autres tuiles
        t = todo.pop()
        nxt = animations.get(t)
        if nxt is not None and nxt not in used:
            used.add(nxt)
            todo.append(nxt)
    ids = sorted(used)
    remap = {old: new for new, old in enumerate(ids)}

    tileset = Image.open(os.path.join(src, "images", "tuiles", "tileset.png")).convert("RGBA")
    cols = tileset.size[0] // 32

    def crop(old):
        x, y = (old % cols) * 32, (old // cols) * 32
        return tileset.crop((x, y, x + 32, y + 32))

    tiles_bin, collide_bin = bytearray(), bytearray()
    for old in ids:
        full = crop(old)
        tiles_bin += to_565(scale(full, factor))
        collide_bin += alpha_mask_bits(full) if old in collisions else bytes(128)   # masque 32x32 (logique)
    with open(os.path.join(dst, "tiles.bin"), "wb") as f:
        f.write(b"ETIL" + struct.pack("<HHI", tile, len(ids), 0) + tiles_bin)
    with open(os.path.join(dst, "collide.bin"), "wb") as f:
        f.write(b"ECOL" + struct.pack("<HHI", 32, len(ids), 128) + collide_bin)
    anim = [EMPTY] * len(ids)
    for a, b in animations.items():
        if a in remap and b in remap:
            anim[remap[a]] = remap[b]
    with open(os.path.join(dst, "anim.bin"), "wb") as f:
        f.write(b"EANI" + struct.pack("<I", len(anim)) + struct.pack("<%dH" % len(anim), *anim))

    os.makedirs(os.path.join(dst, "maps"), exist_ok=True)
    for name, m in maps.items():
        write_map_bin(os.path.join(dst, "maps", name + ".map"), m, world, story, tile, remap)
    write_world_bin(os.path.join(dst, "world.bin"), world, story)
    write_langs(dst, langs)

    if editor_out:                   # palette complete du tileset pour l'editeur (PC uniquement)
        os.makedirs(editor_out, exist_ok=True)
        count = cols * (tileset.size[1] // 32)
        blob = bytearray()
        for old in range(count):
            blob += to_565(scale(crop(old), factor))
        with open(os.path.join(editor_out, "tileset_full.bin"), "wb") as f:
            f.write(b"ETSF" + struct.pack("<HHH", tile, count, cols) + blob)
    return {"tiles_used": len(ids), "tile_px": tile, "maps": {n: {"w": m["w"], "h": m["h"]} for n, m in maps.items()},
            "flags": len(story["flags"]), "dialogues": len(story["dialogues"]),
            "objectives": len(story["objectives"])}


# ------------------------------------------------------------------ sprites
# Variantes de PNJ : echange de palette du personnage (14 couleurs seulement dans l'original)
NPC_VARIANTS = {
    "sage": {(67, 46, 39): (235, 235, 240), (106, 72, 52): (190, 190, 200),
             (196, 60, 60): (70, 160, 110), (136, 46, 46): (40, 110, 75), (104, 28, 28): (25, 75, 50),
             (101, 101, 155): (140, 110, 70)},
    "gardien": {(196, 60, 60): (70, 100, 180), (136, 46, 46): (45, 70, 130), (104, 28, 28): (30, 45, 95),
                (101, 101, 155): (90, 90, 90)},
}

# Icones d'objets dessinees ici (16x16, agrandies x2) : '.' = transparent
ICON_PALETTE = {"K": (23, 23, 23), "G": (200, 220, 235), "R": (196, 60, 60), "r": (136, 46, 46),
                "W": (255, 255, 255), "C": (106, 72, 52), "B": (120, 200, 255), "b": (60, 140, 220),
                "d": (30, 80, 160)}
ICONS = {
    "item_potion": [
        "................", "......KKKK......", "......KCCK......", "......KCCK......",
        ".....KKGGKK.....", "......KGGK......", ".....KGGGGK.....", "....KGRRRRGK....",
        "...KGRRWRRRrK...", "...KRRWRRRRrK...", "...KRRRRRRRrK...", "...KRRRRRRrrK...",
        "...KrRRRRrrrK...", "....KrrrrrrK....", ".....KKKKKK.....", "................"],
    "item_crystal": [
        "................", ".......KK.......", "......KBBK......", ".....KBWBbK.....",
        "....KBWBBbbK....", "...KBWBBBbbdK...", "...KBBBBBbbdK...", "...KBBBBBbbdK...",
        "...KbBBBBbbdK...", "....KbBBbbdK....", ".....KbBbdK.....", "......KbdK......",
        ".......KK.......", "................", "................", "................"],
}


def make_icon(rows):
    assert len(rows) == 16 and all(len(r) == 16 for r in rows)
    img = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c != ".":
                img.putpixel((x, y), ICON_PALETTE[c] + (255,))
    return img.resize((32, 32), Image.NEAREST)


def swap_palette(img, table):
    img = img.convert("RGBA")
    px = img.load()
    for y in range(img.size[1]):
        for x in range(img.size[0]):
            r, g, b, a = px[x, y]
            if a >= ALPHA_MIN and (r, g, b) in table:
                px[x, y] = table[(r, g, b)] + (a,)
    return img


def convert_sprites(src, dst, tile, cj):
    factor = tile / 32
    entries = []     # (nom, image PIL logique 32 px)
    for folder in ("sprites", "objets"):
        for p in sorted(glob.glob(os.path.join(src, "images", folder, "*.png"))):
            entries.append((os.path.splitext(os.path.basename(p))[0], Image.open(p)))
    # frames du personnage reprises par les PNJ : arret + marche (pas d'attaque)
    stems = set()
    for d in ("bas", "haut", "gauche", "droite"):
        for mov in ("base", "marche"):
            for pth in cj.animation.get(d, {}).get(mov, []):
                stems.add(os.path.splitext(os.path.basename(pth))[0])
    for variant, table in NPC_VARIANTS.items():
        for stem in sorted(stems):
            nn = stem.split("_")[1]
            entries.append(("%s_%s" % (variant, nn), swap_palette(
                Image.open(os.path.join(src, "images", "sprites", stem + ".png")), table)))
    for name, rows in ICONS.items():
        entries.append((name, make_icon(rows)))
    index, blob = [], bytearray()
    for name, img in entries:
        img = scale(img, factor)
        index.append({"name": name, "w": img.size[0], "h": img.size[1], "offset": len(blob)})
        blob += to_565(img)
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
    """Efface TOUT texte traduisible des images d'origine (francais incruste, touches du PC) :
    le jeu redessine les textes depuis lang/<code>.bin. Seuls restent les logos et les icones.
    Coordonnees en 640x480."""
    from PIL import ImageDraw
    BLK = (25, 25, 25)
    d = ImageDraw.Draw(im)
    if name == "menu":
        d.rectangle((140, 165, 500, 202), fill=BG)         # "Projet d'ISN : RPG"
        d.rectangle((52, 251, 284, 327), fill=PANEL)       # "Jouer" + "(appuyez sur j)"
        d.rectangle((355, 251, 587, 327), fill=PANEL)      # "Aide" + "(appuyez sur a)"
        d.rectangle((200, 376, 440, 462), fill=BG)         # bouton "Quitter" (pas de quitter sur AKA)
    elif name == "aide":
        d.rectangle((200, 20, 460, 125), fill=BG)          # titre "Aide"
        d.rectangle((245, 190, 585, 240), fill=PANEL)      # "Fleches : se deplacer"
        d.rectangle((53, 295, 360, 449), fill=PANEL)       # "X : Attaquer", "Echap : Quitter"
        d.rectangle((375, 278, 585, 310), fill=BLK)        # "Jeu cree par:" (les noms propres restent)
        d.rectangle((0, 452, 640, 478), fill=BG)           # "Appuyez sur Echap pour revenir au menu"
    elif name == "chargement":
        d.rectangle((0, 150, 640, 330), fill=BG)           # "Chargement..." (redessine, traduit)
    elif name in ("mort", "fin"):
        d.rectangle((10, 195, 630, 330), fill=BG)          # grand titre + sous-titre (le texte deborde jusqu'a x=28..603)
        d.rectangle((100, 398, 540, 428), fill=BG)         # "Appuyez..."
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
    ap.add_argument("src", help="dossier Ethynd d'origine (images, sons, constantes)")
    ap.add_argument("dst")
    ap.add_argument("--world", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "world"),
                    help="dossier du monde (cartes, histoire) ; defaut : ../world")
    ap.add_argument("--tile", type=int, choices=(16, 24, 32), default=24)
    ap.add_argument("--audio", action="store_true")
    ap.add_argument("--sfx-format", choices=("wav", "mp3"), default="wav")
    ap.add_argument("--editor-out", default=None, help="ecrit aussi la palette complete pour l'editeur")
    ap.add_argument("--order", choices=("bgr", "rgb"), default="bgr",
                    help="ordre des bits de couleur : bgr = natif AKA (defaut), rgb = standard")
    a = ap.parse_args()
    global ORDER
    ORDER = a.order
    os.makedirs(a.dst, exist_ok=True)

    ce, cj = load_python_constants(a.src)
    manifest = {}
    manifest.update(convert_world(a.src, a.dst, a.world, a.tile, ce, a.editor_out))
    manifest.update(convert_sprites(a.src, a.dst, a.tile, cj))
    if a.editor_out:
        shutil.copy(os.path.join(a.dst, "sprites.bin"), os.path.join(a.editor_out, "sprites.bin"))
    manifest.update(convert_menus(a.src, a.dst))
    if a.audio:
        manifest.update(convert_audio(a.src, a.dst, a.sfx_format))
    with open(os.path.join(a.dst, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=1)
    print("OK :", {k: v for k, v in manifest.items()
                   if k not in ("sprite_index", "audio", "maps")})


if __name__ == "__main__":
    main()
