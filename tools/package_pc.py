#!/usr/bin/env python3
"""Fabrique l'archive de la version PC : jeu + editeur + DLL (Windows) + assets + monde + docs + lanceurs.

Usage : python3 tools/package_pc.py --game <dossier de build du jeu> [--editor <dossier de build de l'editeur>]
                                    [--sd-zip Ethynd_sdcard.zip] [--editor-assets-zip Ethynd_editor_assets.zip]
                                    --out dist/Ethynd_PC_<systeme>.zip

L'archive est AUTONOME quand --sd-zip est fourni : on la decompresse, on lance `jouer` (ou ethynd_pc) et on joue.
Les DLL (SDL2, SDL2_mixer...) copiees a cote de l'executable par vcpkg sont incluses : sans elles, le jeu ne demarre pas sous Windows.
"""
import argparse
import os
import stat
import sys
import zipfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
TOP = "Ethynd-PC"

README = """ETHYND -- version PC / PC version
====================================

FR  Jouer : lancer `jouer` (Windows : jouer.bat, Linux : ./jouer.sh), ou directement ethynd_pc.
    Editeur de cartes : `editeur` (editeur.bat / ./editeur.sh).
    Touches : fleches ou ZQSD, X/Espace/Entree = A, Retour arriere/B = B, C/H = aide, F3/Tab = diagnostic, M = son, Echap = quitter.
    Les boutons dessines sous le jeu sont cliquables. Options : ethynd_pc [2|3|4] [--lang fr|en] [--no-panel].
    Linux : installer les bibliotheques : sudo apt install libsdl2-2.0-0 libsdl2-mixer-2.0-0

EN  Play: run `jouer` (Windows: jouer.bat, Linux: ./jouer.sh), or ethynd_pc directly.
    Map editor: `editeur` (editeur.bat / ./editeur.sh).
    Keys: arrows or WASD, X/Space/Enter = A, Backspace/B = B, C/H = help, F3/Tab = diagnostics, M = sound, Esc = quit.
    The buttons drawn below the game are clickable. Options: ethynd_pc [2|3|4] [--lang fr|en] [--no-panel].
    Linux: install the libraries: sudo apt install libsdl2-2.0-0 libsdl2-mixer-2.0-0

Documentation : docs/*.md   Licences / Licenses : LICENSE, NOTICE.md
"""


def find(folder, names):
    for base, _, files in os.walk(folder):
        for n in names:
            if n in files:
                return os.path.join(base, n)
    return None


def add_file(z, src, arc, exe=False):
    info = zipfile.ZipInfo.from_file(src, arc)
    info.compress_type = zipfile.ZIP_DEFLATED
    if exe:
        info.external_attr = (stat.S_IFREG | 0o755) << 16
    with open(src, "rb") as f:
        z.writestr(info, f.read())


def add_text(z, arc, text, exe=False, crlf=False):
    info = zipfile.ZipInfo(arc)
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = (stat.S_IFREG | (0o755 if exe else 0o644)) << 16
    z.writestr(info, text.replace("\n", "\r\n") if crlf else text)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game", required=True)
    ap.add_argument("--editor")
    ap.add_argument("--sd-zip")
    ap.add_argument("--editor-assets-zip")
    ap.add_argument("--out", required=True)
    a = ap.parse_args()

    game = find(a.game, ["ethynd_pc.exe", "ethynd_pc"])
    if not game:
        sys.exit("ethynd_pc introuvable dans %s" % a.game)
    editor = find(a.editor, ["ethynd_editor.exe", "ethynd_editor"]) if a.editor else None
    if a.editor and not editor:
        sys.exit("ethynd_editor introuvable dans %s" % a.editor)
    windows = game.endswith(".exe")
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)

    with zipfile.ZipFile(a.out, "w", zipfile.ZIP_DEFLATED) as z:
        added = set()
        for exe in (game, editor):
            if not exe:
                continue
            add_file(z, exe, "%s/%s" % (TOP, os.path.basename(exe)), exe=True)
            added.add(os.path.basename(exe))
            for f in sorted(os.listdir(os.path.dirname(exe))):          # DLL deposees a cote par vcpkg
                if f.lower().endswith(".dll") and f not in added:
                    add_file(z, os.path.join(os.path.dirname(exe), f), "%s/%s" % (TOP, f))
                    added.add(f)
        if windows:
            add_text(z, TOP + "/jouer.bat", "@echo off\ncd /d \"%~dp0\"\nethynd_pc.exe %*\n", crlf=True)
            if editor:
                add_text(z, TOP + "/editeur.bat", "@echo off\ncd /d \"%~dp0\"\nethynd_editor.exe --world world --assets editor_assets %*\n", crlf=True)
        else:
            add_text(z, TOP + "/jouer.sh", "#!/bin/sh\ncd \"$(dirname \"$0\")\"\nexec ./ethynd_pc \"$@\"\n", exe=True)
            if editor:
                add_text(z, TOP + "/editeur.sh", "#!/bin/sh\ncd \"$(dirname \"$0\")\"\nexec ./ethynd_editor --world world --assets editor_assets \"$@\"\n", exe=True)
        add_text(z, TOP + "/LISEZMOI-README.txt", README, crlf=windows)
        for fname in ("LICENSE", "NOTICE.md", "README_PORT.md", "CHANGELOG.md"):
            if os.path.exists(os.path.join(ROOT, fname)):
                add_file(z, os.path.join(ROOT, fname), "%s/%s" % (TOP, fname))
        for sub in ("docs", "world"):                                    # le monde sert a l'editeur ; la doc a tout le monde
            for base, _, files in os.walk(os.path.join(ROOT, sub)):
                for f in files:
                    if f.endswith((".md", ".txt", ".png")) or sub == "world":
                        full = os.path.join(base, f)
                        add_file(z, full, "%s/%s" % (TOP, os.path.relpath(full, ROOT).replace(os.sep, "/")))
        for zp in (a.sd_zip, a.editor_assets_zip):                       # assets : memes fichiers que la carte SD
            if zp:
                with zipfile.ZipFile(zp) as src:
                    for info in src.infolist():
                        if not info.is_dir():
                            z.writestr("%s/%s" % (TOP, info.filename), src.read(info))
    print("->", a.out, "(%d Ko)" % (os.path.getsize(a.out) // 1024))


main()
