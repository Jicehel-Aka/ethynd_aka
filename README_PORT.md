# Ethynd sur Gamebuino AKA

Portage du RPG Pygame **Ethynd** (ProjetIsn2019/Ethynd, Unlicense) vers la Gamebuino AKA
(ESP32-S3). Affichage en 24 px (assets d'origine en 32 px), logique de jeu en unites de 32 px.

## Contenu et installation
Le dossier est un projet ESP-IDF COMPLET : il contient deja les composants `gamebuino` (avec la
piste MP3 `gb_audio_track_mp3` ajoutee), `aka_runtime` et `aka_font`, repris du projet
Dark & Under (couleur) -- ils gardent leurs licences d'origine.

Un seul fichier n'est pas dans l'arbre : **`minimp3.h`** (decodeur MP3, domaine public CC0).
`tools/fetch_minimp3.sh` le telecharge dans `components/gamebuino/include_lib/` (la CI le fait
toute seule). Puis : ESP-IDF 5.5.1, cible esp32s3, `idf.py build`
(le workflow `.github/workflows/build-aka.yml` fait la meme chose).

Les assets du jeu (tuiles, cartes, sprites, menus, audio : ~10 Mo) ne sont pas dans l'arbre
source : voir "Carte SD".

## Carte SD
Decompresser `Ethynd_sdcard.zip` a la racine de la SD : `/sdcard/Ethynd/` (tuiles, cartes,
sprites, menus, audio, langues). Pour le regenerer depuis le depot d'origine :
`tools/build_assets.sh <dossier Ethynd-master> [24]` (necessite Python, Pillow, ffmpeg).

## Commandes
| | Console AKA | PC (SDL) |
|---|---|---|
| Se deplacer | croix directionnelle | fleches ou ZQSD |
| Attaquer / jouer / valider | **A** | X, Espace ou Entree |
| Aide (depuis le menu) | **C** | C ou H |
| Retour (aide) / menu (apres la mort) | **B** (ou A apres la mort) | Retour arriere ou B |
| Quitter | menu systeme (MENU) | Echap |
| Couper le son | menu systeme (volume) | M |

Les images de menu d'origine parlaient des touches du PC (J, a, q, Echap, X) : le convertisseur
les efface et le jeu redessine les textes (francais ou anglais, selon la langue du menu systeme).

## Build PC (SDL)
`cmake -S platform_sdl -B build && cmake --build build` (SDL2 + SDL2_mixer ; MP3 pris en charge
par SDL2_mixer). Lancer : `ethynd_pc [2|3|4] [--assets <dossier>] [--lang fr|en]`. Les assets sont
les memes fichiers que sur la SD (dossier `Ethynd` a cote de l'executable, ou `--assets`).
Meme coeur de jeu que la console (`shared/game`).

## Test sur PC (sans console)
`host_test/build_and_run.sh <dossier assets converti avec --tile 24 --order bgr>` compile
le coeur du jeu avec un renderer logiciel, rejoue un scenario (menu, marche, collisions,
attaque, teleportations, monstres, mort) et enregistre des captures PPM.

## Ecarts volontaires avec Ethynd
- Seuls les monstres dont `attaque` > 0 blessent (l'original blessait aussi avec les animaux).
- Un monstre mort n'a plus de hitbox.
- Apres l'ecran de mort : retour au menu.
- Collision des monstres testee en coordonnees monde (au plus 3 px d'ecart avec l'original).

## Ce qui a ete verifie, et ce qui ne l'a pas ete
Verifie (PC, sans console) : le coeur du jeu compile sans avertissement (-Wall -Wextra -Werror)
et rejoue tout un scenario sous AddressSanitizer + UBSan sans erreur ; les fichiers AKA et SDL
passent une verification de syntaxe (en-tetes gamebuino reels, stubs pour FreeRTOS, minimp3
et SDL).
**Non verifie** : la compilation ESP-IDF complete et le lien (ESP-IDF absent de mon
environnement), la compilation SDL contre les vraies bibliotheques, et tout ce qui touche au
materiel. Lancer `idf.py build` et la CI est la vraie verification.

## A verifier sur la console (non teste sur materiel)
- Fluidite du rendu (environ 14x10 tuiles x 4 couches dessinees tuile par tuile).
- Charge CPU du decodage MP3 (`GB_MP3_TASK_PRIO`, `GB_MP3_TASK_STACK`).
- `joueur_marche.wav` fait 4,6 Mo (longue boucle de pas) : a raccourcir si besoin.
