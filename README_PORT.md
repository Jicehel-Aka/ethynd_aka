# Ethynd sur Gamebuino AKA

Portage du RPG Pygame **Ethynd** (ProjetIsn2019/Ethynd, licence Unlicense) vers la Gamebuino AKA
(ESP32-S3), avec une version PC (SDL2), un éditeur de cartes, des dialogues, des objectifs et un
jeu entièrement **multilingue**.

| Dossier | Contenu |
|---|---|
| `shared/game`, `shared/platform` | le cœur du jeu (identique sur console et PC) et ses interfaces |
| `components/`, `main/` | version **console AKA** (ESP-IDF) : `gamebuino`, `aka_runtime`, `aka_font` (repris de Dark & Under) + `platform_aka` |
| `platform_sdl/` | version **PC** (Windows / Linux, SDL2) |
| `editor/` | **éditeur de cartes** PC (SDL2) |
| `world/` | **les données du jeu** : cartes, monstres, PNJ, portes, objets, histoire, **langues** |
| `tools/` | convertisseur d'assets, vérificateur de traductions, scripts |
| `host_test/` | test du jeu sans écran (histoire complète, portes, langues, atteignabilité) |
| `.github/workflows/` | CI : build AKA, build PC + éditeur, tests, paquet de la carte SD |
| `docs/` | `SUIVI.md` (**état du projet, backlog**), `OUTILS.md`, `PC.md` (version PC), `TEST_AKA.md` + `RAPPORT_TEST_AKA.md` (**premier test sur la console**), `RELEASE.md` (**faire une release**), `EDITEUR.md`, `MULTILANGUE.md`, `DONNEES.md`, `maps/` (aperçus) |
| `CHANGELOG.md`, `NOTICE.md` | journal des modifications ; licences et crédits |

## Le jeu

**Début.** Menu → **A** (Jouer) → écran de chargement → on se réveille dans la maison, face au bas
de l'écran. En haut s'affichent la barre de vie et l'objectif en cours : *« Parler au sage dans la
maison »*. Le **sage** (cheveux blancs, robe verte) est à deux pas à gauche : on s'approche, une bulle
**A** apparaît au-dessus de lui, on appuie sur **A**.

**Histoire.** Des chauves-souris ont volé le **Cristal de lumière** du village et l'ont apporté à un **dragon rouge**,
tapi au fond de la grotte ; sans lui, la nuit tombera. Le sage envoie le joueur dans la grotte, par l'escalier de la cave
(au fond à droite de la maison). Cet escalier et l'entrée de la grotte côté village restent **verrouillés** (un message
l'explique) tant que le sage n'a pas parlé. Objectifs, dans l'ordre :
1. Parler au sage dans la maison
2. Entrer dans la grotte
3. Vaincre 4 chauves-souris (compteur affiché : « (2/4) »)
4. **Vaincre le dragon rouge** (le boss, voir ci-dessous)
5. Retrouver le Cristal de lumière (il n'apparaît qu'une fois le dragon vaincu)
6. Rapporter le Cristal au sage → écran de victoire

**Le dragon** monte la garde dans le coin sud-ouest de la grotte, autour du Cristal. Il rôde dans sa zone, **poursuit le joueur
dès qu'il y entre** (plus lent que lui : on peut le semer) et **ne la quitte jamais**. Il fait 2 points de dégâts par contact,
et recommence toutes les 1,5 s tant que le contact dure ; il a 60 points de vie (une potion est posée avant sa salle). Son cri est
celui d'Ethynd (`monstre_dragon_rouge.wav`). Vaincu, il ne réapparaît plus.

Le long du chemin : des **potions de soin** (ramassées seulement si la vie n'est pas pleine, sinon elles
restent là), un **gardien** près du château qui donne un indice, le **journal des objectifs** (bouton
**B**) avec l'inventaire. Les chauves-souris blessent ; les animaux (chat, oiseaux, poussins) sont inoffensifs.

## Commandes

| | Console AKA | PC (SDL) |
|---|---|---|
| Se déplacer | croix directionnelle | flèches ou ZQSD |
| Attaquer / parler / valider | **A** | X, Espace ou Entrée |
| Journal des objectifs | **B** | Retour arrière ou B |
| Diagnostic (FPS, mémoire) | **D** | F3 ou Tab |
| Aide (depuis le menu) | **C** | C ou H |
| Quitter | menu système (MENU) | Échap |
| Couper le son | menu système (volume) | M |

## Multilingue

**Aucun texte n'est écrit dans le code ni dans les images.** Dialogues, objectifs, noms d'objets,
menus, aide, écrans de mort et de victoire, messages : tout est une *clé de traduction* lue dans
`world/lang/<code>.txt`. **Une langue = un fichier** (`fr.txt`, `en.txt` fournis ; ajoutez `es.txt`,
`de.txt`…). Le jeu suit la langue du menu système de l'AKA ; sur PC : `--lang es`. Une clé absente d'une
langue retombe sur le français ; une langue inconnue aussi. Détails : `docs/MULTILANGUE.md`.

Polices : ASCII (anglais) et accents français, espagnols, allemands, italiens et portugais. Les autres
alphabets (cyrillique, grec, asiatiques, polonais…) ne sont pas disponibles ; `tools/dev.py check` signale tout
caractère que la police ne sait pas dessiner **avant** qu'il s'affiche en « ? ».

## Installation

### Carte SD (obligatoire pour la console)
1. Fabriquer les assets : `tools/build_assets.sh <dossier Ethynd-master>` (Python 3, Pillow, ffmpeg ;
   le dépôt d'origine : github.com/ProjetIsn2019/Ethynd), **ou** lancer le workflow GitHub
   *Package SD card assets* et télécharger l'archive.
2. Copier le contenu de `sdcard_files/Ethynd/` dans `/Ethynd/` sur la carte SD (ou décompresser
   `Ethynd_sdcard.zip` à la racine).

### Console AKA
Le dossier est un projet ESP-IDF complet (composants inclus). Un seul fichier manque : **`minimp3.h`**
(décodeur MP3, domaine public) → `tools/fetch_minimp3.sh` le télécharge (la CI le fait seule). Puis
ESP-IDF 5.5.1, cible `esp32s3`, `idf.py build`.

### PC
```
cmake -S platform_sdl -B build && cmake --build build
build/ethynd_pc [2|3|4] [--assets sdcard_files/Ethynd] [--lang fr|en|es...]
```
(SDL2 et SDL2_mixer ; les MP3 passent par SDL2_mixer.) C'est le jeu **seul**, sans éditeur, et pas un émulateur
de l'AKA : détails, touches et limites dans `docs/PC.md`. La fenêtre affiche, **sous le jeu, le nom du jeu, des boutons dessinés
(croix + A B C D, cliquables à la souris et au doigt) et le rappel des touches du clavier**, traduits (`--no-panel` pour les masquer).

### Éditeur de cartes
```
cmake -S editor -B build_editor && cmake --build build_editor
build_editor/ethynd_editor --world world --assets editor_assets --lang fr
```
`editor_assets/` est produit par `tools/build_assets.sh`. Manuel : `docs/EDITEUR.md`.

## Travailler sur le jeu

Tout passe par **`python3 tools/dev.py`** (Python suffit, sur Windows aussi ; détail dans `docs/OUTILS.md`) :

| | |
|---|---|
| `dev.py check` | vérifie références, traductions, clés du code, polices, longueurs |
| `dev.py assets <Ethynd-master>` | régénère la carte SD et les assets de l'éditeur |
| `dev.py test <Ethynd-master>` | rejoue le jeu et l'éditeur sans écran |
| `dev.py maps <Ethynd-master>` | aperçus PNG des cartes (`docs/maps/`) |
| `dev.py lang new|export|import|status` | langues et échange avec un tableur |
| `dev.py package <Ethynd-master>` | `dist/Ethynd_sdcard.zip` et `dist/Ethynd_editor_assets.zip` |

Cycle habituel : éditer la carte (éditeur ou `world/maps/*.txt`), l'histoire (`world/story.txt`), les textes
(`world/lang/*.txt`) → `check` → `assets` → copier sur la SD.

## Tests et vérifications

- `host_test/build_and_run.sh out24 shots` (`SANITIZE=1` pour AddressSanitizer + UBSan) : rejoue
  l'histoire complète sans écran : menu, langues, porte verrouillée, sage, journal, grotte, 4 chauves-souris,
  Cristal, victoire, mort, nouvelle partie, **et vérifie que tout objet, porte et PNJ est atteignable** à pied
  (parcours sur le réseau exact des pas du joueur).
- `editor/build_and_run_test.sh world editor_assets` : peinture, annuler/rétablir, objets, saisie de propriétés
  et leurs refus, nouvelle carte, sauvegarde **identique octet pour octet** à celle de Python, langues.
- CI : `.github/workflows/tests.yml` exécute tout cela à chaque envoi.

### Ce qui est vérifié, et ce qui ne l'est pas
Le détail, composant par composant, est dans **`docs/SUIVI.md`** (avec le backlog et les problèmes connus). En bref :
- ✅ **testé sur PC, sans console** : cœur du jeu (histoire complète, portes, quêtes, langues, atteignabilité, bord de
  carte, budget de rendu) et cœur de l'éditeur, sous AddressSanitizer + UBSan ; code propre en `-Wall -Wextra -Wshadow
  -Wpedantic` (gnu++23, le standard d'ESP-IDF 5.x).
- 🟡 **syntaxe seulement** : fichiers de la console (avec les vrais en-têtes `gamebuino`), de SDL, fenêtre de l'éditeur,
  piste MP3. Le build Windows a déjà révélé un bug réel (corrigé) : la CI est la vraie vérification.
- ⬜ **jamais exécuté** : la compilation ESP-IDF complète, tout ce qui touche au matériel (fluidité, MP3, accents, audio),
  CTest.

## Écarts volontaires avec Ethynd
- Seuls les monstres dont `attaque` > 0 blessent (l'original blessait aussi avec les animaux).
- Un monstre mort n'a plus de hitbox. Deux chauves-souris de l'original apparaissaient *dans* un mur
  (bloquées pour toujours) : elles sont décalées.
- Après l'écran de mort : retour au menu. Les portes ne se déclenchent qu'après avoir quitté la zone d'arrivée.
- Combat : l'immunité de l'épée (une épée qui touche un monstre annule ses dégâts) ne joue que **pendant** l'attaque (l'original gardait le
  dernier rectangle d'épée) et **jamais contre un boss** ; un contact prolongé blesse toutes les 1,5 s (un seul coup par contact dans l'original) ;
  dégâts = attaque / 2 (au moins 1).
- Le bord de la carte est infranchissable (l'original laissait sortir dans le vide) ; les monstres ne traversent pas les PNJ.
- La collision des monstres est testée en coordonnées monde (au plus 3 px d'écart avec l'original).
- Affichage en 24 px (assets d'origine en 32 px, logique de jeu en unités de 32 px, comme l'original).

## À contrôler sur la console
Fluidité (≈14×10 cases × 4 couches), charge CPU du décodage MP3 (`GB_MP3_TASK_PRIO`, `GB_MP3_TASK_STACK`),
boucles de musique, affichage des accents, 4 canaux audio.

## Licences
Code du portage : **MIT** (`LICENSE`). Code et assets d'Ethynd : Unlicense (domaine public) — musiques : dcinoot
(utilisation confirmée ; prévenir le chef du projet d'origine quand le portage est montrable). Composants `gamebuino`,
`aka_runtime`, `aka_font` : licences de Dark & Under. `minimp3` : CC0. Détail : `NOTICE.md`.
