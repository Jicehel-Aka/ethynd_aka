# Données du jeu (`world/`)

Tout ce qui définit le monde est dans des fichiers texte, hors du code. Formats détaillés (et lecteur de
référence) : `tools/world_io.py`. Coordonnées en **pixels logiques** (32 px par case).

```
world/
  world.txt         start <carte> <x> <y> <direction>      (centre de la hitbox du joueur)
                    maps <carte> <carte> …
  tiles.txt         collide <id> …    anim <id> <id suivant>      (ids du tileset d'origine, 3800 tuiles)
  maps/<nom>.txt    carte : tuiles + objets
  story.txt         drapeaux, objets, dialogues, objectifs  (clés de traduction uniquement)
  lang/<code>.txt   clé = texte
```

## Une carte
```
map maison
size 12 17
music maison                    # none menu maison aventure grotte
layer 0 … layer 3               # 4 couches de CSV (-1 = vide) ; la couche 3 passe devant le joueur
spawn chauve_souris 484 384 10 2     # type x y vie attaque   (attaque 0 = inoffensif ; dégâts = attaque / 2, au moins 1)
spawn dragon_rouge 202 622 60 4 170  # + rayon : GARDIEN DE ZONE (boss), voir plus bas
npc sage sage 224 128 droite has:crystal?sage_thanks;talked_sage?sage_wait;sage_intro
door 294 421 46 13 grotte 736 433 talked_sage deny_cave_house   # x y l h carte dx dy condition refus
item potion 64 352
item crystal 96 640 dragon          # + objectif : l'objet n'apparaît qu'une fois cet objectif terminé
```
- **Monstres** : `chauve_souris`, `chat`, `oiseau`, `poussin`, `dragon_rouge`. **PNJ** : `sage`, `gardien`, `joueur`.
- **Porte** : quand le centre de la hitbox du joueur entre dans le rectangle, il passe sur `carte` en (`dx`,`dy`).
  Si `condition` (un drapeau) n'est pas posé, le dialogue `refus` s'affiche à la place. `-` = aucune.
- **PNJ** : les règles sont essayées dans l'ordre ; la première vraie gagne (`drapeau`, `!drapeau`, `has:objet`,
  ou un dialogue seul = par défaut). Un PNJ est un obstacle ; **A** devant lui lance le dialogue.

- **Gardien de zone (boss)** : un monstre avec un `rayon` > 0 poursuit le joueur quand celui-ci est à moins de `rayon + 48` px de son point
  d'attache (et à moins de 240 px de lui) ; sinon il rôde au hasard ; il ne s'éloigne **jamais** de plus de `rayon` px de son point d'attache.
  Sa hitbox est celle de son sprite (56 px pour le dragon, 32 px pour les autres). Insensible à l'immunité d'épée ; une fois vaincu il ne
  réapparaît plus. Le test d'atteignabilité vérifie qu'on peut arriver jusqu'à lui.
- **Objet conditionné** : `item <id> x y <objectif>` : invisible et non ramassable tant que l'objectif n'est pas terminé (ex. le Cristal et le dragon).

## L'histoire (`story.txt`)
```
flag talked_sage
itemdef crystal item_crystal 0 item.crystal        # id icône soin clé-du-nom  (soin > 0 : consommé sur place)

dialogue sage_intro
say npc.sage sage.intro.1                          # say <clé du locuteur|-> <clé du texte>
set talked_sage                                    # actions exécutées à la fin : set, give, take, heal, win

objective bats enter_cave kill:chauve_souris:4 obj.bats
```
Conditions d'objectif : `flag:<drapeau>` · `kill:<monstre>:<n>` · `item:<objet>:<n>` · `map:<carte>` ·
`reach:<carte>:<x>:<y>:<rayon>`. Le 2ᵉ champ (`enter_cave`) est l'objectif qui doit être terminé avant que celui-ci
apparaisse (`-` = visible dès le départ). `win` termine la partie (écran de victoire).
Limites : 64 drapeaux, 16 objets, 32 objectifs, 8 types de monstres comptés.

## Ajouter un nouveau *type* de PNJ
Les types sont des recoloriages du personnage du joueur, déclarés dans `tools/npc_defs.py` (nom + échange de palette).
Après ajout : `python3 tools/gen_data.py <Ethynd-master> shared/game/EthyndData.h` puis `tools/dev.py assets` ; le type
apparaît dans l'éditeur (touche N, puis `,` `.`). Un animal (`chat`, `oiseau`…) peut aussi servir de PNJ.

## Ajouter un PNJ, du début à la fin
1. Poser un PNJ avec l'éditeur (**N**), puis **P** pour son `id`, son type et ses règles.
2. Dans `story.txt` : un `dialogue` avec des `say` (clés) et, si besoin, des `set` / `give`.
3. Dans `lang/fr.txt` (et les autres langues) : le texte de chaque clé.
4. `python3 tools/check_world.py` vérifie que tout se tient, puis `tools/build_assets.sh`.
