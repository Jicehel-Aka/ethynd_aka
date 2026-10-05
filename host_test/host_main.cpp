// host_main.cpp -- test PC de shared/game : joue des scenarios d'entrees sans ecran et
// enregistre des captures (PPM) + des verifications simples.
#ifndef ETHYND_HOST_TEST
#define ETHYND_HOST_TEST
#endif
#include <cstdio>
#include <cstdlib>
#include <string>
#include "SoftRenderer.h"
#include "FileAssetSource.h"
#include "Game.h"

uint32_t platformMillis() { return 0; }

struct ScriptedInput : IInput {
    InputState cur, prev;
    void poll() override {}
    const InputState& state() const override { return cur; }
    bool justPressed( bool InputState::*b ) const override { return cur.*b && !( prev.*b ); }
    void set( InputState s ) { prev = cur; cur = s; }
};

struct LogAudio : NullAudio {
    int sfx[5] = {0}; Music last = Music::None; int musicChanges = 0;
    void playMusic( Music m ) override { last = m; musicChanges++; }
    void playSfx( Sfx s ) override { sfx[(int)s]++; }
};

static int failures = 0;
#define CHECK( c, msg ) do { if ( !( c ) ) { printf( "  ECHEC : %s\n", msg ); failures++; } else printf( "  ok    : %s\n", msg ); } while ( 0 )

static InputState keys( bool up, bool down, bool left, bool right, bool a = false )
{ InputState s; s.up = up; s.down = down; s.left = left; s.right = right; s.actionA = a; return s; }

int main( int argc, char** argv )
{
    std::string assets = argc > 1 ? argv[1] : "out24";
    std::string out    = argc > 2 ? argv[2] : "shots";
    SoftRenderer r; LogAudio audio; FileAssetSource src( assets ); ScriptedInput in;
    Game g( r, audio, src );
    g.setQuitHint( "MENU : menu systeme" );
    if ( !g.init() ) { printf( "init a echoue (assets introuvables dans %s ?)\n", assets.c_str() ); return 1; }
    printf( "assets charges : %zu images enregistrees\n", r.images.size() );

    auto shot = [&]( const char* n ) { r.fb.assign( r.fb.size(), 0 ); g.render(); r.savePPM( ( out + "/" + n + ".ppm" ).c_str() ); };
    auto tick = [&]( InputState s ) { in.set( s ); g.update( in ); };

    printf( "[menu]\n" );
    shot( "01_menu" );
    CHECK( g.screen() == Game::Screen::Menu, "demarre sur le menu" );
    CHECK( audio.last == Music::Menu, "musique du menu" );

    tick( keys( 0,0,0,0 ) );
    { InputState c; c.actionC = true; tick( c ); }
    CHECK( g.screen() == Game::Screen::Help, "C ouvre l'aide" );
    shot( "01b_aide" );
    { InputState b; b.actionB = true; tick( b ); }
    CHECK( g.screen() == Game::Screen::Menu, "B revient au menu" );
    tick( keys( 0,0,0,0 ) );

    printf( "[jeu]\n" );
    tick( keys( 0,0,0,0, true ) ); tick( keys( 0,0,0,0 ) );
    CHECK( g.screen() == Game::Screen::Playing, "A lance la partie" );
    CHECK( std::string( g.map().name() ) == "maison", "premiere carte : maison" );
    CHECK( g.map().camX == 14 && g.map().camY == 93, "camera de depart (14, 93)" );
    CHECK( audio.last == Music::Maison, "musique de la maison" );
    shot( "02_maison_depart" );

    // marche vers le bas : la camera descend de 3 par tick
    int y0 = g.map().camY;
    for ( int i = 0; i < 5; i++ ) tick( keys( 0,1,0,0 ) );
    CHECK( g.map().camY == y0 - 15, "5 ticks vers le bas : camera -15 (vitesse 3)" );
    shot( "03_maison_marche" );

    // la porte sous le point de depart mene a la grotte (zone x -19..26, y -180..-168 d'Ethynd)
    for ( int i = 0; i < 120 && std::string( g.map().name() ) == "maison"; i++ ) tick( keys( 0,1,0,0 ) );
    CHECK( std::string( g.map().name() ) == "grotte", "marcher tout droit vers le bas mene a la grotte" );
    CHECK( g.map().camX == -416 && g.map().camY == -180, "arrivee dans la grotte a (-416, -180)" );
    CHECK( audio.last == Music::Grotte, "musique de la grotte" );
    shot( "04_grotte_arrivee" );

    // remonter depuis l'arrivee dans la grotte ramene dans la maison (zone x -434..-398, y -138..-111)
    for ( int i = 0; i < 60 && std::string( g.map().name() ) == "grotte"; i++ ) tick( keys( 1,0,0,0 ) );
    CHECK( std::string( g.map().name() ) == "maison", "grotte -> maison en remontant" );
    CHECK( g.map().camX == 0 && g.map().camY == -125, "arrivee dans la maison a (0, -125)" );

    // dans la maison : un obstacle arrete la marche (la camera finit par ne plus bouger)
    for ( int i = 0; i < 300; i++ ) tick( keys( 1,0,0,0 ) );
    int yWall = g.map().camY;
    for ( int i = 0; i < 20; i++ ) tick( keys( 1,0,0,0 ) );
    CHECK( g.map().camY == yWall, "un obstacle arrete la marche vers le haut" );
    printf( "  (camera apres 300 ticks vers le haut : %d, %d)\n", g.map().camX, g.map().camY );

    // attaque : le joueur est bloque pendant l'animation (libre = false) et l'epee apparait
    tick( keys( 0,0,0,0 ) );
    tick( keys( 0,0,0,0, true ) );
    CHECK( g.playerRef().swordActive(), "attaque : epee active" );
    int camBefore = g.map().camX;
    tick( keys( 0,0,1,0 ) );
    CHECK( g.map().camX == camBefore, "pendant l'attaque, les fleches sont ignorees" );
    for ( int i = 0; i < 40; i++ ) tick( keys( 0,0,0,0 ) );
    CHECK( !g.playerRef().swordActive(), "l'attaque se termine" );
    CHECK( audio.sfx[(int)Sfx::Attack] >= 1, "son d'attaque joue" );
    shot( "05_apres_attaque" );

    // teleportations : camera forcee dans chaque zone d'Ethynd
    printf( "[teleportations]\n" );
    g.debugSetCamera( 120, -240 ); tick( keys( 0,0,0,0 ) );
    CHECK( std::string( g.map().name() ) == "aventure", "maison -> aventure" );
    CHECK( g.map().camX == -464 && g.map().camY == -261, "arrivee a (-464, -261)" );
    CHECK( audio.last == Music::Aventure, "musique de l'aventure" );
    shot( "06_aventure" );
    g.debugSetCamera( -464, -245 ); tick( keys( 0,0,0,0 ) );
    CHECK( std::string( g.map().name() ) == "maison", "aventure -> maison" );
    g.debugSetCamera( 120, -240 ); tick( keys( 0,0,0,0 ) );
    g.debugSetCamera( -1103, -440 ); tick( keys( 0,0,0,0 ) );
    CHECK( std::string( g.map().name() ) == "grotte", "aventure -> grotte" );
    CHECK( g.monstersRef().size() == 4, "4 chauves-souris dans la grotte" );
    shot( "07_grotte" );

    // monstres : ils bougent et ne traversent pas les murs (positions bornees a la carte)
    printf( "[monstres]\n" );
    int mx = g.monstersRef()[0].wx, my = g.monstersRef()[0].wy;
    for ( int i = 0; i < 200; i++ ) tick( keys( 0,0,0,0 ) );
    CHECK( g.monstersRef()[0].wx != mx || g.monstersRef()[0].wy != my, "une chauve-souris s'est deplacee" );
    shot( "08_grotte_monstres" );

    // mort : on laisse les chauves-souris toucher le joueur (camera placee sur un monstre)
    printf( "[mort]\n" );
    const Monster& m0 = g.monstersRef()[0];
    for ( int i = 0; i < 4000 && g.screen() == Game::Screen::Playing; i++ ) {
        // place la camera pour que le monstre 0 soit sous le joueur
        bool contact = ( i % 2 == 0 );          // un seul coup par contact : on alterne
        g.debugSetCamera( 308 - m0.wx + ( contact ? 0 : 200 ), 243 - m0.wy );
        tick( keys( 0,0,0,0 ) );
    }
    CHECK( g.screen() == Game::Screen::Dead, "le joueur peut mourir" );
    CHECK( g.playerRef().vie < 1, "vie a 0" );
    CHECK( audio.sfx[(int)Sfx::Hurt] >= 10, "10 blessures jouees" );
    shot( "09_mort" );
    tick( keys( 0,0,0,0, true ) );
    CHECK( g.screen() == Game::Screen::Menu, "A ramene au menu" );

    printf( failures ? "\n%d ECHEC(S)\n" : "\nTOUT EST OK\n", failures );
    return failures ? 1 : 0;
}
