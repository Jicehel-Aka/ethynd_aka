#!/bin/bash
# Compile et lance le test PC de shared/game.
# Usage : ./build_and_run.sh <dossier assets convertis (--tile 24)> [dossier captures]
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
S="$HERE/../shared"
ASSETS="${1:-out24}"
SHOTS="${2:-shots}"
mkdir -p "$SHOTS"
SAN=""; [ -n "$SANITIZE" ] && SAN="-g -fsanitize=address,undefined -fno-omit-frame-pointer"
g++ -std=gnu++17 -O1 -Wall -Wextra $SAN -DETHYND_HOST_TEST \
    -I"$S/platform" -I"$S/game" -I"$HERE" ${AKA_FONT_INC:+-I"$AKA_FONT_INC"} \
    "$HERE/host_main.cpp" "$S/game/Game.cpp" "$S/game/GameMap.cpp" \
    "$S/game/Characters.cpp" "$S/game/AssetStore.cpp" "$S/game/World.cpp" "$S/game/Lang.cpp" -o "${TMPDIR:-/tmp}/ethynd_test"
"${TMPDIR:-/tmp}/ethynd_test" "$ASSETS" "$SHOTS" "${@:3}"
