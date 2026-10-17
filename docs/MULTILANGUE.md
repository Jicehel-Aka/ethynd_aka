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
1. `python3 tools/dev.py lang new es` : crée `world/lang/es.txt` avec **toutes** les clés et des valeurs **vides**.
   Une valeur vide = « pas encore traduit » : le jeu affiche le français à la place et `check` la compte comme manquante.
2. Traduire (après le `=`) dans un éditeur de texte, **ou** passer par un tableur :
   `dev.py lang export` → un CSV (une colonne par langue) → le traducteur remplit la colonne `es` →
   `dev.py lang import traductions.csv` (met à jour les fichiers en gardant commentaires et ordre).
3. `dev.py lang status` (ou `check`) : pourcentage traduit par langue, clés manquantes, **caractères que la police ne
   dessine pas**, textes trop longs pour leur emplacement.
4. `dev.py assets …` (ou le workflow *Package SD card assets*) : produit `lang/es.bin` (le jeu) et `lang/es.json`
   (menu système de l'AKA) dans `sdcard_files/Ethynd/lang/`. Copier sur la SD.
5. Choisir « es » dans le menu Langue du système AKA ; sur PC : `--lang es`.

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
| `pc.*` | panneau de boutons de la version PC (touches du clavier et leur rôle) |

## Nouveaux dialogues et objectifs
Voir `docs/DONNEES.md` : `story.txt` ne contient que des clés ; on ajoute la clé dans **chaque** langue.
`check_world.py` signale une clé utilisée dans l'histoire mais absente de `fr.txt` (erreur) ou d'une autre
langue (avertissement).

## Limites
- **Polices** : ASCII + accents français, espagnols, allemands (ä ö ü ß), italiens (ì ò), portugais (ã õ), « » ¿ ¡ °.
  `œ` s'affiche « oe ». Pas de cyrillique, de grec, d'écritures asiatiques ni de polonais/tchèque/turc (ł č ğ…).
  Un caractère non pris en charge s'affiche « ? » : `check` le signale avant que vous le voyiez à l'écran.
  Ajouter un caractère : `components/aka_font/tools/generate_extended_font.py` (accents composés automatiquement).
- **Longueur** : titre de bouton large de ≈ 14 caractères (réduit automatiquement sinon) ; objectif en cours
  tronqué à 40 caractères dans le bandeau (le journal l'affiche en entier).
- Une clé affichée telle quelle à l'écran (`sage.intro.7`) = traduction à ajouter dans `fr.txt`.
