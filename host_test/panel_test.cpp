// panel_test.cpp -- test sans fenetre du panneau de boutons de la version PC : zones tactiles, suivi des pointeurs,
// dessin (captures PPM pour verification visuelle). Usage : panel_test <dossier world> <dossier captures>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "ControlPanel.h"
#include "VirtualPad.h"
#include "WorldText.h"

static int failures = 0;
#define CHECK( c, msg ) do { if ( !( c ) ) { printf( "  ECHEC : %s\n", msg ); failures++; } else printf( "  ok    : %s\n", msg ); } while ( 0 )

int main( int argc, char** argv )
{
    if ( argc < 3 ) { printf( "usage : panel_test <world> <captures>\n" ); return 2; }
    std::string world = argv[1], shots = argv[2];
    std::map<std::string, std::string> fr, en;
    loadLangFile( world + "/lang/fr.txt", fr ); loadLangFile( world + "/lang/en.txt", en );
    auto trFor = [&]( std::map<std::string, std::string>& m ) { return [&m, &fr]( const std::string& k ) { auto it = m.find( k ); if ( it != m.end() && !it->second.empty() ) return it->second; auto f = fr.find( k ); return f != fr.end() ? f->second : k; }; };

    for ( int zoom = 2; zoom <= 4; zoom++ ) {
        printf( "[zoom %d : fenetre %dx%d dont panneau %dx%d]\n", zoom, 320 * zoom, 240 * zoom + ControlPanel::kHeightUnits * ControlPanel::kUnit, 320 * zoom, ControlPanel::kHeightUnits * ControlPanel::kUnit );
        ControlPanel p( zoom );
        bool allOk = true;
        for ( int b = 0; b < ControlPanel::BTN_COUNT; b++ ) { int cx, cy; p.centerOf( b, cx, cy ); if ( p.hit( cx, cy ) != b ) { allOk = false; printf( "    bouton %d : le centre (%d,%d) donne %d\n", b, cx, cy, p.hit( cx, cy ) ); } }
        CHECK( allOk, "le centre de chacun des 8 boutons le designe" );
        CHECK( p.hit( 2, p.height() - 2 ) == -1 && p.hit( p.width() / 2, 4 ) == -1, "coins et barre de titre : aucun bouton" );
        int ax, ay, bx, by; p.centerOf( ControlPanel::BTN_A, ax, ay ); p.centerOf( ControlPanel::BTN_B, bx, by );
        CHECK( ax < bx && ay == by, "ordre A, B, C, D de gauche a droite" );
        int cx0, cy0; p.centerOf( ControlPanel::BTN_UP, cx0, cy0 ); int dx0, dy0; p.centerOf( ControlPanel::BTN_RIGHT, dx0, dy0 );
        CHECK( p.hit( cx0, cy0 + 2 ) == ControlPanel::BTN_UP && p.hit( dx0 - 3, dy0 ) == ControlPanel::BTN_RIGHT, "croix : haut et droite" );
        { int dx, dy; p.centerOf( ControlPanel::BTN_LEFT, dx, dy ); int ex, ey; p.centerOf( ControlPanel::BTN_DOWN, ex, ey );
          CHECK( p.hit( dx, dy ) == ControlPanel::BTN_LEFT && p.hit( ex, ey ) == ControlPanel::BTN_DOWN, "croix : gauche et bas" ); }
        // pas de chevauchement entre les boutons ronds et la croix
        { int cdx, cdy; p.centerOf( ControlPanel::BTN_RIGHT, cdx, cdy ); CHECK( ax - ( ControlPanel::kUnit * 17 ) > cdx + ControlPanel::kUnit * 24, "la croix et le bouton A ne se touchent pas" ); }

        // suivi des pointeurs (souris et doigts)
        VirtualPad pad( [&]( int x, int y ) { return p.hit( x, y ); } );
        pad.down( -1, ax, ay );
        CHECK( pad.mask() == ( 1u << ControlPanel::BTN_A ), "souris sur A : A enfonce" );
        pad.move( -1, bx, by );
        CHECK( pad.mask() == ( 1u << ControlPanel::BTN_B ), "glisser de A a B : B seul enfonce" );
        pad.move( -1, 2, p.height() - 2 );
        CHECK( pad.mask() == 0, "glisser hors des boutons : rien d'enfonce" );
        pad.move( -1, ax, ay );
        CHECK( pad.mask() == ( 1u << ControlPanel::BTN_A ), "revenir sur A : de nouveau enfonce" );
        int lx, ly; p.centerOf( ControlPanel::BTN_LEFT, lx, ly );
        pad.down( 7, lx, ly );
        CHECK( pad.mask() == ( ( 1u << ControlPanel::BTN_A ) | ( 1u << ControlPanel::BTN_LEFT ) ), "deux doigts : A + gauche en meme temps" );
        pad.up( -1 );
        CHECK( pad.mask() == ( 1u << ControlPanel::BTN_LEFT ), "relacher A : reste gauche" );
        pad.up( 7 );
        CHECK( pad.mask() == 0, "tout relache" );
        pad.down( 3, 2, 2 ); CHECK( pad.mask() == 0, "toucher a cote : rien" );

        // captures : repos et touches enfoncees, en francais et en anglais
        InputState idle, pressed; pressed.right = true; pressed.actionA = true; pressed.actionC = true;
        auto fT = trFor( fr ), eT = trFor( en );
        p.draw( idle, fT ); p.savePPM( ( shots + "/panneau_fr_z" + std::to_string( zoom ) + ".ppm" ).c_str() );
        p.draw( pressed, eT ); p.savePPM( ( shots + "/panneau_en_appuye_z" + std::to_string( zoom ) + ".ppm" ).c_str() );
    }
    printf( failures ? "\n%d ECHEC(S)\n" : "\nTOUT EST OK\n", failures );
    return failures ? 1 : 0;
}
