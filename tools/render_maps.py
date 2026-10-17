#!/usr/bin/env python3
"""Genere des apercus PNG des cartes (tuiles + portes, monstres, PNJ, objets, depart) : pratique pour
relire une carte, la documenter ou verifier une modification sans lancer l'editeur.

Usage : python3 tools/render_maps.py <dossier Ethynd d'origine> [dossier sortie] [--world world] [--grid]
Sortie par defaut : docs/maps/<carte>.png (reduit a 1400 px de large au plus).
"""
import os
import sys

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import world_io as wio


def main():
    a = [x for x in sys.argv[1:] if not x.startswith("--")]
    if not a:
        sys.exit(__doc__)
    src = a[0]
    out = a[1] if len(a) > 1 else os.path.join(HERE, "..", "docs", "maps")
    world_dir = os.path.join(HERE, "..", "world")
    if "--world" in sys.argv:
        world_dir = sys.argv[sys.argv.index("--world") + 1]
    grid = "--grid" in sys.argv
    os.makedirs(out, exist_ok=True)
    world = wio.parse_world(os.path.join(world_dir, "world.txt"))
    collide, _ = wio.parse_tiles(os.path.join(world_dir, "tiles.txt"))
    tileset = Image.open(os.path.join(src, "images", "tuiles", "tileset.png")).convert("RGBA")
    cols = tileset.size[0] // 32
    for name in world["maps"]:
        m = wio.parse_map(os.path.join(world_dir, "maps", name + ".txt"))
        im = Image.new("RGBA", (m["w"] * 32, m["h"] * 32), (30, 30, 36, 255))
        for layer in m["layers"]:
            for y, row in enumerate(layer):
                for x, t in enumerate(row):
                    if t >= 0:
                        im.alpha_composite(tileset.crop(((t % cols) * 32, (t // cols) * 32, (t % cols) * 32 + 32, (t // cols) * 32 + 32)), (x * 32, y * 32))
        ov = Image.new("RGBA", im.size, (0, 0, 0, 0))
        d = ImageDraw.Draw(ov)
        if grid:
            for x in range(0, m["w"], 5):
                d.line((x * 32, 0, x * 32, im.size[1]), fill=(255, 255, 0, 70))
                d.text((x * 32 + 2, 2), str(x), fill=(255, 255, 0, 255))
            for y in range(0, m["h"], 5):
                d.line((0, y * 32, im.size[0], y * 32), fill=(255, 255, 0, 70))
                d.text((2, y * 32 + 2), str(y), fill=(255, 255, 0, 255))
        for s in m["doors"]:
            d.rectangle((s["x"], s["y"], s["x"] + s["w"], s["y"] + s["h"]), outline=(80, 220, 255, 255), fill=(80, 220, 255, 60), width=3)
            d.text((s["x"] + 2, s["y"] + s["h"] + 2), "-> " + s["map"], fill=(80, 220, 255, 255))
        for s in m["spawns"]:
            d.rectangle((s["x"], s["y"], s["x"] + 32, s["y"] + 32), outline=(255, 70, 70, 255), width=3)
            d.text((s["x"], s["y"] - 10), s["type"], fill=(255, 120, 120, 255))
        for s in m["npcs"]:
            d.rectangle((s["x"], s["y"], s["x"] + 32, s["y"] + 32), outline=(110, 230, 140, 255), width=3)
            d.text((s["x"], s["y"] - 10), s["id"], fill=(140, 255, 160, 255))
        for s in m["items"]:
            d.ellipse((s["x"] + 4, s["y"] + 4, s["x"] + 28, s["y"] + 28), outline=(255, 224, 130, 255), width=3)
            d.text((s["x"], s["y"] - 10), s["id"], fill=(255, 224, 130, 255))
        if world["start"][0] == name:
            x, y = world["start"][1], world["start"][2]
            d.line((x - 14, y, x + 14, y), fill=(110, 230, 140, 255), width=4)
            d.line((x, y - 14, x, y + 14), fill=(110, 230, 140, 255), width=4)
        im = Image.alpha_composite(im, ov).convert("RGB")
        if im.size[0] > 1400:
            im = im.resize((1400, round(im.size[1] * 1400 / im.size[0])), Image.LANCZOS)
        p = os.path.join(out, name + ".png")
        im.save(p, optimize=True)
        print("  %s (%dx%d) -> %s" % (name, m["w"], m["h"], p))


main()
