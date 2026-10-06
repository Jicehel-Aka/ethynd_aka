#!/usr/bin/env python3
"""Verifie le monde (cartes, histoire) et les traductions, sans rien generer.

Usage : python3 tools/check_world.py [dossier world]    (defaut : ../world)

 - references croisees : portes, PNJ, objets, dialogues, objectifs, drapeaux (world_io.validate)
 - traductions : pour chaque langue de world/lang/, liste les cles qui manquent par rapport a
   la langue par defaut (fr) et les cles inutiles ; le jeu retombe sur fr pour une cle manquante.
Code de retour : 1 s'il y a une ERREUR (reference cassee, cle absente de fr) ; 0 sinon.
Les cles manquantes dans une autre langue sont des AVERTISSEMENTS (--strict : erreurs).
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import world_io as wio

MONSTERS = ["chauve_souris", "chat", "dragon_rouge", "oiseau", "poussin"]   # constantes d'Ethynd (gen_data.py)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    strict = "--strict" in sys.argv
    d = args[0] if args else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "world")
    world = wio.parse_world(os.path.join(d, "world.txt"))
    story = wio.parse_story(os.path.join(d, "story.txt"))
    langs = wio.load_langs(os.path.join(d, "lang"))
    maps = {n: wio.parse_map(os.path.join(d, "maps", n + ".txt")) for n in world["maps"]}
    errs = wio.validate(world, maps, story, langs=langs, entity_names=MONSTERS)

    ref = langs[wio.DEFAULT_LANG]
    warns = []
    print("Langues : %s (defaut : %s, %d cles)" % (", ".join(sorted(langs)), wio.DEFAULT_LANG, len(ref)))
    for code in sorted(langs):
        if code == wio.DEFAULT_LANG:
            continue
        missing = sorted(k for k in ref if k not in langs[code])
        extra = sorted(k for k in langs[code] if k not in ref)
        done = len(ref) - len(missing)
        print("  %-4s %3d/%d cles traduites (%d%%)" % (code, done, len(ref), 100 * done // len(ref)))
        for k in missing:
            warns.append("%s.txt : cle manquante (repli sur %s) : %s" % (code, wio.DEFAULT_LANG, k))
        for k in extra:
            warns.append("%s.txt : cle inconnue de %s.txt : %s" % (code, wio.DEFAULT_LANG, k))
    used = set(wio.keys_used(story))
    for k in sorted(k for k in ref if k.startswith(("npc.", "item.", "sage.", "gardien.", "deny.", "obj.")) and k not in used):
        warns.append("%s.txt : cle d'histoire jamais utilisee : %s" % (wio.DEFAULT_LANG, k))

    for w in warns:
        print("  avertissement :", w)
    for e in errs:
        print("  ERREUR :", e)
    print("%d carte(s), %d dialogue(s), %d objectif(s), %d erreur(s), %d avertissement(s)" % (
        len(maps), len(story["dialogues"]), len(story["objectives"]), len(errs), len(warns)))
    sys.exit(1 if errs or ( strict and warns ) else 0)


main()
