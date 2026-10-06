#!/bin/bash
# Genere tout ce qui derive du depot Ethynd d'origine et du monde (world/) :
#   - shared/game/EthyndData.h     animations des personnages (depuis les constantes Python)
#   - sdcard_files/Ethynd/         tuiles, cartes, sprites, menus, audio, LANGUES (a copier sur la SD)
#   - editor_assets/               palette complete + sprites pour l'editeur de cartes (PC)
# Usage : tools/build_assets.sh <dossier Ethynd-master> [tile=24]
# Prerequis : Python 3, Pillow, ffmpeg (audio). Le depot d'origine : github.com/ProjetIsn2019/Ethynd
set -e
HERE="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${1:?usage: build_assets.sh <dossier Ethynd-master> [tile]}"
TILE="${2:-24}"
python3 "$HERE/tools/check_world.py" "$HERE/world"
python3 "$HERE/tools/gen_data.py" "$SRC" "$HERE/shared/game/EthyndData.h"
python3 "$HERE/tools/convert_assets.py" "$SRC" "$HERE/sdcard_files/Ethynd" \
        --world "$HERE/world" --tile "$TILE" --audio --editor-out "$HERE/editor_assets"
rm -f "$HERE/sdcard_files/Ethynd/manifest.json"
echo
echo "Carte SD : copier le CONTENU de sdcard_files/Ethynd/ dans /Ethynd/ sur la carte SD."
echo "Editeur  : ethynd_editor --world world --assets editor_assets"
