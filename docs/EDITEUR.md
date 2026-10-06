# Éditeur de cartes

Éditeur PC (SDL2) pour les cartes, monstres, PNJ, portes, objets, point de départ et collisions des tuiles.
Il lit et écrit le format texte de `world/` (voir `docs/DONNEES.md`) ; ses fichiers sont identiques à ceux de
Python. Les dialogues et objectifs se modifient dans `world/story.txt` et `world/lang/*.txt` (l'éditeur les lit
pour vérifier les références).

```
ethynd_editor --world world --assets editor_assets [--lang fr|en|…] [--size 960x600] [--build "<commande>"]
```
`editor_assets/` (palette complète du tileset + sprites) est produit par `tools/build_assets.sh`.
`--build` : commande lancée par **F9** (ex. `bash tools/build_assets.sh ../Ethynd-master`).

## Dessiner
| Touche | Action |
|---|---|
| clic gauche | dessiner / sélectionner (selon l'outil) ; clic droit : pipette |
| **B** pinceau · **F** remplir · **R** rectangle (glisser) · **E** gomme · **I** pipette | outils de tuiles |
| **1–4** | couche active (0–2 : sol et décor, avec collisions ; 3 : devant le joueur) |
| **F1–F4** | afficher / masquer une couche · **G** grille · **C** repères de collision |
| molette · Maj+molette · Espace+glisser | défiler · défiler en horizontal · déplacer la vue |
| palette (à droite) | cliquer une tuile · molette ou PgUp/PgDn · **J** : aller à une tuile par son numéro |
| **X** | active/désactive la collision de la tuile choisie (enregistré dans `tiles.txt`) |

Repères : petit carré rouge = tuile à collision, point jaune dans la palette = tuile animée.

## Objets
| Touche | Action |
|---|---|
| **V** | sélectionner / déplacer (pas de 8 px) ; **M** monstre · **N** PNJ · **T** objet : un clic le place |
| **D** | porte : glisser un rectangle · **S** : point de départ du joueur |
| flèches (Maj : 32 px) · **Suppr** · **, .** | déplacer · supprimer · changer le type (monstre, PNJ, objet, carte de destination) |
| **P** | modifier les propriétés sur **une ligne de texte** (voir ci-dessous) ; Entrée valide, Échap annule |

Lignes de propriétés (les mêmes que dans le fichier, sans le mot-clé) ; une valeur invalide est **refusée**
avec un message :
- monstre : `type x y vie attaque` (attaque 0 = inoffensif)
- PNJ : `id type x y direction règles` — règles : `drapeau?dialogue;!drapeau?dialogue;has:objet?dialogue;dialogue`
- porte : `x y largeur hauteur carte dx dy drapeau|- dialogue|-` — `dx dy` = où arrive le joueur (centre de sa
  hitbox) ; `drapeau` = condition d'ouverture, `dialogue` = ce qui s'affiche si c'est fermé
- objet : `id x y` · départ : `x y direction`

Positions en **pixels logiques** (32 px par case, comme Ethynd) : une case (3, 5) commence en (96, 160).

## Cartes et fichiers
**Tab / Maj+Tab** carte suivante / précédente · **Ctrl+N** nouvelle carte (`nom largeur hauteur`, minuscules,
chiffres et `_`) · **Ctrl+M** musique · **Ctrl+Z / Ctrl+Y** annuler / rétablir (60 niveaux par carte) ·
**Ctrl+S** enregistrer · **F9** reconstruire le jeu · **H** aide. Fermer la fenêtre avec des modifications non
enregistrées demande confirmation (fermer une deuxième fois pour quitter).

## Après l'édition
`python3 tools/check_world.py` puis `tools/build_assets.sh` (ou le workflow GitHub) pour régénérer la carte SD.
Une nouvelle carte doit être reliée par une porte et utilisée par l'histoire pour servir dans le jeu.
