#!/usr/bin/env python3
"""Verifie le monde (cartes, histoire), les traductions et leur coherence avec le code.

Usage : python3 tools/check_world.py [dossier world] [--strict]     (defaut : ../world)

 - references croisees : portes, PNJ, objets, dialogues, objectifs, drapeaux (world_io.validate)
 - traductions : par langue, cles manquantes ou vides (le jeu retombe sur fr), cles inconnues ;
 - cles utilisees par le code (shared/, main/, platform_*/, editor/) : absentes de fr.txt = ERREUR,
   cles ui.* de fr.txt que personne n'utilise = avertissement ;
 - caracteres que la police du jeu ne sait pas dessiner (affichage "?" a l'ecran) ;
 - textes trop longs pour leur emplacement (boutons, bandeau d'objectif).
Code de retour : 1 s'il y a une ERREUR ; avec --strict, les avertissements comptent aussi.
"""
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
sys.path.insert(0, HERE)
import world_io as wio

FALLBACK_CHARS = {0x0153: "oe", 0x0152: "OE", 0x2019: "'", 0x2018: "'", 0x201C: '"', 0x201D: '"', 0x2014: "-", 0x2013: "-"}


def monsters_from_header():
    p = os.path.join(ROOT, "shared", "game", "EthyndData.h")
    if os.path.exists(p):
        m = re.search(r"kEntityNames\[\]\s*=\s*\{([^}]*)\}", open(p, encoding="utf8").read())
        if m:
            return re.findall(r'"([^"]+)"', m.group(1))
    return []


def font_glyphs():
    p = os.path.join(ROOT, "components", "aka_font", "include", "aka_font", "font8x8_extended.h")
    if not os.path.exists(p):
        return None
    return {int(h, 16) for h in re.findall(r"\{\s*0x([0-9A-Fa-f]{4})\s*,", open(p, encoding="utf8", errors="ignore").read())}


def code_keys():
    used = set()
    files = []
    for d in ("shared/game", "main", "platform_sdl", "components/platform_aka", "editor"):
        files += glob.glob(os.path.join(ROOT, d, "*.cpp")) + glob.glob(os.path.join(ROOT, d, "*.h"))
    for f in files:
        if os.path.basename(f).startswith("test_") or os.path.basename(f) == "EthyndData.h":
            continue
        used |= set(re.findall(r'"((?:ui|ed|pc)\.[a-z_0-9]+(?:\.[a-z_0-9]+)*)"', open(f, encoding="utf8", errors="ignore").read()))
    return used


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    strict = "--strict" in sys.argv
    d = args[0] if args else os.path.join(ROOT, "world")
    world = wio.parse_world(os.path.join(d, "world.txt"))
    story = wio.parse_story(os.path.join(d, "story.txt"))
    langs = wio.load_langs(os.path.join(d, "lang"))
    maps = {n: wio.parse_map(os.path.join(d, "maps", n + ".txt")) for n in world["maps"]}
    errs = wio.validate(world, maps, story, langs=langs, entity_names=monsters_from_header())
    warns = []

    ref = langs[wio.DEFAULT_LANG]
    print("Langues : %s (defaut : %s, %d cles)" % (", ".join(sorted(langs)), wio.DEFAULT_LANG, len(ref)))
    for code in sorted(langs):
        if code == wio.DEFAULT_LANG:
            continue
        missing = sorted(k for k in ref if not langs[code].get(k))
        extra = sorted(k for k in langs[code] if k not in ref)
        done = len(ref) - len(missing)
        print("  %-4s %3d/%d cles traduites (%d%%)" % (code, done, len(ref), 100 * done // len(ref)))
        for k in missing:
            warns.append("%s.txt : cle manquante ou vide (repli sur %s) : %s" % (code, wio.DEFAULT_LANG, k))
        for k in extra:
            warns.append("%s.txt : cle inconnue de %s.txt : %s" % (code, wio.DEFAULT_LANG, k))

    # cles referencees par le code
    used = code_keys()
    for k in sorted(k for k in used if k not in ref):
        errs.append("cle utilisee par le code mais absente de %s.txt : %s" % (wio.DEFAULT_LANG, k))
    for k in sorted(k for k in ref if k.startswith("ui.") and k not in used):
        warns.append("%s.txt : cle ui.* jamais utilisee par le code : %s" % (wio.DEFAULT_LANG, k))
    story_keys = set(wio.keys_used(story))
    for k in sorted(k for k in ref if k.startswith(("npc.", "item.", "sage.", "gardien.", "deny.", "obj.")) and k not in story_keys):
        warns.append("%s.txt : cle d'histoire jamais utilisee : %s" % (wio.DEFAULT_LANG, k))

    # caracteres que la police ne sait pas dessiner (les cles aka.* utilisent la police du systeme)
    glyphs = font_glyphs()
    if glyphs is not None:
        for code in sorted(langs):
            seen = {}
            for k, v in langs[code].items():
                if k.startswith("aka."):
                    continue
                for ch in v:
                    c = ord(ch)
                    if 32 <= c < 127 or c in glyphs:
                        continue
                    seen.setdefault(ch, []).append(k)
            for ch, ks in sorted(seen.items()):
                how = "remplace par '%s'" % FALLBACK_CHARS[ord(ch)] if ord(ch) in FALLBACK_CHARS else "affiche '?'"
                warns.append("%s.txt : caractere %r (U+%04X) non dessine par la police, %s : %s%s" % (
                    code, ch, ord(ch), how, ks[0], " (+%d)" % (len(ks) - 1) if len(ks) > 1 else ""))
    # longueurs
    for code in sorted(langs):
        for k in ("ui.menu.play", "ui.menu.help"):
            v = langs[code].get(k, "")
            if len(v) > 14:
                warns.append("%s.txt : %s tient mal dans le bouton (%d caracteres, 14 max) : %r" % (code, k, len(v), v))
        for o in story["objectives"]:
            v = langs[code].get(o["title"], "")
            if len(v) > 36:
                warns.append("%s.txt : titre d'objectif long (%d car.) : tronque dans le bandeau, complet dans le journal : %s" % (code, len(v), o["title"]))

    for w in warns:
        print("  avertissement :", w)
    for e in errs:
        print("  ERREUR :", e)
    print("%d carte(s), %d dialogue(s), %d objectif(s), %d erreur(s), %d avertissement(s)" % (
        len(maps), len(story["dialogues"]), len(story["objectives"]), len(errs), len(warns)))
    sys.exit(1 if errs or (strict and warns) else 0)


main()
