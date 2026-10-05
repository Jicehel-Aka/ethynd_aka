// Game.h -- ecrans et boucle de jeu (port de fonctions/jeu.py).
//
// Ecrans : Menu (A = jouer, C = aide), Help (B = retour), Playing, Dead (A/B = menu).
// Un appel a update() = 1 tick logique (30 par seconde), comme tick(30) d'Ethynd.
//
// ECARTS VOLONTAIRES avec Ethynd (a signaler dans le portage) :
//  - seuls les monstres dont "attaque" > 0 blessent le joueur : dans l'original, le champ
//    "attaque" n'est jamais lu et le chat, les oiseaux et les poussins (attaque 0, marques
//    "invincible") blessent comme les chauves-souris ;
//  - un monstre mort n'a plus de hitbox (l'original laissait sa hitbox fantome en place) ;
//  - apres l'ecran de mort, retour au menu au lieu de quitter le programme.
#pragma once
#include <vector>
#include "AssetStore.h"
#include "GameMap.h"
#include "Characters.h"

class Game {
  public:
    Game( IRenderer& r, IAudio& a, IAssetSource& s ) : renderer( r ), audio( a ), source( s ) {}
    bool init();                          // charge les assets ; false si un fichier manque
    void update( const IInput& input );   // 1 tick
    void render();                        // dessine l'ecran courant (present() a la charge de l'appelant)

    // Textes dessines par-dessus les images de menu (les images d'origine parlaient des
    // touches du PC). code : "fr" (defaut) ou "en". quitHint : phrase propre a la plateforme
    // (AKA : "MENU : menu systeme", PC : "Echap : quitter"), ou "" pour rien.
    void setLanguage( const char* code );
    void setQuitHint( const char* text );

    enum class Screen { Menu, Help, Playing, Dead };
    Screen screen() const { return current; }
    const GameMap& map() const { return gmap; }
    const Player& playerRef() const { return player; }
    const std::vector<Monster>& monstersRef() const { return monsters; }
#ifdef ETHYND_HOST_TEST
    void debugSetCamera( int x, int y ) { gmap.camX = x; gmap.camY = y; }   // tests PC uniquement
#endif

  private:
    IRenderer& renderer;
    IAudio& audio;
    IAssetSource& source;
    AssetStore assets;
    ResolvedChar playerChar;
    ResolvedChar entityChars[8];
    GameMap gmap;
    Player player;
    std::vector<Monster> monsters;
    Screen current = Screen::Menu;
    int lang = 0;                         // 0 = fr, 1 = en
    char quitHint[40] = {0};
    void drawCentered( int cx, int y, const char* text );

    bool enterMap( const char* name, int camX, int camY );   // ecran de chargement + carte + monstres + musique
    void startGame();
    void teleport();
    void updatePlaying( const IInput& input );
    void drawHud();
};
