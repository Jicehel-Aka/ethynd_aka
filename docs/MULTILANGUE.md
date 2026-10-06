# Multilingue : ajouter ou corriger une langue

## Principe
Tout texte visible est une **clé** (`sage.intro.1`, `ui.menu.play`…). Le texte est dans
`world/lang/<code>.txt`, une ligne par clé :

```
sage.intro.1 = Ah, te voilà enfin ! Le village a besoin de toi.
ui.menu.play = Jouer
```
Pas de retour à la ligne à gérer : le jeu coupe les lignes tout seul (38 caractères par ligne dans la boîte de
dialogue, 4 lignes par page, plusieurs pages si besoin). `fr.txt` est la langue **par défaut** et la
référence : toute clé absente d'une autre langue s'affiche en français.

## Ajouter une langue (ex. espagnol)
1. Copier `world/lang/fr.txt` en `world/lang/es.txt` et traduire les valeurs (après le `=`). On peut
   commencer par une traduction partielle : le reste reste en français.
2. `python3 tools/check_world.py` : affiche, par langue, le pourcentage traduit et les clés manquantes.
3. `tools/build_assets.sh …` (ou le workflow *Package SD card assets*) : produit `lang/es.bin` (le jeu) et
   `lang/es.json` (menu système de l'AKA) dans `sdcard_files/Ethynd/lang/`.
4. Copier sur la carte SD. Choisir « es » dans le menu Langue du système AKA ; sur PC : `--lang es`.

Aucune modification de code. Le code de langue est celui du menu système de l'AKA (`getLanguage()`).

## Quelles clés ?
| Préfixe | Contenu |
|---|---|
| `npc.*`, `item.*` | noms de personnages et d'objets |
| `sage.*`, `gardien.*`, `deny.*` | dialogues (une clé par réplique) |
| `obj.*` | titres des objectifs |
| `ui.*` | menus, aide, HUD, journal, messages, écrans de mort et de victoire |
| `aka.*` | libellés du menu système de l'AKA (exportés en `.json`, pas dans le jeu) |
| `ed.*` | interface de l'éditeur de cartes (PC) |

## Nouveaux dialogues et objectifs
Voir `docs/DONNEES.md` : `story.txt` ne contient que des clés ; on ajoute la clé dans **chaque** langue.
`check_world.py` signale une clé utilisée dans l'histoire mais absente de `fr.txt` (erreur) ou d'une autre
langue (avertissement).

## Limites
- **Polices** : ASCII + accents français, espagnols, allemands (composant `aka_font`). Pas d'italien (ì ò),
  de portugais (ã õ), de cyrillique, de grec ni d'écritures asiatiques. `œ` s'affiche « oe ».
- **Longueur** : titre de bouton large de ≈ 14 caractères (réduit automatiquement sinon) ; objectif en cours
  tronqué à 40 caractères dans le bandeau (le journal l'affiche en entier).
- Une clé affichée telle quelle à l'écran (`sage.intro.7`) = traduction à ajouter dans `fr.txt`.
