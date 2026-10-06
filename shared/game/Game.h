// Game.h -- ecrans et boucle de jeu (port de fonctions/jeu.py + dialogues, objectifs, objets).
//
// Ecrans : Menu (A jouer, C aide), Help (B retour), Playing, Dialogue (A suivant), Log (B / A fermer),
// Dead et Win (A : menu). Un appel a update() = 1 tick logique (30 par seconde).
// Les cartes, monstres, PNJ, portes, objets, dialogues et objectifs viennent des fichiers de
// donnees (world.bin, maps/*.map) ; TOUS les textes viennent de lang/<code>.bin (multilangue).
//
// ECARTS VOLONTAIRES avec Ethynd :
//  - seuls les monstres dont "attaque" > 0 blessent le joueur (l'original blessait aussi avec
//    le chat, les oiseaux et les poussins : le champ "attaque" n'y est jamais lu) ;
//  - un monstre mort n'a plus de hitbox (l'original laissait une hitbox fantome) ;
//  - apres l'ecran de mort, retour au menu au lieu de quitter le programme ;
//  - nouveautes : PNJ + dialogues, objectifs, objets (potions, cristal), ecran de victoire.
#pragma once
#include <string>
#include <vector>
#include "AssetStore.h"
#include "GameMap.h"
#include "Characters.h"
#include "World.h"
#include "Lang.h"

struct QuestState {
    uint64_t flags = 0;
    uint8_t  items[kMaxItems] = {0};
    uint16_t kills[kMaxKillTypes] = {0};
    uint32_t objDone = 0;
    bool flag( int i ) const { return i >= 0 && i < 64 && ( flags >> i ) & 1; }
    void setFlag( int i ) { if ( i >= 0 && i < 64 ) flags |= 1ull << i; }
    bool done( int i ) const { return ( objDone >> i ) & 1; }
};

class Game {
  public:
    Game( IRenderer& r, IAudio& a, IAssetSource& s ) : renderer( r ), audio( a ), source( s ) {}
    bool init();                          // charge assets, monde et langue ; false si un fichier manque
    void update( const IInput& input );   // 1 tick
    void render();                        // dessine l'ecran courant (present() a la charge de l'appelant)

    // Langue : code de lang/<code>.bin ("fr", "en", ...). Peut etre appele avant ou apres init().
    void setLanguage( const char* code );
    // Phrase propre a la plateforme affichee dans l'aide, par CLE de traduction (ex. "ui.quit.aka")
    void setQuitHintKey( const char* key ) { quitKey = key ? key : ""; }

    enum class Screen { Menu, Help, Playing, Dialogue, Log, Dead, Win };
    Screen screen() const { return current; }
    const GameMap& map() const { return gmap; }
    int mapIndex() const { return mapIdx; }
    const Player& playerRef() const { return player; }
    const std::vector<Monster>& monstersRef() const { return monsters; }
    const std::vector<Npc>& npcsRef() const { return npcs; }
    const std::vector<MapItem>& itemsRef() const { return items; }
    const QuestState& quest() const { return qs; }
    const WorldData& worldData() const { return world; }
    int currentObjective() const;         // premier objectif visible non termine, -1 si aucun
    std::string objectiveLabel( int i ) const;                 // titre + progression "(2/4)"
    const char* tr( const std::string& key ) const { return lang.get( key ); }
#ifdef ETHYND_HOST_TEST
    void debugSetCamera( int x, int y ) { gmap.camX = x; gmap.camY = y; }   // tests PC uniquement
    void debugSetFlag( int i ) { qs.setFlag( i ); }
    void debugTeleport( int map, int px, int py ) { enterMap( map, kLogicCx - px, kPlayerHitCy - py ); }
#endif

  private:
    IRenderer& renderer;
    IAudio& audio;
    IAssetSource& source;
    AssetStore assets;
    WorldData world;
    Lang lang;
    std::string langCode = Lang::kDefault, quitKey;
    bool ready = false;

    ResolvedChar playerChar, entityChars[8], npcChars[4];
    std::vector<ImageId> itemIcons;
    GameMap gmap;
    int mapIdx = -1;
    Player player;
    std::vector<Monster> monsters;
    std::vector<Npc> npcs;
    std::vector<MapItem> items;
    std::vector<Rect> blockers;                          // rectangles monde des PNJ
    std::vector<std::vector<uint8_t>> taken;             // objets deja ramasses, par carte
    QuestState qs;
    Screen current = Screen::Menu;

    // dialogue en cours
    int dlg = -1, dlgLine = 0, dlgPage = 0, dlgShown = 0;
    std::vector<std::string> dlgRows;
    bool winPending = false;
    bool doorsArmed = false;                             // les portes ne s'activent qu'apres avoir quitte la zone d'arrivee
    // messages ephemeres
    std::vector<std::string> toastQueue;
    std::string toast;
    int toastTicks = 0;

    bool enterMap( int idx, int camX, int camY );        // chargement + carte + objets + musique
    void startGame();
    void updatePlaying( const IInput& input );
    void updateDialogue( const IInput& input );
    void startDialogue( int index );
    void startDialogueLine();
    int  dialoguePageChars() const;
    void finishDialogue();
    int  chooseDialogue( const Npc& n ) const;
    const Npc* npcInReach() const;
    Rect playerWorld() const { return Rect{ Player::hitbox().x - gmap.camX, Player::hitbox().y - gmap.camY, kPlayerHitW, kPlayerHitH }; }
    void checkDoors();
    void checkItems();
    void updateObjectives();
    bool objectiveMet( const Objective& o ) const;
    void pushToast( const std::string& s ) { toastQueue.push_back( s ); }

    // rendu
    void drawHud();
    void drawDialogue();
    void drawLog();
    void drawMenuScreens();
    int  textWidth( const char* s, int scale ) const;
    void textAt( int x, int y, const char* s, RGBColor c, int scale = 1 );
    void textCentered( int cx, int y, const char* s, RGBColor c, int scale, int maxW );
    int  textWrapped( int x, int y, int cols, const char* s, RGBColor c );   // renvoie le nombre de lignes
};
