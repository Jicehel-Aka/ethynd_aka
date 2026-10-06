// test_editor.cpp -- test sans ecran du cœur de l'editeur : edition, annuler/retablir, objets,
// saisie de proprietes, sauvegarde (identique a celle de Python), langues, capture de l'interface.
// Usage : test_editor <dossier world> <dossier assets editeur> <dossier de travail> <dossier captures>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include "EditorCore.h"

static int failures = 0;
#define CHECK( c, msg ) do { if ( !( c ) ) { printf( "  ECHEC : %s\n", msg ); failures++; } else printf( "  ok    : %s\n", msg ); } while ( 0 )

static std::string slurp( const std::string& p ) { std::ifstream f( p, std::ios::binary ); std::stringstream s; s << f.rdbuf(); return s.str(); }

struct Driver {
    EditorCore& ed;
    void mouse( EdEvent::Type t, int x, int y, int button = 1 ) { EdEvent e; e.type = t; e.x = x; e.y = y; e.button = button; ed.handle( e ); }
    void click( int x, int y, int button = 1 ) { mouse( EdEvent::MouseDown, x, y, button ); mouse( EdEvent::MouseUp, x, y, button ); }
    void drag( int x0, int y0, int x1, int y1 ) { mouse( EdEvent::MouseDown, x0, y0 ); mouse( EdEvent::MouseMove, x1, y1 ); mouse( EdEvent::MouseUp, x1, y1 ); }
    void key( int k, bool ctrl = false, bool shift = false ) { EdEvent e; e.type = EdEvent::KeyDown; e.key = k; e.ctrl = ctrl; e.shift = shift; ed.handle( e ); }
    void type( const std::string& s ) { EdEvent e; e.type = EdEvent::Text; e.text = s; ed.handle( e ); }
    void wheel( int x, int y, int dy ) { EdEvent e; e.type = EdEvent::Wheel; e.x = x; e.y = y; e.dy = dy; ed.handle( e ); }
    // pixel (fenetre) du centre de la case (cx, cy) de la carte courante
    int px( int cx ) { return cx * EditorCore::kTile - ed.scrollX + 12; }
    int py( int cy ) { return cy * EditorCore::kTile - ed.scrollY + 12; }
};

int main( int argc, char** argv )
{
    if ( argc < 5 ) { printf( "usage : test_editor <world> <assets> <workdir> <shots>\n" ); return 2; }
    std::string world = argv[1], assets = argv[2], work = argv[3], shots = argv[4];
    // copie de travail du monde : on n'ecrit jamais dans le vrai
    char cmd[1024];
    snprintf( cmd, sizeof cmd, "rm -rf '%s' && mkdir -p '%s' && cp -r '%s'/* '%s'/", work.c_str(), work.c_str(), world.c_str(), work.c_str() );
    if ( system( cmd ) != 0 ) return 2;

    EditorCore ed; std::string err;
    if ( !ed.open( work, assets, "fr", err ) ) { printf( "ouverture : %s\n", err.c_str() ); return 1; }
    Driver d{ ed };
    SoftRenderer r( 960, 600 );
    printf( "monde charge : %zu cartes, %zu tuiles a collision\n", ed.world.maps.size(), ed.world.collide.size() );

    printf( "[sauvegarde sans modification : identique a Python]\n" );
    CHECK( ed.saveAll(), "saveAll reussit" );
    bool same = true;
    for ( const EdMap& m : ed.world.maps ) {
        std::string a = slurp( world + "/maps/" + m.name + ".txt" ), b = slurp( work + "/maps/" + m.name + ".txt" );
        if ( a != b ) { same = false; printf( "    difference : %s (%zu / %zu octets)\n", m.name.c_str(), a.size(), b.size() ); }
    }
    CHECK( same, "les cartes relues puis reecrites sont identiques octet pour octet" );
    CHECK( slurp( world + "/world.txt" ) == slurp( work + "/world.txt" ), "world.txt identique" );
    CHECK( slurp( world + "/tiles.txt" ) == slurp( work + "/tiles.txt" ), "tiles.txt identique" );

    printf( "[peinture]\n" );
    ed.draw( r );                                           // charge les images
    int mi = ed.world.mapIndex( "maison" );
    while ( ed.curMap != mi ) d.key( ED_TAB );
    EdMap& m = ed.cur();
    ed.layer = 0; ed.selTile = 3736;
    int before = m.at( 0, 2, 2 );
    d.key( 'b' ); d.click( d.px( 2 ), d.py( 2 ) );
    CHECK( m.at( 0, 2, 2 ) == 3736, "pinceau : la case prend la tuile choisie" );
    d.drag( d.px( 3 ), d.py( 3 ), d.px( 8 ), d.py( 3 ) );
    CHECK( m.at( 0, 3, 3 ) == 3736 && m.at( 0, 5, 3 ) == 3736 && m.at( 0, 8, 3 ) == 3736, "pinceau : glisser peint sans trou" );
    d.key( 'z', true );
    CHECK( m.at( 0, 5, 3 ) == before || m.at( 0, 5, 3 ) != 3736, "Ctrl+Z annule le trait entier" );
    CHECK( m.at( 0, 2, 2 ) == 3736, "... mais pas le point precedent" );
    d.key( 'y', true );
    CHECK( m.at( 0, 5, 3 ) == 3736, "Ctrl+Y le retablit" );
    d.key( 'e' ); d.click( d.px( 2 ), d.py( 2 ) );
    CHECK( m.at( 0, 2, 2 ) == -1, "gomme : case videe" );
    d.key( 'z', true );
    d.key( 'f' ); ed.selTile = 12; d.click( d.px( 6 ), d.py( 8 ) );
    CHECK( m.at( 0, 6, 8 ) == 12 && m.at( 0, 7, 9 ) == 12, "remplir : la zone connexe prend la tuile" );
    d.key( 'r' ); ed.selTile = 40; d.drag( d.px( 1 ), d.py( 11 ), d.px( 3 ), d.py( 12 ) );
    CHECK( m.at( 0, 1, 11 ) == 40 && m.at( 0, 3, 12 ) == 40 && m.at( 0, 2, 12 ) == 40, "rectangle : 3x2 cases" );
    d.key( 'i' ); d.click( d.px( 1 ), d.py( 11 ) );
    CHECK( ed.selTile == 40, "pipette : prend la tuile sous le curseur" );
    ed.selTile = 77; d.click( d.px( 1 ), d.py( 11 ), 3 );
    CHECK( ed.selTile == 40, "clic droit : pipette" );
    d.key( '3' );
    CHECK( ed.layer == 2, "touche 3 : couche 3" );
    d.key( ED_F1 + 3 );
    CHECK( !ed.layerVisible[3], "F4 masque la couche 4" );
    d.key( ED_F1 + 3 );

    printf( "[objets]\n" );
    size_t nsp = m.spawns.size();
    d.key( 'm' ); d.click( d.px( 6 ), d.py( 12 ) );
    CHECK( m.spawns.size() == nsp + 1 && ed.selKind == EdSel::Spawn, "monstre place et selectionne" );
    CHECK( m.spawns.back().x == 6 * 32 && m.spawns.back().y == 12 * 32, "positionne sur la case (pixels logiques)" );
    d.key( 'p' );
    d.type( " " );                                          // la ligne de proprietes est preremplie : on la remplace
    for ( int i = 0; i < 60; i++ ) d.key( ED_BACKSPACE );
    d.type( "chat 192 384 50 0" ); d.key( ED_ENTER );
    CHECK( m.spawns.back().type == "chat" && m.spawns.back().vie == 50 && m.spawns.back().attaque == 0, "proprietes appliquees (type, vie, attaque)" );
    d.key( 'p' ); for ( int i = 0; i < 60; i++ ) d.key( ED_BACKSPACE ); d.type( "dragon_inconnu 0 0 1 1" ); d.key( ED_ENTER );
    CHECK( m.spawns.back().type == "chat", "type de monstre inconnu : refuse" );
    printf( "    message : %s\n", ed.status.c_str() );
    d.key( '.' );
    CHECK( m.spawns.back().type != "chat", "touche . : type suivant" );
    d.key( ED_RIGHT ); d.key( ED_DOWN, false, true );
    CHECK( m.spawns.back().x == 192 + 8 && m.spawns.back().y == 384 + 32, "fleches : 8 px, Maj : 32 px" );
    d.key( ED_DELETE );
    CHECK( m.spawns.size() == nsp && ed.selKind == EdSel::None, "Suppr supprime l'objet" );
    d.key( 'z', true );
    CHECK( m.spawns.size() == nsp + 1, "Ctrl+Z restaure l'objet supprime" );

    size_t nd = m.doors.size();
    d.key( 'd' ); d.drag( d.px( 2 ), d.py( 14 ), d.px( 3 ), d.py( 14 ) );
    CHECK( m.doors.size() == nd + 1, "porte creee en glissant" );
    CHECK( m.doors.back().w == 64 && m.doors.back().h == 32, "rectangle de 2x1 cases" );
    d.key( 'p' ); for ( int i = 0; i < 80; i++ ) d.key( ED_BACKSPACE ); d.type( "64 448 64 32 grotte 736 433 talked_sage deny_cave_house" ); d.key( ED_ENTER );
    CHECK( m.doors.back().map == "grotte" && m.doors.back().requires_ == "talked_sage", "porte : destination et condition" );
    d.key( 'p' ); for ( int i = 0; i < 80; i++ ) d.key( ED_BACKSPACE ); d.type( "64 448 64 32 grotte 736 433 drapeau_bidon -" ); d.key( ED_ENTER );
    CHECK( m.doors.back().requires_ == "talked_sage", "drapeau inconnu : refuse" );
    d.key( 'p' ); for ( int i = 0; i < 80; i++ ) d.key( ED_BACKSPACE ); d.type( "64 448 64 32 nulle_part 0 0 - -" ); d.key( ED_ENTER );
    CHECK( m.doors.back().map == "grotte", "carte inconnue : refuse" );

    d.key( 'n' ); d.click( d.px( 5 ), d.py( 6 ) );
    d.key( 'p' ); for ( int i = 0; i < 80; i++ ) d.key( ED_BACKSPACE ); d.type( "marchande sage 160 192 bas has:crystal?sage_thanks;talked_sage?sage_wait;sage_intro" ); d.key( ED_ENTER );
    CHECK( m.npcs.back().id == "marchande" && m.npcs.back().rules.find( "has:crystal" ) == 0, "PNJ : id, type et regles de dialogue" );
    d.key( 'p' ); for ( int i = 0; i < 160; i++ ) d.key( ED_BACKSPACE ); d.type( "x sage 160 192 bas dialogue_fantome" ); d.key( ED_ENTER );
    CHECK( m.npcs.back().rules.find( "has:crystal" ) == 0, "regle vers un dialogue inconnu : refusee" );
    d.key( 't' ); d.click( d.px( 3 ), d.py( 9 ) );
    CHECK( !m.items.empty() && m.items.back().id == "potion", "objet place (potion par defaut)" );
    d.key( 's' ); d.click( d.px( 7 ), d.py( 7 ) );
    CHECK( ed.world.startMap == "maison" && ed.world.startX == 7 * 32 + 16, "position de depart deplacee" );
    d.key( 'v' ); d.click( d.px( 5 ) - 4, d.py( 6 ) - 4 );
    CHECK( ed.selKind == EdSel::Npc, "outil Objets : un clic selectionne le PNJ" );
    d.drag( d.px( 5 ), d.py( 6 ), d.px( 8 ), d.py( 6 ) );
    CHECK( m.npcs.back().x != 5 * 32, "glisser deplace l'objet" );
    printf( "    deplace en x = %d (multiple de 8 : %s)\n", m.npcs.back().x, m.npcs.back().x % 8 == 0 ? "oui" : "non" );

    printf( "[collisions, palette]\n" );
    ed.selTile = 3736; bool had = ed.world.collide.count( 3736 );
    d.key( 'x' );
    CHECK( ed.world.collide.count( 3736 ) != (size_t)had, "X bascule la collision de la tuile" );
    d.key( 'x' );
    d.key( 'j' ); d.type( "1500" ); d.key( ED_ENTER );
    CHECK( ed.selTile == 1500, "J : aller a la tuile 1500" );
    d.click( 960 - 256 + 8 + 12, 78 + 12 );
    CHECK( ed.selTile != 1500, "clic dans la palette : choisit une tuile" );

    printf( "[cartes]\n" );
    d.key( 'n', true ); d.type( "donjon 20 12" ); d.key( ED_ENTER );
    CHECK( ed.world.mapNames.back() == "donjon" && ed.cur().w == 20 && ed.cur().h == 12, "Ctrl+N : nouvelle carte 20x12" );
    d.key( 'n', true ); d.type( "Mauvais Nom 20 12" ); d.key( ED_ENTER );
    CHECK( ed.world.mapNames.size() == 4, "nom invalide refuse" );
    d.key( 'm', true ); for ( int i = 0; i < 10; i++ ) d.key( ED_BACKSPACE ); d.type( "grotte" ); d.key( ED_ENTER );
    CHECK( ed.cur().music == "grotte", "Ctrl+M : musique de la carte" );
    ed.layer = 0; d.key( 'b' ); ed.selTile = 5; d.click( d.px( 0 ), d.py( 0 ) );
    CHECK( ed.cur().at( 0, 0, 0 ) == 5, "on peut peindre sur la nouvelle carte" );

    printf( "[sauvegarde et relecture]\n" );
    CHECK( ed.dirty, "etat modifie" );
    CHECK( ed.saveAll() && !ed.dirty, "Ctrl+S sauvegarde" );
    EditorCore ed2; CHECK( ed2.open( work, assets, "fr", err ), "le monde sauvegarde se relit" );
    CHECK( ed2.world.maps.size() == 4 && ed2.world.maps[3].name == "donjon" && ed2.world.maps[3].layer[0][0] == 5, "nouvelle carte presente avec sa tuile" );
    int mi2 = ed2.world.mapIndex( "maison" );
    CHECK( ed2.world.maps[mi2].doors.back().map == "grotte" && ed2.world.maps[mi2].npcs.back().id == "marchande", "portes et PNJ conserves" );

    printf( "[langues]\n" );
    EditorCore en; CHECK( en.open( work, assets, "en", err ), "ouverture en anglais" );
    CHECK( en.tr( "ed.tool.brush" ) == "Brush" && ed.tr( "ed.tool.brush" ) == "Pinceau", "interface : Brush / Pinceau" );
    EditorCore xx; xx.open( work, assets, "xx", err );
    CHECK( xx.tr( "ed.tool.brush" ) == "Pinceau", "langue inconnue : repli sur le francais" );

    printf( "[captures]\n" );
    d.key( 'z', true ); d.key( 'z', true );
    while ( ed.curMap != mi ) d.key( ED_TAB );
    ed.selKind = EdSel::None; ed.showHelp = false; d.key( 'v' ); d.click( d.px( 5 ) - 4, d.py( 6 ) - 4 );
    d.mouse( EdEvent::MouseMove, 300, 200 );
    ed.draw( r ); r.savePPM( ( shots + "/editeur_maison.ppm" ).c_str() );
    while ( ed.curMap != ed.world.mapIndex( "grotte" ) ) d.key( ED_TAB );
    d.key( 'h' ); ed.draw( r ); r.savePPM( ( shots + "/editeur_aide.ppm" ).c_str() );
    d.key( 'h' );
    EditorCore e2; e2.open( work, assets, "en", err ); SoftRenderer r2( 960, 600 );
    while ( e2.curMap != e2.world.mapIndex( "aventure" ) ) { EdEvent k; k.type = EdEvent::KeyDown; k.key = ED_TAB; e2.handle( k ); }
    e2.draw( r2 ); r2.savePPM( ( shots + "/editeur_aventure_en.ppm" ).c_str() );

    printf( failures ? "\n%d ECHEC(S)\n" : "\nTOUT EST OK\n", failures );
    return failures ? 1 : 0;
}
