#!/bin/bash
# Telecharge minimp3.h (decodeur MP3, domaine public CC0, github.com/lieff/minimp3)
# dans components/gamebuino/include_lib/. A lancer une fois avant "idf.py build".
set -e
HERE="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$HERE/components/gamebuino/include_lib/minimp3.h"
URL="https://raw.githubusercontent.com/lieff/minimp3/master/minimp3.h"
if command -v curl >/dev/null; then curl -fsSL "$URL" -o "$DEST"; else wget -q "$URL" -O "$DEST"; fi
grep -q "MINIMP3_IMPLEMENTATION" "$DEST" || { echo "minimp3.h telecharge semble invalide" >&2; rm -f "$DEST"; exit 1; }
echo "OK : $DEST"
