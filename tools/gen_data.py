#!/usr/bin/env python3
"""Genere shared/game/EthyndData.h depuis les constantes Python d'Ethynd.

Usage : python3 gen_data.py <dossier Ethynd> <sortie EthyndData.h>

Extrait : animations/timings du joueur et des entites (noms de sprites, sans
chemin ni extension) et les monstres de chaque niveau. Les zones de
teleportation ne sont PAS dans les constantes (elles sont codees en dur dans
fonctions/jeu.py) : elles sont reecrites dans Game.cpp.
"""
import os
import sys
import types

DIRS = ["bas", "haut", "gauche", "droite"]
MOVS = ["base", "marche", "attaque"]


def stem(path):
    return os.path.splitext(os.path.basename(path))[0]


def main():
    src, out = sys.argv[1], sys.argv[2]
    pg = types.ModuleType("pygame")
    for k in ("K_UP", "K_LEFT", "K_DOWN", "K_RIGHT", "K_x"):
        setattr(pg, k, k)
    sys.modules["pygame"] = pg
    sys.path.insert(0, src)
    from constantes import constantes_entite as ce
    from constantes import constantes_joueur as cj
    from constantes import constantes_partie as cp

    # (id, timings{mov:[tick,last,libre,reset]}, anim{dir:{mov:[chemins]}})
    defs = [("joueur", cj.timings, cj.animation)]
    for ident in ce.animation["monstre"]:
        defs.append((ident, ce.timings["monstre"][ident],
                     ce.animation["monstre"][ident]))

    L = []
    w = L.append
    w("// GENERE par tools/gen_data.py a partir des constantes Python d'Ethynd.")
    w("// NE PAS EDITER A LA MAIN.")
    w("#pragma once")
    w('#include "Defs.h"')
    w("")
    for ident, timings, anim in defs:
        for d in DIRS:
            for m in MOVS:
                names = [stem(p) for p in anim.get(d, {}).get(m, [])]
                if names:
                    arr = ", ".join('"%s"' % n for n in names)
                    w("static const char* const k_%s_%s_%s[] = { %s };"
                      % (ident, d, m, arr))
    w("")
    for ident, timings, anim in defs:
        w("static const CharDef kDef_%s = {" % ident)
        w('    "%s",' % ident)
        w("    { // timings : tick, derniere frame, libre, retour a base")
        for m in MOVS:
            t = timings.get(m, [None])
            if t[0] is None:
                w("        { -1, 0, true, false },  // %s" % m)
            else:
                w("        { %d, %d, %s, %s },  // %s" % (
                    t[0], t[1], str(t[2]).lower(), str(t[3]).lower(), m))
        w("    },")
        w("    { // animations [direction][mouvement]")
        for d in DIRS:
            row = []
            for m in MOVS:
                names = anim.get(d, {}).get(m, [])
                if names:
                    row.append("{ %d, k_%s_%s_%s }" % (len(names), ident, d, m))
                else:
                    row.append("{ 0, nullptr }")
            w("        { %s },  // %s" % (", ".join(row), d))
        w("    },")
        w("};")
        w("")
    ids = [i for i, _, _ in defs if i != "joueur"]
    w("static const CharDef* const kEntityDefs[] = { %s };"
      % ", ".join("&kDef_" + i for i in ids))
    w("static const char* const kEntityNames[] = { %s };"
      % ", ".join('"%s"' % i for i in ids))
    w("static const int kEntityDefCount = %d;" % len(ids))
    w("")
    w("// Niveaux : monstres (deplacement aleatoire). Positions en pixels logiques (32 px/case)")
    w("struct SpawnDef { const char* type; int16_t x, y, w, h; int16_t vie, attaque; };")
    for nom, groupes in cp.niveau.items():
        spawns = []
        for typ, lst in groupes.items():
            for (pos, taille, dep, vie, att) in lst:
                assert dep == "aleatoire"
                spawns.append('{ "%s", %d, %d, %d, %d, %d, %d }' % (
                    typ, pos[0], pos[1], taille[0], taille[1], vie, att))
        w("static const SpawnDef kSpawns_%s[] = { %s };" % (nom, ", ".join(spawns)))
        w("static const int kSpawnCount_%s = %d;" % (nom, len(spawns)))
    w("")
    w("struct LevelSpawns { const char* map; const SpawnDef* spawns; int count; };")
    items = ", ".join('{ "%s", kSpawns_%s, kSpawnCount_%s }' % (n, n, n)
                      for n in cp.niveau)
    w("static const LevelSpawns kLevelSpawns[] = { %s };" % items)
    w("static const int kLevelSpawnsCount = %d;" % len(cp.niveau))
    open(out, "w").write("\n".join(L) + "\n")
    print("OK", out, len(L), "lignes")


main()
