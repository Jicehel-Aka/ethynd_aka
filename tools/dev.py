#!/usr/bin/env python3
"""Point d'entree unique des outils (Windows, Linux, macOS : Python suffit, pas de bash).

  python3 tools/dev.py check                       verifie le monde et les traductions
  python3 tools/dev.py assets <Ethynd-master>      genere la carte SD (sdcard_files/Ethynd) + assets de l'editeur
  python3 tools/dev.py test <Ethynd-master>        compile et lance les tests du jeu et de l'editeur (g++ requis)
  python3 tools/dev.py maps <Ethynd-master>        apercus PNG des cartes -> docs/maps/
  python3 tools/dev.py package <Ethynd-master>     fabrique dist/Ethynd_sdcard.zip et dist/Ethynd_editor_assets.zip
  python3 tools/dev.py lang new <code>             cree world/lang/<code>.txt (valeurs vides a traduire)
  python3 tools/dev.py lang export [fichier.csv]   exporte toutes les langues vers un tableur (CSV)
  python3 tools/dev.py lang import <fichier.csv>   re-importe les traductions du tableur
  python3 tools/dev.py lang status                 pourcentage traduit par langue (= check)

Options de `assets` / `package` : --tile 16|24|32 (defaut 24), --no-audio (ignore ffmpeg).
"""
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
PY = sys.executable


def run(*cmd, **kw):
    print("$", " ".join(str(c) for c in cmd))
    r = subprocess.run([str(c) for c in cmd], **kw)
    if r.returncode != 0:
        sys.exit(r.returncode)


def opt(name, default=None):
    if name in sys.argv:
        i = sys.argv.index(name)
        return sys.argv[i + 1] if i + 1 < len(sys.argv) else default
    return default


def positional():
    skip = False
    out = []
    for i, a in enumerate(sys.argv[2:], 2):
        if skip:
            skip = False
        elif a == "--tile":
            skip = True
        elif not a.startswith("--"):
            out.append(a)
    return out


def src_arg():
    p = positional()
    if not p:
        sys.exit("indiquer le dossier du depot Ethynd d'origine (github.com/ProjetIsn2019/Ethynd)")
    if not os.path.isdir(os.path.join(p[0], "images")):
        sys.exit("%s ne ressemble pas au depot Ethynd (dossier images/ absent)" % p[0])
    return p[0]


def build_assets(src, sd_out, editor_out):
    tile = opt("--tile", "24")
    run(PY, os.path.join(HERE, "check_world.py"), os.path.join(ROOT, "world"))
    run(PY, os.path.join(HERE, "gen_data.py"), src, os.path.join(ROOT, "shared", "game", "EthyndData.h"))
    cmd = [PY, os.path.join(HERE, "convert_assets.py"), src, sd_out, "--world", os.path.join(ROOT, "world"),
           "--tile", tile, "--editor-out", editor_out]
    if "--no-audio" not in sys.argv:
        if shutil.which("ffmpeg"):
            cmd.append("--audio")
        else:
            print("ffmpeg introuvable : audio ignore (--no-audio pour ne plus voir ce message)")
    run(*cmd)
    mf = os.path.join(sd_out, "manifest.json")
    if os.path.exists(mf):
        os.remove(mf)


def cmd_test():
    src = src_arg()
    if not shutil.which("g++"):
        sys.exit("g++ introuvable (Linux : apt install g++ ; Windows : MSYS2/MinGW, ou utiliser CTest : voir docs/OUTILS.md)")
    tmp = tempfile.mkdtemp(prefix="ethynd_test_")
    out24, edit, shots = [os.path.join(tmp, d) for d in ("out24", "editor", "shots")]
    run(PY, os.path.join(HERE, "check_world.py"), os.path.join(ROOT, "world"))
    run(PY, os.path.join(HERE, "convert_assets.py"), src, out24, "--world", os.path.join(ROOT, "world"), "--tile", "24", "--editor-out", edit)
    env = dict(os.environ, AKA_FONT_INC=os.path.join(ROOT, "components", "aka_font", "include"))
    run("bash", os.path.join(ROOT, "host_test", "build_and_run.sh"), out24, shots, env=env)
    run("bash", os.path.join(ROOT, "editor", "build_and_run_test.sh"), os.path.join(ROOT, "world"), edit, shots)
    print("captures dans", shots)


def package():
    src = src_arg()
    dist = os.path.join(ROOT, "dist")
    os.makedirs(dist, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix="ethynd_pkg_")
    sd, edit = os.path.join(tmp, "Ethynd"), os.path.join(tmp, "editor_assets")
    build_assets(src, sd, edit)
    for name, folder, arc in (("Ethynd_sdcard.zip", sd, "Ethynd"), ("Ethynd_editor_assets.zip", edit, "editor_assets")):
        with zipfile.ZipFile(os.path.join(dist, name), "w", zipfile.ZIP_DEFLATED) as z:
            for base, _, files in os.walk(folder):
                for f in files:
                    full = os.path.join(base, f)
                    z.write(full, os.path.join(arc, os.path.relpath(full, folder)))
        print("->", os.path.join(dist, name))


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help", "help"):
        sys.exit(__doc__)
    c = sys.argv[1]
    lang_dir = os.path.join(ROOT, "world", "lang")
    if c == "check":
        run(PY, os.path.join(HERE, "check_world.py"), os.path.join(ROOT, "world"), *[a for a in sys.argv[2:] if a == "--strict"])
    elif c == "assets":
        build_assets(src_arg(), os.path.join(ROOT, "sdcard_files", "Ethynd"), os.path.join(ROOT, "editor_assets"))
        print("\nCarte SD : copier le CONTENU de sdcard_files/Ethynd/ dans /Ethynd/ sur la SD.")
    elif c == "test":
        cmd_test()
    elif c == "maps":
        run(PY, os.path.join(HERE, "render_maps.py"), src_arg())
    elif c == "package":
        package()
    elif c == "lang":
        sub = sys.argv[2] if len(sys.argv) > 2 else ""
        tcsv = os.path.join(HERE, "translations_csv.py")
        if sub == "new" and len(sys.argv) > 3:
            run(PY, tcsv, "new", sys.argv[3], lang_dir)
        elif sub == "export":
            run(PY, tcsv, "export", lang_dir, sys.argv[3] if len(sys.argv) > 3 else "translations.csv")
        elif sub == "import" and len(sys.argv) > 3:
            run(PY, tcsv, "import", sys.argv[3], lang_dir)
        elif sub == "status":
            run(PY, os.path.join(HERE, "check_world.py"), os.path.join(ROOT, "world"))
        else:
            sys.exit(__doc__)
    else:
        sys.exit(__doc__)


main()
