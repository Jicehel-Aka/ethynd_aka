#!/usr/bin/env python3
"""Ecrit sur la sortie standard les notes d'une release, tirees de CHANGELOG.md.
Usage : python3 tools/release_notes.py [--tag v0.1.0] [--sections N]
Par defaut TOUT l'historique (premiere release) ; --sections 1 : seulement la section la plus recente (releases suivantes)."""
import os
import sys

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
tag = sys.argv[sys.argv.index("--tag") + 1] if "--tag" in sys.argv else ""
lines = open(os.path.join(root, "CHANGELOG.md"), encoding="utf-8").read().split("\n")
heads = [i for i, l in enumerate(lines) if l.startswith("## ")]
if not heads:
    sys.exit("CHANGELOG.md : aucune section")
n = int(sys.argv[sys.argv.index("--sections") + 1]) if "--sections" in sys.argv else len(heads)
start = heads[0]
end = heads[n] if n < len(heads) else len(lines)
print("# Ethynd sur Gamebuino AKA %s\n" % tag)
print("\n".join(lines[start:end]).strip())
print("""
---
**Contenu de cette release**
- `Ethynd_sdcard.zip` : à décompresser à la racine de la carte SD de la console (`/Ethynd/`).
- `Ethynd_aka_firmware.zip` : firmware de la console (`ethynd_aka.bin`, bootloader, table de partitions).
- `Ethynd_PC_windows.zip`, `Ethynd_PC_linux.zip` : version PC autonome (jeu, éditeur de cartes, assets, documentation).
- `Ethynd_editor_assets.zip` : palette de tuiles de l'éditeur (déjà incluse dans les archives PC).

Licence MIT pour le code du portage ; voir `NOTICE.md` pour les crédits (Ethynd : Unlicense, musiques : dcinoot, composants Dark & Under, minimp3 : CC0).
Première utilisation : `docs/TEST_AKA.md` (console) et `docs/PC.md` (PC).""")
