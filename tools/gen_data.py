#!/usr/bin/env python3
"""Genere shared/game/EthyndData.h depuis les constantes Python d'Ethynd.

Usage : python3 gen_data.py <dossier Ethynd> <sortie EthyndData.h>

Extrait : animations/timings du joueur et des entites (noms de sprites, sans
chemin ni extension), plus les variantes de PNJ (sage, gardien : memes animations que le
joueur, sprites recolores par convert_assets.py, sans attaque).
Les monstres, portes, PNJ, objets et l'histoire ne sont PLUS ici : ils viennent des
fichiers de donnees de world/ (cartes + story.txt), lus a l'execution.
"""
NPC_VARIANTS = ["sage", "gardien"]
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

    # (id, timings{mov:[tick,last,libre,reset]}, anim{dir:{mov:[chemins]}})
    defs = [("joueur", cj.timings, cj.animation)]
    for ident in ce.animation["monstre"]:
        defs.append((ident, ce.timings["monstre"][ident],
                     ce.animation["monstre"][ident]))
    monsters = [i for i, _, _ in defs[1:]]
    for variant in NPC_VARIANTS:       # PNJ : sprites du joueur recolores, arret + marche
        anim = {}
        for d in DIRS:
            anim[d] = {}
            for m in ("base", "marche"):
                anim[d][m] = ["%s_%s" % (variant, stem(p).split("_")[1])
                              for p in cj.animation.get(d, {}).get(m, [])]
        tim = {m: t for m, t in cj.timings.items() if m != "attaque"}
        defs.append((variant, tim, anim))

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
    w("static const CharDef* const kEntityDefs[] = { %s };"
      % ", ".join("&kDef_" + i for i in monsters))
    w("static const char* const kEntityNames[] = { %s };"
      % ", ".join('"%s"' % i for i in monsters))
    w("static const int kEntityDefCount = %d;      // monstres (indices des compteurs de victimes)" % len(monsters))
    w("")
    w("// Types utilisables pour un PNJ : variantes dediees puis joueur")
    npcs = NPC_VARIANTS + ["joueur"]
    w("static const CharDef* const kNpcDefs[] = { %s };" % ", ".join("&kDef_" + i for i in npcs))
    w("static const char* const kNpcNames[] = { %s };" % ", ".join('"%s"' % i for i in npcs))
    w("static const int kNpcDefCount = %d;" % len(npcs))
    open(out, "w").write("\n".join(L) + "\n")
    print("OK", out, len(L), "lignes")


main()
