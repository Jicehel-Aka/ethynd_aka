# Version PC (le jeu sans l'éditeur)

`ethynd_pc` est le **jeu seul** sur Windows / Linux : une fenêtre 320×240 agrandie ×2, ×3 ou ×4. C'est un programme
distinct de l'éditeur (`ethynd_editor`) : deux dossiers (`platform_sdl/` et `editor/`), deux CMake, deux exécutables ;
le jeu ne contient aucun code d'édition.

## Ce que c'est — et ce que ce n'est pas
C'est le **même cœur de jeu** que sur la console (`shared/game` : cartes, combat, PNJ, dialogues, objectifs, langues),
les **mêmes fichiers d'assets** (la carte SD, en BGR565 natif) et le même pas de 30 ticks par seconde. Ce n'est **pas
un émulateur** de l'AKA : l'ESP32, l'écran, le mixeur audio `gb_audio` et le décodeur MP3 (`minimp3`) ne sont pas
simulés. Conséquences :

| | Console AKA | PC |
|---|---|---|
| Cœur du jeu, histoire, collisions, langues | oui | **identique** |
| Rendu | `gb_graphics` (LCD) | tampon logiciel 320×240 envoyé à SDL |
| Audio | `gb_audio_track_mp3` / `_wav` (4 pistes) | SDL2_mixer : **ne teste pas** la piste MP3 de la console |
| Menu système (pause, volumes, langue, crédits) | oui (`aka_runtime`) | non : langue par `--lang`, son coupé avec **M**, Échap quitte |
| Fluidité / charge CPU | à mesurer sur le matériel | sans rapport avec la console |

À quoi ça sert : jouer et tester l'histoire, les cartes et les traductions sans la console, déboguer, montrer le jeu.
Ça ne prouve **rien** sur les performances, le MP3 ou l'affichage réel de la console.

## Compiler
Prérequis : CMake ≥ 3.16, SDL2 et SDL2_mixer (Linux : `libsdl2-dev libsdl2-mixer-dev` ; Windows : vcpkg
`sdl2 sdl2-mixer`). SDL2_image n'est pas nécessaire.
```
cmake -S platform_sdl -B build
cmake --build build --config Release
```
Les exécutables de la CI sont l'artefact `ethynd-pc-<système>` du workflow *Build PC (SDL)*.

## Lancer
Il faut les assets convertis (le contenu de `/Ethynd/` de la carte SD) : `python3 tools/dev.py assets <Ethynd-master>`
les produit dans `sdcard_files/Ethynd/`, ou télécharger l'archive du workflow *Package SD card assets*.
```
ethynd_pc [2|3|4] [--assets <dossier>] [--lang fr|en|es…] [--no-panel]
```
- zoom : **2 par défaut** (3 sans panneau) ; fenêtre de 640×720 avec le panneau en zoom 2, 960×960 en zoom 3 ;
- `--lang` : une langue de `world/lang/` présente dans les assets (repli sur le français) ;
- `--no-panel` : fenêtre de jeu seule, sans les boutons ;
- sans `--assets`, il cherche `Ethynd/` à côté de l'exécutable, puis `../sdcard_files/Ethynd`, puis le dossier courant.

## Le panneau de boutons (sous la zone de jeu)
Sous l'image du jeu, la fenêtre affiche le **nom du jeu** (ETHYND, « Version PC » à droite), une **croix directionnelle** et les
boutons **A B C D** dessinés, chacun avec le **rappel des touches du clavier** et de ce qu'il fait ; une ligne en bas rappelle M (son)
et Échap (quitter). Le panneau est **vivant** :
- un bouton **s'allume** quand la touche correspondante est pressée au clavier ;
- on peut **cliquer** les boutons à la souris (maintenir = rester appuyé, glisser d'un bouton à l'autre) ;
- sur un **écran tactile**, plusieurs doigts à la fois (ex. croix + A) ; les clics synthétisés par le tactile ne sont pas comptés deux fois ;
- sa langue suit celle du jeu (clés `pc.*` de `world/lang/<code>.txt`, donc traduisible comme le reste).

| Touche | Action |
|---|---|
| flèches ou ZQSD / WASD (position physique des touches) | se déplacer |
| X, Espace ou Entrée | A : attaquer, parler, valider |
| Retour arrière ou B | B : journal des objectifs, retour |
| C ou H | C : aide (depuis le menu) |
| F3 ou Tab | D : diagnostic (FPS, ticks par seconde) |
| M | couper / remettre le son |
| Échap | quitter |

## Problèmes courants
- *« Assets introuvables »* : passer `--assets` ; le message de l'erreur nomme le fichier manquant (`Missing: …`).
- *Pas de musique sous Windows* : SDL2_mixer de vcpkg sans décodeur MP3 ; le jeu l'écrit dans la console et continue
  (effets sonores WAV conservés).
- *Caractères « ? »* : la langue utilise un caractère que la police ne dessine pas ; `dev.py check` le signale.
- *Fenêtre trop petite / grande* : changer le zoom (2, 3 ou 4) ; `--no-panel` la raccourcit de 240 px.

## État
Le panneau (dessin, zones tactiles, suivi des pointeurs) est testé sans fenêtre (`host_test/panel_test.cpp`, avec captures) ; la fenêtre SDL elle-même
(rendu sous le jeu, clics et doigts réels) n'a jamais été ouverte. Syntaxe vérifiée, build Windows déjà exercé par la CI (un bug réel trouvé et corrigé) ; jamais ouvert en fenêtre dans
l'environnement de rédaction. Voir `docs/SUIVI.md`.
