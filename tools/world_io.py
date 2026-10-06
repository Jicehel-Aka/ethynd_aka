"""world_io.py -- format texte du monde d'Ethynd (source de verite, editable a la main).

world/
  world.txt        start <carte> <x> <y> <dir>      (x, y : centre de la hitbox du joueur, px logiques)
                   maps <carte> <carte> ...         (ordre = index dans le binaire)
  tiles.txt        collide <id> <id> ...            (tuiles a collision, ids du tileset d'origine)
                   anim <id> <id_suivant>           (animation de tuile)
  maps/<nom>.txt   map / size / music / layer N + lignes CSV / spawn / npc / door / item
  story.txt        flag / itemdef / dialogue (say, set, give, take, heal, win) / objective
                   -- AUCUN texte en dur : uniquement des CLES de traduction
  lang/<code>.txt  cle = texte   (une langue par fichier : fr, en, es, de... ; fr = langue par defaut)

Tout est en pixels LOGIQUES (32 px par case, comme Ethynd). Les identifiants de tuiles sont
ceux du tileset d'origine (3800 tuiles) : le compilateur ne garde que les tuiles utilisees.
Tout texte visible est une cle (ex. sage.intro.1) traduite dans world/lang/<code>.txt : ajouter une
langue = ajouter un fichier, sans toucher au code. Cle absente d'une langue : repli sur fr.
"""
import os
import re

DIRS = ["bas", "haut", "gauche", "droite"]
MUSICS = ["none", "menu", "maison", "aventure", "grotte"]


class WorldError(Exception):
    pass


def _lines(path):
    with open(path, encoding="utf-8") as f:
        for n, raw in enumerate(f, 1):
            s = raw.strip()
            if s and not s.startswith("#"):
                yield n, s


def _err(path, n, msg):
    raise WorldError("%s:%d : %s" % (path, n, msg))


# ------------------------------------------------------------------ world.txt / tiles.txt
def parse_world(path):
    w = {"start": ("maison", 0, 0, "bas"), "maps": []}
    for n, s in _lines(path):
        t = s.split()
        if t[0] == "start" and len(t) == 5:
            if t[4] not in DIRS:
                _err(path, n, "direction inconnue %r" % t[4])
            w["start"] = (t[1], int(t[2]), int(t[3]), t[4])
        elif t[0] == "maps":
            w["maps"] = t[1:]
        else:
            _err(path, n, "ligne inconnue : %s" % s)
    return w


def write_world(path, w):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("# Monde d'Ethynd -- voir tools/world_io.py pour le format\n")
        f.write("start %s %d %d %s\n" % w["start"])
        f.write("maps %s\n" % " ".join(w["maps"]))


def parse_tiles(path):
    collide, anim = set(), {}
    for n, s in _lines(path):
        t = s.split()
        if t[0] == "collide":
            collide.update(int(v) for v in t[1:])
        elif t[0] == "anim" and len(t) == 3:
            anim[int(t[1])] = int(t[2])
        else:
            _err(path, n, "ligne inconnue : %s" % s)
    return collide, anim


def write_tiles(path, collide, anim):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("# Proprietes des tuiles (ids du tileset d'origine)\n")
        ids = sorted(collide)
        for i in range(0, len(ids), 16):
            f.write("collide %s\n" % " ".join(str(v) for v in ids[i:i + 16]))
        for a in sorted(anim):
            f.write("anim %d %d\n" % (a, anim[a]))


# ------------------------------------------------------------------ cartes
def new_map(name, w, h):
    return {"name": name, "w": w, "h": h, "music": "none",
            "layers": [[[-1] * w for _ in range(h)] for _ in range(4)],
            "spawns": [], "npcs": [], "doors": [], "items": []}


def parse_map(path):
    m = None
    layer = None
    row = 0
    for n, s in _lines(path):
        t = s.split()
        if layer is not None and re.fullmatch(r"-?\d+(,-?\d+)*,?", s):
            vals = [int(v) for v in s.rstrip(",").split(",")]
            if len(vals) != m["w"] or row >= m["h"]:
                _err(path, n, "ligne de tuiles de %d valeurs (attendu %d, ligne %d/%d)"
                     % (len(vals), m["w"], row + 1, m["h"]))
            m["layers"][layer][row] = vals
            row += 1
            continue
        k = t[0]
        if k == "map":
            name = t[1]
        elif k == "size":
            m = new_map(name, int(t[1]), int(t[2]))
        elif m is None:
            _err(path, n, "'size' doit preceder le reste")
        elif k == "music":
            if t[1] not in MUSICS:
                _err(path, n, "musique inconnue %r (%s)" % (t[1], ", ".join(MUSICS)))
            m["music"] = t[1]
        elif k == "layer":
            layer, row = int(t[1]), 0
            if not 0 <= layer <= 3:
                _err(path, n, "couche 0..3")
        elif k == "spawn" and len(t) == 6:
            m["spawns"].append({"type": t[1], "x": int(t[2]), "y": int(t[3]), "vie": int(t[4]), "attaque": int(t[5])})
        elif k == "npc" and len(t) == 7:
            if t[5] not in DIRS:
                _err(path, n, "direction inconnue %r" % t[5])
            m["npcs"].append({"id": t[1], "type": t[2], "x": int(t[3]), "y": int(t[4]), "dir": t[5], "rules": t[6]})
        elif k == "door" and len(t) == 10:
            m["doors"].append({"x": int(t[1]), "y": int(t[2]), "w": int(t[3]), "h": int(t[4]), "map": t[5],
                               "dx": int(t[6]), "dy": int(t[7]), "requires": t[8], "deny": t[9]})
        elif k == "item" and len(t) == 4:
            m["items"].append({"id": t[1], "x": int(t[2]), "y": int(t[3])})
        else:
            _err(path, n, "ligne inconnue ou incomplete : %s" % s)
    if m is None:
        raise WorldError("%s : pas de ligne 'size'" % path)
    return m


def write_map(path, m):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("map %s\nsize %d %d\nmusic %s\n" % (m["name"], m["w"], m["h"], m["music"]))
        for i in range(4):
            f.write("layer %d\n" % i)
            for r in m["layers"][i]:
                f.write(",".join(str(v) for v in r) + "\n")
        for s in m["spawns"]:
            f.write("spawn %s %d %d %d %d\n" % (s["type"], s["x"], s["y"], s["vie"], s["attaque"]))
        for s in m["npcs"]:
            f.write("npc %s %s %d %d %s %s\n" % (s["id"], s["type"], s["x"], s["y"], s["dir"], s["rules"]))
        for d in m["doors"]:
            f.write("door %d %d %d %d %s %d %d %s %s\n" % (d["x"], d["y"], d["w"], d["h"], d["map"], d["dx"], d["dy"],
                                                           d["requires"], d["deny"]))
        for s in m["items"]:
            f.write("item %s %d %d\n" % (s["id"], s["x"], s["y"]))


# ------------------------------------------------------------------ langues
DEFAULT_LANG = "fr"


def parse_lang(path):
    """cle = texte (le texte peut contenir des '=' ; \\n n'est pas gere : le jeu coupe les lignes)."""
    d = {}
    for n, s in _lines(path):
        k, eq, v = s.partition("=")
        if not eq or not k.strip():
            _err(path, n, "format attendu : cle = texte")
        d[k.strip()] = v.strip()
    return d


def load_langs(lang_dir):
    langs = {}
    for fn in sorted(os.listdir(lang_dir)):
        if fn.endswith(".txt"):
            langs[fn[:-4]] = parse_lang(os.path.join(lang_dir, fn))
    if DEFAULT_LANG not in langs:
        raise WorldError("%s : la langue par defaut (%s.txt) est obligatoire" % (lang_dir, DEFAULT_LANG))
    return langs


def keys_used(story):
    """Cles de traduction referencees par l'histoire (le reste, ui.*, est verifie ailleurs)."""
    ks = []
    for it in story["items"]:
        ks.append(it["name"])
    for d in story["dialogues"]:
        for ln in d["lines"]:
            ks += [ln["speaker"], ln["text"]]
    for o in story["objectives"]:
        ks.append(o["title"])
    return [k for k in ks if k and k != "-"]


# ------------------------------------------------------------------ histoire
def parse_story(path):
    st = {"flags": [], "items": [], "dialogues": [], "objectives": []}
    cur = None
    for n, s in _lines(path):
        t = s.split()
        head = t[0]
        if head == "flag" and len(t) == 2:
            if t[1] in st["flags"]:
                _err(path, n, "drapeau deja declare : %s" % t[1])
            st["flags"].append(t[1])
        elif head == "itemdef" and len(t) == 5:
            st["items"].append({"id": t[1], "icon": t[2], "heal": int(t[3]), "name": t[4]})
        elif head == "dialogue" and len(t) == 2:
            cur = {"id": t[1], "lines": [], "actions": []}
            st["dialogues"].append(cur)
        elif head in ("say", "set", "give", "take", "heal", "win"):
            if cur is None:
                _err(path, n, "'%s' hors d'un 'dialogue'" % head)
            if head == "say" and len(t) == 3:
                cur["lines"].append({"speaker": t[1], "text": t[2]})
            elif head != "say":
                cur["actions"].append((head, t[1:]))
            else:
                _err(path, n, "say <cle locuteur|-> <cle texte>")
        elif head == "objective" and len(t) == 5:
            st["objectives"].append({"id": t[1], "after": t[2], "cond": t[3], "title": t[4]})
        else:
            _err(path, n, "ligne inconnue ou incomplete : %s" % s)
    return st


def validate(world, maps, story, langs=None, entity_names=()):
    """Verifie les references croisees ; renvoie la liste des erreurs (vide = tout va bien)."""
    errs = []
    if langs is not None:
        ref = langs[DEFAULT_LANG]
        for k in keys_used(story):
            if k not in ref:
                errs.append("cle de traduction absente de %s.txt : %s" % (DEFAULT_LANG, k))
    flags = set(story["flags"])
    items = {i["id"] for i in story["items"]}
    dlgs = {d["id"] for d in story["dialogues"]}
    objs = [o["id"] for o in story["objectives"]]
    names = set(world["maps"])
    if len(flags) > 64:
        errs.append("plus de 64 drapeaux")
    if len(items) > 16:
        errs.append("plus de 16 objets")
    if len(objs) > 32:
        errs.append("plus de 32 objectifs")
    if world["start"][0] not in names:
        errs.append("carte de depart inconnue : %s" % world["start"][0])
    for d in story["dialogues"]:
        for kind, args in d["actions"]:
            if kind == "set" and args[0] not in flags:
                errs.append("dialogue %s : drapeau inconnu %s" % (d["id"], args[0]))
            if kind in ("give", "take") and args[0] not in items:
                errs.append("dialogue %s : objet inconnu %s" % (d["id"], args[0]))
    for o in story["objectives"]:
        if o["after"] != "-" and o["after"] not in objs:
            errs.append("objectif %s : 'apres' inconnu %s" % (o["id"], o["after"]))
        c = o["cond"].split(":")
        if c[0] == "flag" and c[1] not in flags:
            errs.append("objectif %s : drapeau inconnu %s" % (o["id"], c[1]))
        elif c[0] == "item" and c[1] not in items:
            errs.append("objectif %s : objet inconnu %s" % (o["id"], c[1]))
        elif c[0] in ("map", "reach") and c[1] not in names:
            errs.append("objectif %s : carte inconnue %s" % (o["id"], c[1]))
        elif c[0] == "kill" and entity_names and c[1] not in entity_names:
            errs.append("objectif %s : monstre inconnu %s" % (o["id"], c[1]))
        elif c[0] not in ("flag", "kill", "item", "map", "reach"):
            errs.append("objectif %s : condition inconnue %s" % (o["id"], c[0]))
    for name, m in maps.items():
        for d in m["doors"]:
            if d["map"] not in names:
                errs.append("%s : porte vers une carte inconnue %s" % (name, d["map"]))
            if d["requires"] != "-" and d["requires"] not in flags:
                errs.append("%s : porte, drapeau inconnu %s" % (name, d["requires"]))
            if d["deny"] != "-" and d["deny"] not in dlgs:
                errs.append("%s : porte, dialogue inconnu %s" % (name, d["deny"]))
        for s in m["items"]:
            if s["id"] not in items:
                errs.append("%s : objet inconnu %s" % (name, s["id"]))
        for s in m["spawns"]:
            if entity_names and s["type"] not in entity_names:
                errs.append("%s : monstre inconnu %s" % (name, s["type"]))
        for s in m["npcs"]:
            if s["rules"] != "-":
                for rule in s["rules"].split(";"):
                    cond, _, dl = rule.rpartition("?")
                    if dl not in dlgs:
                        errs.append("%s : PNJ %s, dialogue inconnu %s" % (name, s["id"], dl))
                    c = cond.lstrip("!")
                    if cond and not c.startswith("has:") and c not in flags:
                        errs.append("%s : PNJ %s, drapeau inconnu %s" % (name, s["id"], c))
                    if c.startswith("has:") and c[4:] not in items:
                        errs.append("%s : PNJ %s, objet inconnu %s" % (name, s["id"], c[4:]))
    return errs
