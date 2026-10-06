// WorldText.h -- lecture/ecriture du format texte du monde (miroir de tools/world_io.py).
// Les fichiers ecrits ici sont identiques octet pour octet a ceux ecrits par Python.
#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

struct EdSpawn { std::string type; int x = 0, y = 0, vie = 10, attaque = 1; };
struct EdNpc   { std::string id, type, dir = "bas", rules = "-"; int x = 0, y = 0; };
struct EdDoor  { int x = 0, y = 0, w = 32, h = 32; std::string map, requires_ = "-", deny = "-"; int dx = 0, dy = 0; };
struct EdItem  { std::string id; int x = 0, y = 0; };

struct EdMap {
    std::string name, music = "none";
    int w = 0, h = 0;
    std::vector<int> layer[4];                  // w*h ids du tileset d'origine, -1 = vide
    std::vector<EdSpawn> spawns;
    std::vector<EdNpc> npcs;
    std::vector<EdDoor> doors;
    std::vector<EdItem> items;
    void resetLayers( int nw, int nh ) { w = nw; h = nh; for ( auto& l : layer ) l.assign( (size_t)nw * nh, -1 ); }
    int& at( int l, int x, int y ) { return layer[l][(size_t)y * w + x]; }
};

struct EdStory {                                 // ids connus de story.txt (pour verifier les references)
    std::vector<std::string> flags, dialogues, objectives;
    std::vector<std::string> itemIds, itemIcons;
};

struct EdWorld {
    std::string dir;
    std::string startMap = "maison"; int startX = 0, startY = 0; std::string startDir = "bas";
    std::vector<std::string> mapNames;
    std::set<int> collide;
    std::map<int, int> anim;
    EdStory story;
    std::vector<EdMap> maps;
    int mapIndex( const std::string& n ) const { for ( size_t i = 0; i < maps.size(); i++ ) if ( maps[i].name == n ) return (int)i; return -1; }
};

bool loadWorld( const std::string& dir, EdWorld& w, std::string& err );
bool saveMapFile( const std::string& dir, const EdMap& m, std::string& err );
bool saveWorldFile( const std::string& dir, const EdWorld& w, std::string& err );     // world.txt
bool saveTilesFile( const std::string& dir, const EdWorld& w, std::string& err );     // tiles.txt
bool loadLangFile( const std::string& path, std::map<std::string, std::string>& out );
