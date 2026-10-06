// EditorCore.h -- editeur de cartes d'Ethynd (PC). Aucune dependance a SDL : le cœur recoit des
// evenements abstraits (souris, clavier, texte) et dessine dans un SoftRenderer ; la fenetre SDL
// (main_sdl.cpp) et le test sans ecran (test_editor.cpp) utilisent exactement le meme code.
//
// Il edite le monde au format texte (world/ : cartes, portes, monstres, PNJ, objets, depart,
// collisions des tuiles). Les textes de l'histoire (dialogues, objectifs) restent dans
// story.txt et lang/*.txt, que l'editeur lit pour verifier les references.
#pragma once
#include <map>
#include <string>
#include <vector>
#include "SoftRenderer.h"
#include "WorldText.h"

enum EdKey { ED_TAB = 9, ED_ENTER = 13, ED_ESC = 27, ED_BACKSPACE = 8, ED_DELETE = 127, ED_SPACE = 32,
             ED_UP = 1000, ED_DOWN, ED_LEFT, ED_RIGHT, ED_PGUP, ED_PGDN, ED_F1 = 1100 };   // ED_F1 + n - 1 = Fn

struct EdEvent {
    enum Type { MouseDown, MouseUp, MouseMove, Wheel, KeyDown, KeyUp, Text } type = MouseMove;
    int x = 0, y = 0;                  // souris, en pixels de la fenetre
    int button = 1;                    // 1 gauche, 2 milieu, 3 droit
    int dy = 0;                        // molette (positif = vers le haut)
    int key = 0;                       // lettre minuscule ASCII ou EdKey
    bool ctrl = false, shift = false;
    std::string text;                  // saisie (UTF-8)
};

enum class EdTool { Brush, Fill, Rect, Erase, Pick, Select, Spawn, Npc, Door, Item, Start };
enum class EdSel { None, Spawn, Npc, Door, Item, Start };

class EditorCore {
  public:
    // worldDir : dossier world/ ; assetsDir : sortie de convert_assets.py --editor-out (tileset_full.bin)
    // et sprites.bin ; langCode : langue de l'interface (lang/<code>.txt, repli fr).
    bool open( const std::string& worldDir, const std::string& assetsDir, const std::string& langCode, std::string& err );
    void setViewSize( int w, int h ) { vw = w; vh = h; }
    void setBuildCommand( const std::string& c ) { buildCmd = c; }
    void handle( const EdEvent& e );
    void draw( SoftRenderer& r );
    bool saveAll();

    // ---- etat expose (tests et fenetre)
    EdWorld world;
    int curMap = 0;
    EdTool tool = EdTool::Brush;
    int layer = 0, selTile = 0;
    bool dirty = false, showGrid = true, showColl = true, showHelp = false, quitAsked = false;
    bool layerVisible[4] = { true, true, true, true };
    EdSel selKind = EdSel::None; int selIdx = -1;
    std::string status;
    int scrollX = 0, scrollY = 0;                   // defilement de la vue, en pixels d'affichage

    EdMap& cur() { return world.maps[curMap]; }
    std::string tr( const std::string& key ) const;
    std::string tr( const std::string& key, const std::string& a, const std::string& b = "" ) const;
    int undoDepth() const { return curMap < (int)undoStack.size() ? (int)undoStack[curMap].size() : 0; }
    static constexpr int kTile = 24;                // pixels d'affichage par case
    static constexpr int kPanel = 256;              // largeur du panneau de droite
    static constexpr int kStatusH = 40;

  private:
    int vw = 960, vh = 600;
    std::string assetsDir, buildCmd;
    std::map<std::string, std::string> L, Lfr;
    // assets
    std::vector<uint8_t> tilesetBlob, spritesBlob;
    int tilesetCount = 0, tilesetCols = 0;
    std::vector<ImageId> tileImg;
    std::map<std::string, ImageId> spriteImg;
    std::vector<std::string> spriteNames;
    bool imagesReady = false;
    void ensureImages( SoftRenderer& r );
    ImageId spriteFor( const std::string& type ) const;
    // annuler / retablir : instantanes de la carte courante
    std::vector<std::vector<EdMap>> undoStack, redoStack;
    void snapshot();
    void doUndo( bool redo );
    // interaction
    bool mouseDown = false, panHeld = false, panning = false;
    int mouseX = 0, mouseY = 0, lastCellX = -1, lastCellY = -1, startCellX = 0, startCellY = 0;
    int dragOffX = 0, dragOffY = 0, panStartX = 0, panStartY = 0, panScrollX = 0, panScrollY = 0;
    bool dragMoved = false;
    int palScroll = 0;                              // premiere ligne visible de la palette
    // saisie d'une ligne de texte
    enum class Prompt { None, Props, NewMap, Goto, Music } promptKind = Prompt::None;
    std::string promptText, promptLabel;

    // conversions
    bool inCanvas( int x, int y ) const { return x >= 0 && y >= 0 && x < vw - kPanel && y < vh - kStatusH; }
    int cellX( int px ) const { return ( px + scrollX ) >= 0 ? ( px + scrollX ) / kTile : -1; }
    int cellY( int py ) const { return ( py + scrollY ) >= 0 ? ( py + scrollY ) / kTile : -1; }
    int logicX( int px ) const { return ( px + scrollX ) * 32 / kTile; }
    int logicY( int py ) const { return ( py + scrollY ) * 32 / kTile; }
    int toDispX( int lx ) const { return lx * kTile / 32 - scrollX; }
    int toDispY( int ly ) const { return ly * kTile / 32 - scrollY; }
    int palCols() const { return ( kPanel - 16 ) / kTile; }
    int palTop() const { return 78; }
    int palRows() const { return ( vh - kStatusH - palTop() - 4 ) / kTile; }
    void clampScroll();

    // edition
    void paintCell( int cx, int cy, int value );
    void paintLine( int x0, int y0, int x1, int y1, int value );
    void floodFill( int cx, int cy, int value );
    void fillRect( int x0, int y0, int x1, int y1, int value );
    void pickTile( int cx, int cy );
    bool selectAt( int px, int py );
    void placeAt( int cx, int cy );
    void deleteSelected();
    void nudgeSelected( int dx, int dy );
    void cycleType( int dir );
    std::string propsLine() const;
    bool applyProps( const std::string& line, std::string& why );
    bool validateRules( const std::string& rules, std::string& why ) const;
    void commitPrompt();
    void startPrompt( Prompt k, const std::string& label, const std::string& initial );
    bool newMap( const std::string& line );
    void switchMap( int delta );
    void gotoTile( int id );
    void onKey( const EdEvent& e );
    void setStatus( const std::string& s ) { status = s; }
    void runBuild();

    // rendu
    void drawCanvas( SoftRenderer& r );
    void drawPanel( SoftRenderer& r );
    void drawStatus( SoftRenderer& r );
    void drawObjects( SoftRenderer& r );
    void drawHelp( SoftRenderer& r );
    std::string toolName() const;
};
