#!/bin/bash
# Genere tout ce qui derive du depot Ethynd d'origine :
#   - shared/game/EthyndData.h  (animations / monstres, depuis les constantes Python)
#   - sdcard_files/Ethynd/      (tuiles, cartes, sprites, menus, audio convertis)
# Usage : tools/build_assets.sh <dossier Ethynd-master> [tile=24]
set -e
HERE="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${1:?usage: build_assets.sh <dossier Ethynd-master> [tile]}"
TILE="${2:-24}"
python3 "$HERE/tools/gen_data.py" "$SRC" "$HERE/shared/game/EthyndData.h"
python3 "$HERE/tools/convert_assets.py" "$SRC" "$HERE/sdcard_files/Ethynd" --tile "$TILE" --audio
echo "Copier sdcard_files/Ethynd sur la carte SD (racine : /Ethynd), et sdcard_files/Ethynd/lang avec."
