#!/usr/bin/env python3
"""Importe le jeu Ethynd d'origine (cartes CSV + constantes Python + zones de teleportation
codees en dur dans fonctions/jeu.py) vers le format texte editable de world/.

Usage : python3 tools/import_ethynd.py <dossier Ethynd-master> <dossier world>

A lancer UNE fois : ensuite world/ est la source de verite (editeur ou editeur de texte).
Les PNJ, objets et l'histoire (story.txt) sont ajoutes par ce script ou ecrits a la main ;
les portes d'origine sont converties : une zone de camera [x0,x1]x[y0,y1] devient un
rectangle monde (le joueur est a (320 - camX, 253 - camY) dans le monde).
"""
import os
import sys
import types

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import world_io as wio

# (carte, zone camera x0,x1,y0,y1) -> (carte d'arrivee, camera d'arrivee) : fonctions/jeu.py
ZONES = [
    ("maison",   (110, 146, -249, -235), "aventure", (-464, -261)),
    ("maison",   (-19, 26, -180, -168),  "grotte",   (-416, -180)),
    ("grotte",   (133, 220, -610, -600), "aventure", (-1103, -460)),
    ("grotte",   (-434, -398, -138, -111), "maison", (0, -125)),
    ("aventure", (-467, -461, -255, -237), "maison", (128, -234)),
    ("aventure", (-1106, -1100, -451, -430), "grotte", (175, -591)),
]
# Portes verrouillees tant que le sage n'a pas parle (histoire ajoutee au jeu)
LOCKED = {("maison", "grotte"): "deny_cave_house", ("aventure", "grotte"): "deny_cave_village"}

# Deux chauves-souris de l'original apparaissent DANS un mur (bloquees pour toujours) : decalees
SPAWN_FIX = {("grotte", 500, 400): (484, 384), ("grotte", 700, 600): (684, 584)}

START = ("maison", 320 - 14, 253 - 93, "bas")
MUSIC = {"maison": "maison", "aventure": "aventure", "grotte": "grotte"}

# PNJ et objets ajoutes par ce portage : (carte, ...)
NPCS = [
    ("maison", {"id": "sage", "type": "sage", "x": 224, "y": 128, "dir": "droite",
                "rules": "has:crystal?sage_thanks;talked_sage?sage_wait;sage_intro"}),
    ("aventure", {"id": "gardien", "type": "gardien", "x": 1486, "y": 764, "dir": "bas", "rules": "gardien_talk"}),
]
ITEMS = [
    ("grotte", {"id": "crystal", "x": 96, "y": 640}),
    ("grotte", {"id": "potion", "x": 416, "y": 256}),
    ("aventure", {"id": "potion", "x": 984, "y": 832}),
    ("aventure", {"id": "potion", "x": 1500, "y": 1300}),
    ("maison", {"id": "potion", "x": 64, "y": 352}),
]


def main():
    src, dst = sys.argv[1], sys.argv[2]
    pg = types.ModuleType("pygame")
    for k in ("K_UP", "K_LEFT", "K_DOWN", "K_RIGHT", "K_x"):
        setattr(pg, k, k)
    sys.modules["pygame"] = pg
    sys.path.insert(0, src)
    from constantes import constantes_tuiles as ct
    from constantes import constantes_partie as cp

    os.makedirs(os.path.join(dst, "maps"), exist_ok=True)
    names = ["maison", "aventure", "grotte"]
    wio.write_world(os.path.join(dst, "world.txt"),
                    {"start": START, "maps": names})
    wio.write_tiles(os.path.join(dst, "tiles.txt"),
                    {int(v) for v in ct.collisions}, {int(a): int(b) for a, b in ct.animations.items()})

    for name in names:
        layers = []
        for i in range(4):
            with open(os.path.join(src, "maps", "%s_%d.csv" % (name, i))) as f:
                layers.append([[int(v) for v in l.strip().split(",")] for l in f if l.strip()])
        m = wio.new_map(name, len(layers[0][0]), len(layers[0]))
        m["layers"] = layers
        m["music"] = MUSIC[name]
        for typ, lst in cp.niveau.get(name, {}).items():
            for (pos, taille, dep, vie, att) in lst:
                x, y = SPAWN_FIX.get((name, pos[0], pos[1]), (pos[0], pos[1]))
                m["spawns"].append({"type": typ, "x": x, "y": y, "vie": vie, "attaque": att})
        for (mp, (x0, x1, y0, y1), dest, (cx, cy)) in ZONES:
            if mp != name:
                continue
            lock = LOCKED.get((mp, dest), "-")
            m["doors"].append({"x": 320 - x1, "y": 253 - y1, "w": x1 - x0 + 1, "h": y1 - y0 + 1,
                               "map": dest, "dx": 320 - cx, "dy": 253 - cy,
                               "requires": "talked_sage" if lock != "-" else "-", "deny": lock})
        for (mp, n) in NPCS:
            if mp == name:
                m["npcs"].append(dict(n))
        for (mp, it) in ITEMS:
            if mp == name:
                m["items"].append(dict(it))
        wio.write_map(os.path.join(dst, "maps", name + ".txt"), m)
        print("carte", name, m["w"], "x", m["h"], len(m["spawns"]), "monstres,", len(m["doors"]), "portes")


main()
