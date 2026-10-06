#!/bin/bash
# Compile et lance le test sans ecran de l'editeur (aucune fenetre, pas de SDL).
# Usage : ./build_and_run_test.sh <dossier world> <dossier assets editeur> [dossier captures]
#   (assets editeur : sortie de convert_assets.py --editor-out)
# SANITIZE=1 ajoute AddressSanitizer + UBSan.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
S="$HERE/../shared"
WORLD="${1:?usage: build_and_run_test.sh <world> <assets editeur> [captures]}"
ASSETS="${2:?assets editeur manquants}"
SHOTS="${3:-shots}"
mkdir -p "$SHOTS"
SAN=""; [ -n "$SANITIZE" ] && SAN="-g -fsanitize=address,undefined -fno-omit-frame-pointer"
g++ -std=gnu++17 -O1 -Wall -Wextra $SAN \
    -I"$HERE" -I"$S/platform" -I"$S/game" -I"$HERE/../components/aka_font/include" \
    "$HERE/test_editor.cpp" "$HERE/EditorCore.cpp" "$HERE/WorldText.cpp" -o "${TMPDIR:-/tmp}/ethynd_editor_test"
"${TMPDIR:-/tmp}/ethynd_editor_test" "$WORLD" "$ASSETS" "${TMPDIR:-/tmp}/ethynd_editor_work" "$SHOTS"
