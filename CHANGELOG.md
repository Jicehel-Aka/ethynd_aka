# Journal des modifications

## 2026-10-12 — workflow de release
- `release.yml` : sur étiquette `v*` (ou lancement manuel de répétition) construit le firmware de la console, la carte SD, les versions PC
  **autonomes** Windows et Linux (jeu + éditeur + DLL + assets + docs + lanceurs) et crée une release GitHub **en brouillon** (pré-release si
  l'étiquette contient un tiret) avec les notes du `CHANGELOG.md`.
- `tools/package_pc.py` (archive PC) et `tools/release_notes.py` ; `build-pc.yml` livre maintenant des archives complètes avec leurs DLL
  (avant : l'exécutable seul, inutilisable sous Windows). `docs/RELEASE.md` : procédure et liste de contrôle.

## 2026-10-12 — panneau de boutons de la version PC
- Sous la zone de jeu : nom du jeu, croix + boutons **A B C D** dessinés (ils s'allument avec le clavier), rappel des **touches PC** et de leur rôle,
  ligne M / Échap ; **cliquable** à la souris et **multi-doigts** au tactile ; traduit (clés `pc.*`) ; `--no-panel` pour le masquer ; zoom 2 par défaut.
- Testé sans fenêtre : zones de chaque bouton à 3 zooms, suivi de plusieurs pointeurs (glisser, deux doigts), captures du panneau.

## 2026-10-12 — le dragon
- **Boss** : le dragon rouge (défini par Ethynd mais jamais placé) garde le Cristal dans le coin sud-ouest de la grotte : hitbox à la taille de
  son sprite (56 px), poursuite du joueur dans sa zone sans jamais la quitter, 60 PV, cri d'Ethynd, ne réapparaît plus une fois vaincu.
- **Histoire** : nouvel objectif « Vaincre le dragon rouge » ; le Cristal n'apparaît qu'après (`item … <objectif>`) ; textes FR/EN mis à jour.
- **Combat** : dégâts = attaque / 2 ; contact prolongé = coup toutes les 1,5 s ; immunité d'épée seulement pendant l'attaque et jamais contre un boss.
- **Données/éditeur** : `spawn … [rayon]`, `item … [objectif]` (cartes en format `EMA3`) ; l'éditeur les lit, les affiche et les valide.
- **Tests** : zone, poursuite, Cristal caché, combat, contact prolongé, persistance de la défaite, atteignabilité de la salle du boss.

## 2026-10-11 — préparation du test console
- **Licence ouverte MIT** (`LICENSE`, `NOTICE.md`) ; musiques confirmées.
- **Bug corrigé** : l'entrée était lue une fois par tour de boucle au lieu d'une fois par tick (`shared/platform/FixedStep.h`) : un
  appui pouvait être perdu, ou vu deux fois (dialogue qui saute une page) dès que le rendu n'est pas pile à 30 FPS. Testé avec un
  témoin qui reproduit l'ancienne boucle (9 / 59 fronts au lieu de 40).
- **Diagnostic** : bouton **D** (F3 / Tab sur PC) = FPS, ticks/s, mémoire libre ; journal série toutes les 5 s sur la console.
- **Doc** : `docs/TEST_AKA.md` (procédure, parcours, mesures, dépannage) et `docs/RAPPORT_TEST_AKA.md` (modèle de rapport).

## 2026-10-09 — audit, outils, éditeur
- **Jeu** : le bord de la carte est infranchissable (l'original laissait sortir dans le vide) ; les monstres ne
  traversent plus les PNJ ; message d'erreur précis à l'écran si un fichier manque sur la SD (« Missing: … »).
- **Polices** : ajout de ä ö Ä Ö ì ò Ì Ò ã õ Ã Õ ° (l'allemand annoncé dans l'ancienne doc n'était pas couvert).
- **Outils** : `tools/dev.py` (point d'entrée unique, sans bash), `translations_csv.py` (tableur + nouvelle langue),
  `render_maps.py` (aperçus PNG des cartes), `check_world.py` étendu (clés du code, polices, longueurs),
  `npc_defs.py` (source unique des variantes de PNJ), `host_test/CMakeLists.txt` (CTest).
- **Éditeur** : `L` choisit la destination d'une porte en cliquant sur la carte d'arrivée ; `Ctrl+R` redimensionne.
- **Traductions** : une valeur vide = « pas encore traduit » (repli sur le français, compté comme manquant).
- **Tests** : bord de carte, diagnostic d'assets manquants, budget de rendu par carte, destination de porte,
  redimensionnement, valeurs vides.
- **Windows** : correction du build (`SDL.h` manquant dans `SdlAudio.h`), `NOMINMAX`, `/utf-8`.
- **Doc** : `docs/PC.md` (version PC seule, sans éditeur ; ce qu'elle simule et ce qu'elle ne simule pas).
- **Repo** : `.gitattributes` (scripts en LF), `NOTICE.md`, `docs/SUIVI.md`, `docs/OUTILS.md`.

## 2026-10-08 — dialogues, objectifs, multilingue, éditeur
- Données du jeu sorties du code (`world/`), PNJ + dialogues, objectifs, objets, écran de victoire.
- Tous les textes en clés de traduction (`world/lang/<code>.txt`) ; images de menu sans texte.
- Éditeur de cartes PC (cœur testé sans écran + fenêtre SDL), workflows GitHub (tests, paquet SD, build PC + éditeur).

## Avant
- Portage du cœur d'Ethynd (cartes, collisions pixel-perfect, monstres, épée, téléportations), affichage 24 px,
  piste audio MP3 (minimp3), son de marche raccourci, build PC (SDL), build console, CI.
