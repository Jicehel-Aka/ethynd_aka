# Premier test sur la console AKA

Objectif : savoir **ce qui marche, ce qui ne marche pas, et à quelle vitesse**, avec le moins d'aller-retours possible.
Remplis `docs/RAPPORT_TEST_AKA.md` au fil du test et renvoie-le (avec le début du journal série en cas de problème).

## 0. Ce qu'il faut
- ESP-IDF **5.5.1**, cible `esp32s3`, la console branchée en USB (journal série : `idf.py monitor`).
- Une carte SD FAT32.
- Le projet complet (`ethynd_aka_source.zip`) et les assets (`Ethynd_sdcard.zip`).

## 1. Compiler
```
tools/fetch_minimp3.sh          # une seule fois : télécharge minimp3.h (décodeur MP3)
idf.py build
```
Une erreur de compilation est **attendue** (le code console n'a jamais été compilé avec ESP-IDF) : ne renvoie que la
**première** erreur avec ses 3 lignes de contexte, puis relance. Pour installer sur la console, procède comme pour Dark & Under
(la table de partitions `partitions.csv` est la même ; si ton lanceur en utilise une autre, garde la sienne).

## 2. Carte SD
Décompresser `Ethynd_sdcard.zip` **à la racine** : on doit obtenir

```
/Ethynd/tiles.bin  collide.bin  anim.bin  sprites.bin  world.bin
/Ethynd/maps/maison.map  aventure.map  grotte.map
/Ethynd/menu/*.raw           /Ethynd/audio/*.mp3 *.wav
/Ethynd/lang/fr.bin fr.json en.bin en.json
```

## 3. Démarrage — ce qu'il faut voir
- **Journal série** : `init ok : langue=fr, heap libre=…, PSRAM libre=…`, puis toutes les 5 s
  `fps=… tps=… heap=… min=… psram=…`.
- **À l'écran** : le menu (titre Ethynd, boutons Jouer / Aide avec leurs légendes).
- Écran noir avec « Ethynd: assets not found on SD » : la 2ᵉ ligne donne le fichier manquant (`Missing: …`).

## 4. Parcours (≈ 10 minutes)
| # | Action | Attendu |
|---|---|---|
| 1 | Menu : **C** puis **B** | l'aide s'affiche (accents lisibles), B revient |
| 2 | **A** | écran « Chargement… », puis la maison ; barre de vie + objectif « Parler au sage… » |
| 3 | Marcher dans toutes les directions | mouvement fluide, collisions avec les meubles, marche animée, bruit de pas |
| 4 | Aller au sage (à gauche), **A** | bulle « A » au-dessus de lui, boîte de dialogue, texte qui s'écrit, **A** pour continuer — **sans sauter de page** |
| 5 | **B** | journal des objectifs (1er coché), **B** ferme |
| 6 | Escalier de la cave (fond à droite) | la grotte s'ouvre ; objectif « Entrer dans la grotte » accompli |
| 7 | Combattre une chauve-souris avec **A** | épée, son de coup, la chauve-souris disparaît, compteur « (1/4) » |
| 8 | Se faire toucher | vie qui baisse, son de blessure |
| 9 | Ramasser potion / cristal | message « Objet obtenu », le cristal apparaît dans le journal |
| 10 | Menu système (MENU) : volume, langue **English**, retour | le jeu se met en pause ; la langue change **sans redémarrer** (menu, dialogues) |
| 11 | Rapporter le cristal au sage | écran « Félicitations ! » ; **A** → menu |
| 12 | Mourir | écran « Game Over », **A** → menu |

## 5. Mesures (bouton **D** = affichage de diagnostic en bas à gauche)
Note, **dans la maison, dans la grotte et dans la grande carte du village** (la plus lourde) :
- **FPS** (images par seconde) : ≥ 30 fluide · 20–30 acceptable · **< 20 : le rendu doit être optimisé** (envoie les chiffres) ;
- **TPS** : doit rester à ≈ 30 (c'est la vitesse du jeu) ;
- mémoire (`H…k P…k` = tas interne et PSRAM libres) : doit rester **stable** — noter la valeur au début et après 5 minutes de jeu
  (une baisse continue = fuite).

## 6. Si ça ne va pas
| Symptôme | Cause probable | Quoi envoyer |
|---|---|---|
| Redémarrages en boucle | plantage au démarrage (pile, mémoire) | les 30 premières lignes du journal série |
| « Missing: … » | fichier absent ou mal placé sur la SD | rien : vérifier l'arborescence du §2 |
| Textes remplacés par `ui.menu.play`… | `lang/fr.bin` absent ou illisible | contenu de `/Ethynd/lang/` |
| `?` dans un texte | caractère absent de la police | la langue et la phrase |
| Couleurs fausses (rouge ↔ bleu) | ordre BGR | une photo ; essayer `--order rgb` au convertisseur |
| Pas de musique / grésillements | MP3 : fichier, priorité ou pile de la tâche | journal série, `/Ethynd/audio/` ; réglages `GB_MP3_TASK_PRIO` / `GB_MP3_TASK_STACK` |
| Musique qui « claque » à la reprise | boucle MP3 sans trou non respectée | quelle musique |
| Effets sonores trop rapides/lents | fréquence du WAV (doit être 44,1 kHz mono) | nom du son |
| Dialogue qui saute des pages | lecture des boutons (corrigée : lue à chaque tick) | si ça arrive encore : FPS mesuré |
| Jeu lent | rendu trop lourd | les FPS des trois cartes (§5) |
| Mauvais boutons | correspondance des touches | quel bouton fait quoi |
| La langue ne change pas | pas de `lang/<code>.bin` pour ce code | code affiché par le menu système |

## 7. Après le test
Renvoie `RAPPORT_TEST_AKA.md` complété. Les mesures décident de la suite : optimisation du rendu si FPS < 20, sinon
sauvegarde de partie (voir `docs/SUIVI.md`).
