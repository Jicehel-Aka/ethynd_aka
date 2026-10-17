# Outils

Tout passe par **`python3 tools/dev.py`** (Windows, Linux, macOS : Python 3 suffit ; Pillow pour les images,
ffmpeg pour l'audio, g++ pour les tests locaux). Les scripts individuels restent utilisables.

| Commande | Rôle |
|---|---|
| `dev.py check` | vérifie le monde, les traductions et leur cohérence avec le code (`check_world.py`) |
| `dev.py assets <Ethynd-master>` | génère `sdcard_files/Ethynd/` (carte SD) et `editor_assets/` (éditeur) |
| `dev.py test <Ethynd-master>` | compile et lance les tests du jeu et de l'éditeur (g++, sans écran) |
| `dev.py maps <Ethynd-master>` | aperçus PNG des cartes avec portes, monstres, PNJ, objets → `docs/maps/` |
| `dev.py package <Ethynd-master>` | fabrique `dist/Ethynd_sdcard.zip` et `dist/Ethynd_editor_assets.zip` |
| `dev.py lang new es` | crée `world/lang/es.txt` (toutes les clés, valeurs vides à traduire) |
| `dev.py lang export [f.csv]` / `lang import f.csv` | échange avec un tableur (colonnes = langues) |
| `dev.py lang status` | pourcentage traduit par langue |

Le dépôt d'origine (`github.com/ProjetIsn2019/Ethynd`) fournit images, sons et constantes d'animation.

## Scripts
| Script | Rôle |
|---|---|
| `tools/check_world.py` | références (portes, PNJ, objets, dialogues, objectifs), traductions manquantes, clés du code absentes de `fr.txt`, **caractères que la police ne dessine pas**, textes trop longs. `--strict` : les avertissements deviennent des erreurs |
| `tools/convert_assets.py` | tuiles, cartes, sprites, menus, audio, langues, `world.bin` → fichiers de la SD ; `--editor-out` pour l'éditeur |
| `tools/gen_data.py` | `shared/game/EthyndData.h` (animations) depuis les constantes Python d'Ethynd |
| `tools/npc_defs.py` | variantes de PNJ (palettes) : source unique de `gen_data.py` et `convert_assets.py` |
| `tools/import_ethynd.py` | import **unique** d'Ethynd (CSV + constantes) vers `world/` ; ensuite `world/` est la source de vérité |
| `tools/world_io.py` | lecture/écriture/validation du format texte du monde |
| `tools/translations_csv.py` | `export`, `import`, `new` (voir `docs/MULTILANGUE.md`) |
| `tools/render_maps.py` | aperçus PNG des cartes (`--grid` : coordonnées en cases) |
| `tools/package_pc.py` | archive PC autonome (jeu, éditeur, DLL, assets, monde, docs, lanceurs) — utilisée par `build-pc.yml` et `release.yml` |
| `tools/release_notes.py` | notes de release tirées de `CHANGELOG.md` |
| `tools/fetch_minimp3.sh` | télécharge `minimp3.h` (CC0) pour la console |
| `tools/build_assets.sh` | équivalent bash de `dev.py assets` |
| `host_test/build_and_run.sh`, `editor/build_and_run_test.sh` | tests (g++), `SANITIZE=1` pour ASan + UBSan |
| `host_test/CMakeLists.txt` | mêmes tests avec CMake/CTest (voir ci-dessous) |

## Tests avec CMake / CTest (Windows compris)
```
python3 tools/dev.py assets <Ethynd-master>      # produit sdcard_files/Ethynd et editor_assets
cmake -S host_test -B build_test -DETHYND_ASSETS=sdcard_files/Ethynd -DETHYND_EDITOR_ASSETS=editor_assets
cmake --build build_test --config Release
ctest --test-dir build_test -C Release --output-on-failure
```
(Fichier non exécuté dans l'environnement de rédaction : cmake y était absent. La CI Linux utilise les scripts bash.)

## Ce que le test du jeu mesure
Panneau de boutons PC (zones tactiles, pointeurs multiples, captures), boucle à pas fixe (**un appui = un seul front montant**, quelle que soit la cadence du rendu, avec un témoin qui reproduit l'ancienne
boucle défectueuse), diagnostic (bouton D), histoire complète (menu, langues, porte verrouillée, sage, journal, grotte, 4 chauves-souris, Cristal, victoire, mort,
nouvelle partie), bord de carte, fichier manquant, **atteignabilité** de chaque objet, porte et PNJ (parcours sur le réseau
exact des pas du joueur) et **budget de rendu** : pire cas par carte — aujourd'hui 497 appels `drawImage` et
≈ 370 000 pixels parcourus par image sur l'aventure (≈ 4,8 fois l'écran), 291 sur la grotte, 213 sur la maison.
