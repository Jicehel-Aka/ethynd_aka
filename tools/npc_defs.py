"""npc_defs.py -- variantes de PNJ : recoloriage du personnage du joueur (14 couleurs dans l'original).

Source UNIQUE utilisee par gen_data.py (tables d'animation C++) et convert_assets.py (sprites).
Pour ajouter un PNJ : ajouter une entree ici, relancer tools/gen_data.py puis tools/convert_assets.py ;
le type devient disponible dans l'editeur (touche N, puis , . pour changer de type).
"""
NPC_VARIANTS = {
    "sage": {(67, 46, 39): (235, 235, 240), (106, 72, 52): (190, 190, 200),
             (196, 60, 60): (70, 160, 110), (136, 46, 46): (40, 110, 75), (104, 28, 28): (25, 75, 50),
             (101, 101, 155): (140, 110, 70)},
    "gardien": {(196, 60, 60): (70, 100, 180), (136, 46, 46): (45, 70, 130), (104, 28, 28): (30, 45, 95),
                (101, 101, 155): (90, 90, 90)},
}
