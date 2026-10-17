# Suivi du projet — Ethynd sur Gamebuino AKA

*Mis à jour le 2026-10-09. Ce fichier est le tableau de bord : état réel, ce qui est vérifié ou non, problèmes connus,
travail restant (priorisé), décisions prises. Les nouveautés par date sont dans `CHANGELOG.md`.*

## 1. État par composant

Légende : ✅ testé automatiquement sur PC · 🟡 syntaxe vérifiée seulement (non compilé en vrai / non exécuté) · ⬜ non vérifié.

| Composant | État | Preuve / remarque |
|---|---|---|
| Boucle à pas fixe (`FixedStep.h`) | ✅ | simulation : 40 appuis → 40 fronts à 5, 33 et 70 ms par tour ; l'ancienne boucle en donnait 9 et 59 |
| Cœur du jeu (`shared/game`) | ✅ | histoire complète rejouée sans écran, sous AddressSanitizer + UBSan, `-Wall -Wextra -Wshadow -Wpedantic` en gnu++23 sans avertissement |
| Monde (`world/`) + validation | ✅ | `check_world.py` : références, traductions, clés du code, polices, longueurs |
| Atteignabilité du monde | ✅ | chaque objet, porte, PNJ atteignable à pied (parcours sur le réseau exact des pas) |
| Multilingue (fr, en ; ajout par fichier) | ✅ | repli sur le français, langue partielle, valeur vide, langue inconnue testés |
| Convertisseur d'assets | ✅ | produit les `.bin`/`.map`/`.raw`/langues ; relu par le jeu et l'éditeur |
| Éditeur de cartes — cœur | ✅ | peinture, annuler/rétablir, objets, propriétés + refus, destination de porte, redimensionnement, nouvelle carte, sauvegarde **identique octet pour octet** à Python, langues |
| Éditeur de cartes — fenêtre SDL | 🟡 | `main_sdl.cpp` : syntaxe vérifiée avec un stub SDL ; jamais ouverte |
| Panneau de boutons PC | ✅ / 🟡 | dessin, zones et suivi des pointeurs testés sans fenêtre (captures) ; **intégration à la fenêtre SDL (clics/doigts réels) jamais exécutée** |
| Build PC (SDL) | 🟡 | syntaxe ; **le build Windows a révélé un bug réel** (corrigé : `SDL.h` manquant). Linux/Windows : à confirmer par la CI |
| Build console (ESP-IDF) | 🟡 | syntaxe avec les en-têtes `gamebuino` réels et des stubs FreeRTOS ; **jamais compilé avec ESP-IDF** |
| Piste audio MP3 (`gb_audio_track_mp3`) | 🟡 | syntaxe ; jamais exécutée (ni avec un vrai décodeur, ni sur matériel). Formules de rognage gapless vérifiées contre ffmpeg |
| Licence et crédits | ✅ | `LICENSE` (MIT), `NOTICE.md` : musiques confirmées |
| Tests CTest (`host_test/CMakeLists.txt`) | ⬜ | `cmake` absent de l'environnement de rédaction ; les scripts bash, eux, tournent |
| Workflows GitHub (tests, builds, paquet SD) | 🟡 | YAML valide ; le build PC a tourné (et trouvé un bug), les autres jamais lancés |
| Workflow de **release** (`release.yml`) + `package_pc.py` | 🟡 | YAML valide, empaquetage PC testé avec de faux dossiers de build ; **jamais exécuté sur GitHub** : faire une répétition à blanc (`docs/RELEASE.md`) |
| Sur la console : fluidité, MP3, accents, 4 canaux audio | ⬜ | rien n'a tourné sur le matériel |

## 2. À faire d'abord (bloquant pour une première version jouable)
1. **Compiler pour de vrai** (`tools/fetch_minimp3.sh` puis `idf.py build`, ou le workflow) et corriger ce qui sort.
2. **Tester sur la console** : copier la SD (`dev.py assets` ou workflow *Package SD card assets*), puis vérifier dans cet ordre :
   démarrage (un message « Missing: … » désigne le fichier absent), fluidité, musiques (boucles sans clic), accents,
   volumes, niveau de chaque son.
3. ~~Licence du dépôt~~ : **fait** — licence ouverte **MIT** (`LICENSE`, `NOTICE.md`) ; à adapter (nom du titulaire, ou autre licence ouverte) si besoin.
4. ~~Accord pour les musiques~~ : **confirmé** par le porteur du portage. Reste à **prévenir le chef du projet d'origine** une fois le
   portage montrable (prévu).
5. **Répétition à blanc de la release** (*Actions → Release → Run workflow*, étiquette de test) puis vraie release (`docs/RELEASE.md`).
6. **Faire le premier test console** en suivant `docs/TEST_AKA.md` et remplir `docs/RAPPORT_TEST_AKA.md` (FPS aux trois cartes, mémoire, son).

## 3. Problèmes connus et limites
| # | Sujet | Détail |
|---|---|---|
| L1 | Budget de rendu | pire cas mesuré : **497 appels `drawImage`, ≈ 370 000 pixels par image** (≈ 4,8× l'écran) sur l'aventure ; 291 sur la grotte, 213 sur la maison. Impossible à juger sans la console : à mesurer (voir B2) |
| L2 | Pas de sauvegarde | la progression (drapeaux, objets, objectifs, vie, position) est perdue à l'extinction |
| L3 | Musique par carte | liste fixe de 5 (`none menu maison aventure grotte`) ; une nouvelle carte ne peut pas avoir sa propre musique sans toucher au code (`AkaAudio`, `SdlAudio`, `IAudio`) |
| L4 | Polices | latin avec accents fr/es/de/it/pt ; pas de cyrillique, grec, asiatique, polonais… (`check` signale les caractères non dessinés) |
| L5 | Types de monstres/PNJ | monstres : 5 (constantes d'Ethynd) ; PNJ : recoloriages du joueur + animaux. Ajouter un monstre demande du code (animations) |
| L6 | Histoire | éditée dans des fichiers texte, pas dans l'éditeur de cartes (dialogues, objectifs, drapeaux) |
| L7 | Boss | le dragon est en place (voir README) mais **non équilibré en conditions réelles** : 60 points de vie, 2 dégâts / 1,5 s, vitesse 2 px/tick (le joueur : 3). Le test automatique le bat avec la caméra calée sur l'épée (cas idéal) : à ajuster après le test console (`vie`, `attaque`, `rayon` dans `world/maps/grotte.txt`) |
| L8 | Collisions des monstres | testées en coordonnées monde : au plus 3 px d'écart avec l'original (documenté) |
| L9 | Fiche `aka_font` de Dark & Under | annonçait l'allemand (ä ö) à tort ; corrigé ici en ajoutant les glyphes — à reporter dans Dark & Under |

## 4. Backlog (priorisé)
**P1 — avant de publier**
- B1. Build et test réels sur console (voir §2). *Effort : dépend des erreurs.*
- B2. Mesurer la fluidité ; **si insuffisante** : aplatir les couches 0–2 du fond en une seule image par case visible
  (recalculée seulement quand une tuile animée change), ce qui réduit fortement le surdessin (la grande carte a jusqu'à
  3 couches empilées). *Effort : 1 jour.*
- B3. **Sauvegarde sur SD** (`save.bin` : drapeaux, objets, compteurs, objectifs, vie, carte, position ; option « Continuer » au
  menu). *Effort : ½ jour ; le cœur expose déjà `QuestState`.*

**P2 — confort et robustesse**
- B4. Musique par carte pilotée par les données (nom de fichier dans `maps/*.txt`, plus d'énumération fixe).
- B5. Journaliser/afficher les sons introuvables côté AKA (aujourd'hui : silence).
- B6. Éditeur : copier/coller de zones, édition des animations de tuiles, suppression/renommage de carte, avertissement
  « zone inatteignable » (réutiliser le parcours du test), aperçu de la hitbox du joueur.
- B7. Job CI Windows qui lance les tests via CTest (une fois confirmé en local).
- B8. Option de vitesse du texte et de taille de boîte (accessibilité).

**P3 — contenu**
- B9. Plus de quêtes/PNJ (marchand, objets clés), le dragon comme boss final, nouvelles cartes.
- B10. Autres langues (traducteurs : `dev.py lang export/import`).

## 5. Décisions prises
| Décision | Raison |
|---|---|
| Licence ouverte **MIT** pour le code du portage ; musiques OK | demande du porteur (« licence ouverte, musique ok ») ; MIT : permissive, compatible avec Unlicense, CC0, zlib et les composants de Dark & Under |
| Pas de sauvegarde de partie ni d'optimisation de rendu avant le **premier test console** | éviter d'empiler des fonctions non testées sur le matériel ; priorités fixées par les mesures (D = FPS/mémoire) |
| Affichage 24 px, **logique en unités de 32 px** | champ de vision 13,3×10 cases sur 320×240 ; comportement identique à Ethynd (vitesses, hitbox, collisions) sans calculs fractionnaires |
| Tous les textes en **clés de traduction**, un fichier par langue | exigence « tout doit être multilingue » ; ajout d'une langue sans code |
| Données du jeu dans `world/` (texte), éditeur sur mesure | demande d'externaliser les données ; format identique pour Python et C++ (sauvegarde octet pour octet) |
| MP3 pour les musiques (minimp3), WAV pour les effets | taille sur la SD (et narration de Wesnoth) ; effets courts sans latence |
| Couleurs **BGR565 natif AKA** pour tous les assets | mêmes fichiers sur console, PC et éditeur |
| Seuls les monstres avec `attaque > 0` blessent ; bord de carte = mur | corrige deux défauts de l'original (voir README) |
| Texte des menus redessiné à l'exécution (plus de texte dans les images) | traduction |

## 6. Comment mettre ce suivi à jour
Après chaque session : mettre à jour le tableau §1 (état + preuve), déplacer les points résolus du §3/§4 vers
`CHANGELOG.md`, noter toute décision au §5.
