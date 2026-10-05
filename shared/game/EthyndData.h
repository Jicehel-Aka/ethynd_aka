// GENERE par tools/gen_data.py a partir des constantes Python d'Ethynd.
// NE PAS EDITER A LA MAIN.
#pragma once
#include "Defs.h"

static const char* const k_joueur_bas_base[] = { "personnage_00" };
static const char* const k_joueur_bas_marche[] = { "personnage_01", "personnage_02", "personnage_03", "personnage_00" };
static const char* const k_joueur_bas_attaque[] = { "personnage_44", "personnage_45", "personnage_46", "personnage_47" };
static const char* const k_joueur_haut_base[] = { "personnage_22" };
static const char* const k_joueur_haut_marche[] = { "personnage_23", "personnage_24", "personnage_25", "personnage_22" };
static const char* const k_joueur_haut_attaque[] = { "personnage_48", "personnage_49", "personnage_50", "personnage_51" };
static const char* const k_joueur_gauche_base[] = { "personnage_33" };
static const char* const k_joueur_gauche_marche[] = { "personnage_34", "personnage_35", "personnage_36", "personnage_33" };
static const char* const k_joueur_gauche_attaque[] = { "personnage_56", "personnage_57", "personnage_58", "personnage_59" };
static const char* const k_joueur_droite_base[] = { "personnage_11" };
static const char* const k_joueur_droite_marche[] = { "personnage_12", "personnage_13", "personnage_14", "personnage_11" };
static const char* const k_joueur_droite_attaque[] = { "personnage_52", "personnage_53", "personnage_54", "personnage_55" };
static const char* const k_chauve_souris_bas_base[] = { "chauve_souris_00" };
static const char* const k_chauve_souris_bas_marche[] = { "chauve_souris_01", "chauve_souris_02", "chauve_souris_03" };
static const char* const k_chauve_souris_haut_base[] = { "chauve_souris_08" };
static const char* const k_chauve_souris_haut_marche[] = { "chauve_souris_09", "chauve_souris_09", "chauve_souris_11" };
static const char* const k_chauve_souris_gauche_base[] = { "chauve_souris_12" };
static const char* const k_chauve_souris_gauche_marche[] = { "chauve_souris_13", "chauve_souris_14", "chauve_souris_15" };
static const char* const k_chauve_souris_droite_base[] = { "chauve_souris_04" };
static const char* const k_chauve_souris_droite_marche[] = { "chauve_souris_05", "chauve_souris_06", "chauve_souris_07" };
static const char* const k_chat_bas_base[] = { "chat_00" };
static const char* const k_chat_bas_marche[] = { "chat_01", "chat_00", "chat_02" };
static const char* const k_chat_haut_base[] = { "chat_09" };
static const char* const k_chat_haut_marche[] = { "chat_10", "chat_09", "chat_11" };
static const char* const k_chat_gauche_base[] = { "chat_03" };
static const char* const k_chat_gauche_marche[] = { "chat_04", "chat_03", "chat_05" };
static const char* const k_chat_droite_base[] = { "chat_06" };
static const char* const k_chat_droite_marche[] = { "chat_07", "chat_06", "chat_08" };
static const char* const k_oiseau_bas_base[] = { "oiseau_00" };
static const char* const k_oiseau_bas_marche[] = { "oiseau_01", "oiseau_00", "oiseau_02" };
static const char* const k_oiseau_haut_base[] = { "oiseau_09" };
static const char* const k_oiseau_haut_marche[] = { "oiseau_10", "oiseau_09", "oiseau_11" };
static const char* const k_oiseau_gauche_base[] = { "oiseau_03" };
static const char* const k_oiseau_gauche_marche[] = { "oiseau_04", "oiseau_03", "oiseau_05" };
static const char* const k_oiseau_droite_base[] = { "oiseau_06" };
static const char* const k_oiseau_droite_marche[] = { "oiseau_07", "oiseau_06", "oiseau_08" };
static const char* const k_poussin_bas_base[] = { "poussin_09" };
static const char* const k_poussin_bas_marche[] = { "poussin_10", "poussin_09", "poussin_11" };
static const char* const k_poussin_haut_base[] = { "poussin_00" };
static const char* const k_poussin_haut_marche[] = { "poussin_01", "poussin_00", "poussin_02" };
static const char* const k_poussin_gauche_base[] = { "poussin_03" };
static const char* const k_poussin_gauche_marche[] = { "poussin_04", "poussin_03", "poussin_05" };
static const char* const k_poussin_droite_base[] = { "poussin_06" };
static const char* const k_poussin_droite_marche[] = { "poussin_07", "poussin_06", "poussin_08" };
static const char* const k_dragon_rouge_bas_base[] = { "dragon_rouge_00" };
static const char* const k_dragon_rouge_bas_marche[] = { "dragon_rouge_00", "dragon_rouge_01", "dragon_rouge_02", "dragon_rouge_03" };
static const char* const k_dragon_rouge_haut_marche[] = { "dragon_rouge_12", "dragon_rouge_13", "dragon_rouge_14", "dragon_rouge_15" };
static const char* const k_dragon_rouge_gauche_marche[] = { "dragon_rouge_04", "dragon_rouge_05", "dragon_rouge_06", "dragon_rouge_07" };
static const char* const k_dragon_rouge_droite_marche[] = { "dragon_rouge_08", "dragon_rouge_09", "dragon_rouge_10", "dragon_rouge_11" };

static const CharDef kDef_joueur = {
    "joueur",
    { // timings : tick, derniere frame, libre, retour a base
        { -1, 0, true, false },  // base
        { 4, 3, true, false },  // marche
        { 2, 3, true, true },  // attaque
    },
    { // animations [direction][mouvement]
        { { 1, k_joueur_bas_base }, { 4, k_joueur_bas_marche }, { 4, k_joueur_bas_attaque } },  // bas
        { { 1, k_joueur_haut_base }, { 4, k_joueur_haut_marche }, { 4, k_joueur_haut_attaque } },  // haut
        { { 1, k_joueur_gauche_base }, { 4, k_joueur_gauche_marche }, { 4, k_joueur_gauche_attaque } },  // gauche
        { { 1, k_joueur_droite_base }, { 4, k_joueur_droite_marche }, { 4, k_joueur_droite_attaque } },  // droite
    },
};

static const CharDef kDef_chauve_souris = {
    "chauve_souris",
    { // timings : tick, derniere frame, libre, retour a base
        { -1, 0, true, false },  // base
        { 4, 2, true, false },  // marche
        { -1, 0, true, false },  // attaque
    },
    { // animations [direction][mouvement]
        { { 1, k_chauve_souris_bas_base }, { 3, k_chauve_souris_bas_marche }, { 0, nullptr } },  // bas
        { { 1, k_chauve_souris_haut_base }, { 3, k_chauve_souris_haut_marche }, { 0, nullptr } },  // haut
        { { 1, k_chauve_souris_gauche_base }, { 3, k_chauve_souris_gauche_marche }, { 0, nullptr } },  // gauche
        { { 1, k_chauve_souris_droite_base }, { 3, k_chauve_souris_droite_marche }, { 0, nullptr } },  // droite
    },
};

static const CharDef kDef_chat = {
    "chat",
    { // timings : tick, derniere frame, libre, retour a base
        { -1, 0, true, false },  // base
        { 10, 2, true, false },  // marche
        { -1, 0, true, false },  // attaque
    },
    { // animations [direction][mouvement]
        { { 1, k_chat_bas_base }, { 3, k_chat_bas_marche }, { 0, nullptr } },  // bas
        { { 1, k_chat_haut_base }, { 3, k_chat_haut_marche }, { 0, nullptr } },  // haut
        { { 1, k_chat_gauche_base }, { 3, k_chat_gauche_marche }, { 0, nullptr } },  // gauche
        { { 1, k_chat_droite_base }, { 3, k_chat_droite_marche }, { 0, nullptr } },  // droite
    },
};

static const CharDef kDef_oiseau = {
    "oiseau",
    { // timings : tick, derniere frame, libre, retour a base
        { -1, 0, true, false },  // base
        { 10, 2, true, false },  // marche
        { -1, 0, true, false },  // attaque
    },
    { // animations [direction][mouvement]
        { { 1, k_oiseau_bas_base }, { 3, k_oiseau_bas_marche }, { 0, nullptr } },  // bas
        { { 1, k_oiseau_haut_base }, { 3, k_oiseau_haut_marche }, { 0, nullptr } },  // haut
        { { 1, k_oiseau_gauche_base }, { 3, k_oiseau_gauche_marche }, { 0, nullptr } },  // gauche
        { { 1, k_oiseau_droite_base }, { 3, k_oiseau_droite_marche }, { 0, nullptr } },  // droite
    },
};

static const CharDef kDef_poussin = {
    "poussin",
    { // timings : tick, derniere frame, libre, retour a base
        { -1, 0, true, false },  // base
        { 8, 2, true, false },  // marche
        { -1, 0, true, false },  // attaque
    },
    { // animations [direction][mouvement]
        { { 1, k_poussin_bas_base }, { 3, k_poussin_bas_marche }, { 0, nullptr } },  // bas
        { { 1, k_poussin_haut_base }, { 3, k_poussin_haut_marche }, { 0, nullptr } },  // haut
        { { 1, k_poussin_gauche_base }, { 3, k_poussin_gauche_marche }, { 0, nullptr } },  // gauche
        { { 1, k_poussin_droite_base }, { 3, k_poussin_droite_marche }, { 0, nullptr } },  // droite
    },
};

static const CharDef kDef_dragon_rouge = {
    "dragon_rouge",
    { // timings : tick, derniere frame, libre, retour a base
        { -1, 0, true, false },  // base
        { 4, 3, true, false },  // marche
        { -1, 0, true, false },  // attaque
    },
    { // animations [direction][mouvement]
        { { 1, k_dragon_rouge_bas_base }, { 4, k_dragon_rouge_bas_marche }, { 0, nullptr } },  // bas
        { { 0, nullptr }, { 4, k_dragon_rouge_haut_marche }, { 0, nullptr } },  // haut
        { { 0, nullptr }, { 4, k_dragon_rouge_gauche_marche }, { 0, nullptr } },  // gauche
        { { 0, nullptr }, { 4, k_dragon_rouge_droite_marche }, { 0, nullptr } },  // droite
    },
};

static const CharDef* const kEntityDefs[] = { &kDef_chauve_souris, &kDef_chat, &kDef_oiseau, &kDef_poussin, &kDef_dragon_rouge };
static const char* const kEntityNames[] = { "chauve_souris", "chat", "oiseau", "poussin", "dragon_rouge" };
static const int kEntityDefCount = 5;

// Niveaux : monstres (deplacement aleatoire). Positions en pixels logiques (32 px/case)
struct SpawnDef { const char* type; int16_t x, y, w, h; int16_t vie, attaque; };
static const SpawnDef kSpawns_grotte[] = { { "chauve_souris", 269, 315, 32, 32, 10, 2 }, { "chauve_souris", 500, 400, 32, 32, 10, 2 }, { "chauve_souris", 770, 450, 32, 32, 10, 2 }, { "chauve_souris", 700, 600, 32, 32, 10, 2 } };
static const int kSpawnCount_grotte = 4;
static const SpawnDef kSpawns_maison[] = { { "chat", 100, 320, 32, 32, 10000, 0 } };
static const int kSpawnCount_maison = 1;
static const SpawnDef kSpawns_aventure[] = { { "oiseau", 1000, 806, 32, 32, 10000, 0 }, { "oiseau", 2189, 1753, 32, 32, 10000, 0 }, { "poussin", 550, 696, 32, 32, 10000, 0 }, { "poussin", 326, 1618, 32, 32, 10000, 0 }, { "poussin", 1967, 2137, 32, 32, 10000, 0 } };
static const int kSpawnCount_aventure = 5;

struct LevelSpawns { const char* map; const SpawnDef* spawns; int count; };
static const LevelSpawns kLevelSpawns[] = { { "grotte", kSpawns_grotte, kSpawnCount_grotte }, { "maison", kSpawns_maison, kSpawnCount_maison }, { "aventure", kSpawns_aventure, kSpawnCount_aventure } };
static const int kLevelSpawnsCount = 3;
