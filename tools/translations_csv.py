#!/usr/bin/env python3
"""Echange des traductions avec un tableur (pour les traducteurs non techniques).

  python3 tools/translations_csv.py export [world/lang] [sortie.csv]
      -> une ligne par cle, une colonne par langue (fr en premier) ; encodage UTF-8 avec BOM (Excel)
  python3 tools/translations_csv.py import <fichier.csv> [world/lang]
      -> met a jour les fichiers lang/<code>.txt : les commentaires et l'ordre sont conserves,
         une cellule vide ne change rien, les nouvelles cles sont ajoutees a la fin
  python3 tools/translations_csv.py new <code> [world/lang]
      -> cree lang/<code>.txt avec toutes les cles de fr.txt et des valeurs VIDES (a traduire :
         le jeu retombe sur le francais tant qu'une valeur est vide)
"""
import csv
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import world_io as wio

DEFAULT_DIR = os.path.join(HERE, "..", "world", "lang")


def export(lang_dir, out):
    langs = wio.load_langs(lang_dir)
    order = [wio.DEFAULT_LANG] + sorted(c for c in langs if c != wio.DEFAULT_LANG)
    keys = list(langs[wio.DEFAULT_LANG])
    for c in order[1:]:
        keys += [k for k in langs[c] if k not in langs[wio.DEFAULT_LANG] and k not in keys]
    with open(out, "w", encoding="utf-8-sig", newline="") as f:
        w = csv.writer(f, delimiter=";")
        w.writerow(["key"] + order)
        for k in keys:
            w.writerow([k] + [langs[c].get(k, "") for c in order])
    print("exporte : %d cles, langues %s -> %s" % (len(keys), ", ".join(order), out))


def update_file(path, updates):
    """Remplace les valeurs des cles existantes ; ajoute les nouvelles a la fin ; garde le reste intact."""
    lines = open(path, encoding="utf-8").read().split("\n") if os.path.exists(path) else []
    done = set()
    for i, l in enumerate(lines):
        m = re.match(r"^(\s*)([^#=\s][^=]*?)\s*=", l)
        if m and m.group(2).strip() in updates:
            k = m.group(2).strip()
            lines[i] = "%s = %s" % (k, updates[k])
            done.add(k)
    new = [k for k in updates if k not in done]
    if new:
        while lines and lines[-1] == "":
            lines.pop()
        lines += ["", "# --- importe d'un tableur"] + ["%s = %s" % (k, updates[k]) for k in new]
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines).rstrip("\n") + "\n")
    return len(done), len(new)


def do_import(csv_path, lang_dir):
    with open(csv_path, encoding="utf-8-sig", newline="") as f:
        rows = list(csv.reader(f, delimiter=";"))
    head = rows[0]
    if head[0] != "key":
        sys.exit("premiere colonne attendue : key")
    for ci in range(1, len(head)):
        code = head[ci].strip()
        if not re.fullmatch(r"[a-z]{2,3}(-[A-Za-z]{2,4})?", code):
            print("colonne ignoree (code de langue invalide) :", head[ci])
            continue
        updates = {r[0].strip(): r[ci].strip() for r in rows[1:] if len(r) > ci and r[0].strip() and r[ci].strip()}
        a, b = update_file(os.path.join(lang_dir, code + ".txt"), updates)
        print("  %-4s %d valeur(s) mise(s) a jour, %d cle(s) ajoutee(s)" % (code, a, b))


def new_language(code, lang_dir):
    if not re.fullmatch(r"[a-z]{2,3}(-[A-Za-z]{2,4})?", code):
        sys.exit("code de langue invalide : %r (ex. es, de, pt-BR)" % code)
    dst = os.path.join(lang_dir, code + ".txt")
    if os.path.exists(dst):
        sys.exit("%s existe deja" % dst)
    ref = wio.parse_lang(os.path.join(lang_dir, wio.DEFAULT_LANG + ".txt"))
    out = ["# Langue : %s. Valeur vide = pas encore traduit (le jeu affiche alors le francais)." % code,
           "# Reference : fr.txt. Voir docs/MULTILANGUE.md.", ""]
    section = None
    for line in open(os.path.join(lang_dir, wio.DEFAULT_LANG + ".txt"), encoding="utf-8").read().split("\n"):
        s = line.strip()
        if s.startswith("# ---"):
            out += ["", s]
        elif s and not s.startswith("#") and "=" in s:
            k = s.partition("=")[0].strip()
            out.append("%s = " % k)
    with open(dst, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out).rstrip("\n") + "\n")
    print("cree : %s (%d cles a traduire)" % (dst, len(ref)))


def main():
    a = sys.argv[1:]
    if not a or a[0] not in ("export", "import", "new"):
        sys.exit(__doc__)
    if a[0] == "export":
        d = a[1] if len(a) > 1 else DEFAULT_DIR
        export(d, a[2] if len(a) > 2 else "translations.csv")
    elif a[0] == "import":
        if len(a) < 2:
            sys.exit(__doc__)
        do_import(a[1], a[2] if len(a) > 2 else DEFAULT_DIR)
    else:
        if len(a) < 2:
            sys.exit(__doc__)
        new_language(a[1], a[2] if len(a) > 2 else DEFAULT_DIR)


main()
