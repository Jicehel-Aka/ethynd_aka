// World.h -- donnees du monde lues depuis les fichiers convertis (world.bin, maps/*.map) :
// histoire (drapeaux, objets, dialogues, objectifs) et objets de carte (monstres, PNJ, portes).
// Ecrit par tools/convert_assets.py a partir du format texte de world/ ; edite par l'editeur.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "Defs.h"
#include "Collision.h"
#include "IAssetSource.h"

constexpr int kMaxFlags = 64, kMaxItems = 16, kMaxObjectives = 32, kMaxKillTypes = 8;

// Aucun texte en dur : tout est une CLE de traduction (lang/<code>.bin, voir Lang.h).
struct DlgLine { std::string speaker, text; };       // speaker vide : narration
enum ActionType : uint8_t { ACT_SET = 1, ACT_GIVE = 2, ACT_TAKE = 3, ACT_WIN = 4, ACT_HEAL = 5 };
struct DlgAction { uint8_t type; int16_t a, b; };    // set a ; give/take objet a, quantite b ; heal a
struct Dialogue { std::vector<DlgLine> lines; std::vector<DlgAction> actions; };

struct ItemDef { std::string icon; uint8_t heal; std::string name; };

enum CondType : uint8_t { COND_FLAG = 0, COND_KILL = 1, COND_ITEM = 2, COND_MAP = 3, COND_REACH = 4 };
struct Objective {
    std::string title;
    int8_t after = -1;                               // objectif precedent (-1 : visible d'emblee)
    uint8_t cond = 0;
    std::string s;                                   // kill : type de monstre
    int16_t p[4] = {0, 0, 0, 0};                     // flag: p0 | kill: p0=n | item: p0=objet p1=n | map: p0 | reach: map x y rayon
};

// Condition d'un dialogue de PNJ : la premiere regle vraie gagne.
enum RuleKind : uint8_t { RULE_DEFAULT = 0, RULE_FLAG = 1, RULE_NOT_FLAG = 2, RULE_HAS_ITEM = 3 };
struct TalkRule { uint8_t kind; int16_t arg; int16_t dialogue; };

struct SpawnObj { std::string type; int16_t x, y, vie, attaque; };
struct NpcObj   { std::string type; int16_t x, y; Dir dir; std::vector<TalkRule> rules; };
struct DoorObj  { Rect r; uint8_t destMap; int16_t dx, dy; int16_t needFlag, deny; };  // dx, dy : centre du joueur a l'arrivee
struct ItemObj  { uint8_t def; int16_t x, y; };

struct MapObjects {
    uint8_t music = 0;
    std::vector<SpawnObj> spawns;
    std::vector<NpcObj>   npcs;
    std::vector<DoorObj>  doors;
    std::vector<ItemObj>  items;
};

struct WorldData {
    uint8_t startMap = 0; int16_t startX = 0, startY = 0; Dir startDir = DIR_BAS;
    std::vector<std::string> mapNames;
    int flagCount = 0;
    std::vector<ItemDef>   items;
    std::vector<Dialogue>  dialogues;
    std::vector<Objective> objectives;

    bool load( IAssetSource& src );                  // world.bin
    int  mapIndex( const char* name ) const;         // -1 si inconnue
};
