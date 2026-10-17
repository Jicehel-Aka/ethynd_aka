# Faire une release

Le workflow **Release** (`.github/workflows/release.yml`) fabrique tout à partir d'une étiquette (*tag*) `v*` et crée une release GitHub
en **brouillon** : rien n'est publié tant que tu n'as pas relu et cliqué « Publish ». Une étiquette avec un tiret (`v0.1.0-rc1`) est
marquée **pré-release**.

## Ce que contient une release
| Fichier | Contenu |
|---|---|
| `Ethynd_sdcard.zip` | à décompresser à la racine de la SD de la console (`/Ethynd/` : assets, MP3/WAV, **toutes les langues**) |
| `Ethynd_aka_firmware.zip` | `ethynd_aka.bin`, bootloader, table de partitions (installation comme Dark & Under) |
| `Ethynd_PC_windows.zip`, `Ethynd_PC_linux.zip` | version PC **autonome** : jeu, éditeur de cartes, DLL (Windows), assets, monde, documentation, lanceurs `jouer` / `editeur` |
| `Ethynd_editor_assets.zip` | palette de tuiles de l'éditeur (déjà dans les archives PC) |

Les notes sont tirées de `CHANGELOG.md` (tout l'historique ; `python3 tools/release_notes.py --sections 1` pour la seule dernière
section, à utiliser pour les releases suivantes en modifiant l'appel dans le workflow).

## Avant d'étiqueter (liste de contrôle)
- [ ] Les workflows **Tests**, **Build AKA** et **Build PC (SDL)** sont verts sur la branche principale.
- [ ] Le test sur la console est fait et `docs/RAPPORT_TEST_AKA.md` rempli (compilation, parcours, FPS des trois cartes, son).
- [ ] `docs/SUIVI.md` à jour (état, problèmes connus) ; `CHANGELOG.md` : la section de la version est écrite.
- [ ] `LICENSE` : le nom du titulaire est renseigné ; `NOTICE.md` à jour.
- [ ] Les musiques : accord confirmé ; le chef du projet d'origine est prévenu (prévu).
- [ ] Les traductions : `python3 tools/dev.py check --strict` (aucun avertissement) si tu veux une release sans texte retombant sur le français.

## 1. Répétition à blanc (recommandé la première fois)
*Actions → Release → Run workflow*, avec une étiquette de test (`v0.0.0-test`). Le workflow construit tout et crée un **brouillon** ;
vérifie les fichiers joints (taille, contenu), télécharge une archive PC et lance-la, puis **supprime le brouillon et l'étiquette**.
Le premier passage échouera probablement quelque part (aucun de ces workflows, sauf *Build PC*, n'a encore tourné) : c'est le but.

## 2. La vraie release
```
git tag v0.1.0-rc1
git push origin v0.1.0-rc1
```
Le workflow se lance, crée le brouillon ; relire les notes, vérifier les fichiers, puis **Publish release**.
Versions : `vMAJEUR.MINEUR.CORRECTIF` ; `-rc1`, `-rc2`… pour les candidates (pré-releases).

## Dépannage
| Message / symptôme | Piste |
|---|---|
| `ethynd_pc introuvable dans build` (job *PC*) | le build a échoué ou écrit ailleurs ; voir l'étape précédente |
| Le jeu Windows ne démarre pas (DLL manquante) | `package_pc.py` copie les DLL déposées à côté de l'exécutable par vcpkg ; vérifier qu'elles sont dans l'archive |
| Job *AKA firmware* rouge | première erreur du journal `idf.py build` ; voir `docs/TEST_AKA.md` §1 |
| `fail_on_unmatched_files` | un des jobs n'a rien produit : ouvrir ses journaux |
| Droits refusés à la création de la release | *Settings → Actions → General → Workflow permissions* : « Read and write » |

## Retour arrière
Supprimer le brouillon (ou la release) puis l'étiquette : `git push origin :refs/tags/v0.1.0-rc1` et `git tag -d v0.1.0-rc1`.
