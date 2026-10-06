// host_main.cpp -- test PC de shared/game : joue l'histoire complete sans ecran (entrees scriptees),
// verifie le monde (positions atteignables) et enregistre des captures PPM.
#ifndef ETHYND_HOST_TEST
#define ETHYND_HOST_TEST
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <set>
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
    int sfx[5] = {0}; Music last = Music::None;
    void playMusic( Music m ) override { last = m; }
    void playSfx( Sfx s ) override { sfx[(int)s]++; }
};

static int failures = 0;
#define CHECK( c, msg ) do { if ( !( c ) ) { printf( "  ECHEC : %s\n", msg ); failures++; } else printf( "  ok    : %s\n", msg ); } while ( 0 )

static InputState keys( bool up, bool down, bool left, bool right, bool a = false, bool b = false )
{ InputState s; s.up = up; s.down = down; s.left = left; s.right = right; s.actionA = a; s.actionB = b; return s; }
static InputState press( bool a, bool b = false, bool c = false ) { InputState s; s.actionA = a; s.actionB = b; s.actionC = c; return s; }

int main( int argc, char** argv )
{
    std::string assets = argc > 1 ? argv[1] : "out24";
    std::string out    = argc > 2 ? argv[2] : "shots";
    SoftRenderer r; LogAudio audio; FileAssetSource src( assets ); ScriptedInput in;
    Game g( r, audio, src );
    g.setQuitHintKey( "ui.quit.aka" );
    if ( !g.init() ) { printf( "init a echoue (assets introuvables dans %s ?)\n", assets.c_str() ); return 1; }
    const WorldData& w = g.worldData();
    printf( "assets charges : %zu images, %zu cartes, %zu dialogues, %zu objectifs\n", r.images.size(), w.mapNames.size(), w.dialogues.size(), w.objectives.size() );

    auto shot = [&]( const char* n ) { r.fb.assign( r.fb.size(), 0 ); g.render(); r.savePPM( ( out + "/" + n + ".ppm" ).c_str() ); };
    auto tick = [&]( InputState s ) { in.set( s ); g.update( in ); };
    auto tapA = [&]() { tick( press( true ) ); tick( press( false ) ); };
    auto skipDialogue = [&]( int maxTaps = 40 ) {            // A jusqu'a la fin du dialogue
        int n = 0;
        while ( g.screen() == Game::Screen::Dialogue && n++ < maxTaps ) { for ( int i = 0; i < 30; i++ ) tick( press( false ) ); tapA(); }
    };
    auto mapIdx = [&]( const char* n ) { return w.mapIndex( n ); };

    // ------------------------------------------------------------------ menu, langues
    printf( "[menu et langues]\n" );
    shot( "01_menu_fr" );
    CHECK( g.screen() == Game::Screen::Menu, "demarre sur le menu" );
    CHECK( std::string( g.tr( "ui.menu.play" ) ) == "Jouer", "francais : 'Jouer'" );
    g.setLanguage( "en" );
    CHECK( std::string( g.tr( "ui.menu.play" ) ) == "Play", "anglais : 'Play' (changement de langue a chaud)" );
    shot( "02_menu_en" );
    g.setLanguage( "xx" );
    CHECK( std::string( g.tr( "ui.menu.play" ) ) == "Jouer", "langue inconnue : repli sur le francais" );
    CHECK( std::string( g.tr( "cle.inexistante" ) ) == "cle.inexistante", "cle absente : la cle elle-meme s'affiche" );
    g.setLanguage( "fr" );
    tick( press( false, false, true ) ); tick( press( false ) );
    CHECK( g.screen() == Game::Screen::Help, "C ouvre l'aide" );
    shot( "03_aide_fr" );
    g.setLanguage( "en" ); shot( "04_aide_en" ); g.setLanguage( "fr" );
    tick( press( false, true ) ); tick( press( false ) );
    CHECK( g.screen() == Game::Screen::Menu, "B revient au menu" );

    // ------------------------------------------------------------------ debut de partie
    printf( "[debut de partie]\n" );
    tapA();
    CHECK( g.screen() == Game::Screen::Playing, "A lance la partie" );
    CHECK( std::string( g.map().name() ) == "maison", "on commence dans la maison" );
    CHECK( audio.last == Music::Maison, "musique de la maison" );
    CHECK( g.npcsRef().size() == 1, "un PNJ dans la maison (le sage)" );
    int obj = g.currentObjective();
    CHECK( obj >= 0 && std::string( g.tr( w.objectives[obj].title ) ) == "Parler au sage dans la maison", "premier objectif : parler au sage" );
    shot( "05_maison_depart" );

    // porte verrouillee tant que le sage n'a pas parle
    printf( "[porte verrouillee]\n" );
    g.debugTeleport( mapIdx( "maison" ), 316, 396 );
    const DoorObj* cellar = nullptr;                                     // l'escalier de la cave : porte vers la grotte
    for ( const DoorObj& d : g.map().objects().doors ) if ( d.destMap == mapIdx( "grotte" ) ) cellar = &d;
    CHECK( cellar != nullptr, "la maison a une porte vers la grotte" );
    auto approach = [&]( const DoorObj& d, int& ax, int& ay ) {          // point d'ou une ligne droite vers le bas atteint la porte
        for ( int x = d.r.x + d.r.w / 2; x >= d.r.x; x-- )
            for ( int y0 = d.r.y - 12; y0 >= d.r.y - 80; y0 -= 3 ) {
                bool ok = true;
                for ( int y = y0; y <= d.r.y + d.r.h / 2 && ok; y += 3 )
                    if ( g.map().collidesWorld( Rect{ x - 12, y - 10, kPlayerHitW, kPlayerHitH } ) ) ok = false;
                if ( ok ) { ax = x; ay = y0; return true; }
            }
        return false;
    };
    int ax = 0, ay = 0;
    CHECK( cellar && approach( *cellar, ax, ay ), "un chemin droit mene a l'escalier" );
    g.debugTeleport( mapIdx( "maison" ), ax, ay );
    for ( int i = 0; i < 90 && g.screen() == Game::Screen::Playing; i++ ) tick( keys( 0, 1, 0, 0 ) );
    CHECK( g.screen() == Game::Screen::Dialogue, "l'escalier de la cave est verrouille : un dialogue s'ouvre" );
    CHECK( std::string( g.map().name() ) == "maison", "on reste dans la maison" );
    for ( int i = 0; i < 60; i++ ) tick( press( false ) );
    shot( "06_dialogue_porte" );
    skipDialogue();
    CHECK( g.screen() == Game::Screen::Playing, "le dialogue se ferme avec A" );

    // ------------------------------------------------------------------ parler au sage
    printf( "[le sage]\n" );
    g.debugTeleport( mapIdx( "maison" ), 306, 160 );
    for ( int i = 0; i < 80; i++ ) tick( keys( 0, 0, 1, 0 ) );          // marche vers la gauche : le PNJ bloque
    const Npc& sage = g.npcsRef()[0];
    Rect pw{ Player::hitbox().x - g.map().camX, Player::hitbox().y - g.map().camY, kPlayerHitW, kPlayerHitH };
    CHECK( pw.x >= sage.wx + 32 - 2 && pw.x <= sage.wx + 32 + 4, "le sage est un obstacle (le joueur s'arrete contre lui)" );
    tick( press( false ) );
    shot( "07_pres_du_sage" );
    tapA();
    CHECK( g.screen() == Game::Screen::Dialogue, "A devant le sage : dialogue (pas d'attaque)" );
    for ( int i = 0; i < 60; i++ ) tick( press( false ) );
    shot( "08_dialogue_sage" );
    skipDialogue();
    CHECK( g.quest().flag( 0 ), "drapeau 'talked_sage' pose" );
    CHECK( g.quest().done( 0 ), "objectif 1 termine" );
    CHECK( g.currentObjective() == 1, "objectif suivant : entrer dans la grotte" );
    for ( int i = 0; i < 80; i++ ) tick( press( false ) );
    shot( "09_objectif_accompli" );
    tapA();                                                              // reparler : dialogue d'attente
    CHECK( g.screen() == Game::Screen::Dialogue, "reparler au sage : autre dialogue" );
    skipDialogue();

    // ------------------------------------------------------------------ journal
    tick( press( false, true ) ); tick( press( false ) );
    CHECK( g.screen() == Game::Screen::Log, "B ouvre le journal des objectifs" );
    shot( "10_journal" );
    tick( press( false, true ) ); tick( press( false ) );
    CHECK( g.screen() == Game::Screen::Playing, "B ferme le journal" );

    // ------------------------------------------------------------------ la grotte
    printf( "[la grotte]\n" );
    g.debugTeleport( mapIdx( "maison" ), ax, ay );
    for ( int i = 0; i < 90 && std::string( g.map().name() ) == "maison"; i++ ) tick( keys( 0, 1, 0, 0 ) );
    CHECK( std::string( g.map().name() ) == "grotte", "l'escalier mene a la grotte une fois le sage vu" );
    CHECK( audio.last == Music::Grotte, "musique de la grotte" );
    CHECK( g.monstersRef().size() == 4, "4 chauves-souris" );
    tick( press( false ) );
    CHECK( g.quest().done( 1 ), "objectif 'entrer dans la grotte' termine" );
    shot( "11_grotte" );

    // 4 chauves-souris : on place la camera pour qu'elles passent sous l'epee, A maintenu
    int lifeBefore = g.playerRef().vie;
    for ( size_t k = 0; k < 4; k++ ) {
        for ( int i = 0; i < 80 && g.monstersRef()[k].alive(); i++ ) {
            const Monster& m = g.monstersRef()[k];
            g.debugSetCamera( 300 - m.wx, 252 - m.wy );
            tick( keys( 0, 0, 0, 0, true ) );
        }
    }
    CHECK( g.quest().kills[1] >= 4 || g.quest().done( 2 ), "4 chauves-souris vaincues" );
    CHECK( g.quest().done( 2 ), "objectif 'vaincre 4 chauves-souris' termine" );
    printf( "  (vie : %d -> %d)\n", lifeBefore, g.playerRef().vie );
    CHECK( g.playerRef().vie >= 1, "le joueur a survecu" );

    // le Cristal
    g.debugTeleport( mapIdx( "grotte" ), 96 + 16, 640 + 16 );
    for ( int i = 0; i < 4; i++ ) tick( press( false ) );
    CHECK( g.quest().items[1] == 1, "Cristal ramasse" );
    CHECK( g.quest().done( 3 ), "objectif 'retrouver le Cristal' termine" );
    for ( int i = 0; i < 20; i++ ) tick( press( false ) );
    shot( "12_cristal" );

    // ------------------------------------------------------------------ rapporter le cristal
    printf( "[victoire]\n" );
    g.debugTeleport( mapIdx( "maison" ), 306, 160 );
    for ( int i = 0; i < 80; i++ ) tick( keys( 0, 0, 1, 0 ) );
    tapA();
    CHECK( g.screen() == Game::Screen::Dialogue, "le sage reconnait le Cristal" );
    skipDialogue();
    CHECK( g.screen() == Game::Screen::Win, "fin du dialogue : ecran de victoire" );
    CHECK( g.quest().flag( 1 ) && g.quest().items[1] == 0, "Cristal remis au sage" );
    CHECK( g.quest().done( 4 ), "dernier objectif termine" );
    shot( "13_victoire_fr" );
    g.setLanguage( "en" ); shot( "14_victoire_en" ); g.setLanguage( "fr" );
    tapA();
    CHECK( g.screen() == Game::Screen::Menu, "A ramene au menu" );

    // ------------------------------------------------------------------ potions, mort
    printf( "[potions et mort]\n" );
    tapA();                                                              // nouvelle partie : l'etat est remis a zero
    CHECK( !g.quest().flag( 0 ) && g.quest().objDone == 0, "nouvelle partie : quete remise a zero" );
    g.debugTeleport( mapIdx( "grotte" ), 300, 250 );
    for ( int i = 0; i < 4000 && g.screen() == Game::Screen::Playing; i++ ) {
        const Monster& m0 = g.monstersRef()[0];
        bool contact = ( i % 2 == 0 );                                   // un seul coup par contact : on alterne
        g.debugSetCamera( 308 - m0.wx + ( contact ? 0 : 200 ), 243 - m0.wy );
        tick( keys( 0, 0, 0, 0 ) );
    }
    CHECK( g.screen() == Game::Screen::Dead, "le joueur peut mourir" );
    shot( "15_mort_fr" ); g.setLanguage( "en" ); shot( "16_mort_en" ); g.setLanguage( "fr" );
    tapA();
    CHECK( g.screen() == Game::Screen::Menu, "A ramene au menu apres la mort" );

    // ------------------------------------------------------------------ le monde est-il jouable ?
    // Parcours en largeur sur le reseau EXACT du jeu : le joueur avance de 3 px par tick sur
    // chaque axe a partir de son point d'arrivee. Une position n'est atteignable que si la hitbox
    // 25x20 ne touche ni tuile a collision ni PNJ.
    printf( "[verification du monde : tout est-il atteignable ?]\n" );
    tapA();                                                              // dans une partie, pour disposer des cartes
    auto center = [&]( const Rect& rc ) { return std::pair<int,int>( rc.x + rc.w / 2, rc.y + rc.h / 2 ); };
    int unreachable = 0;
    std::vector<std::vector<uint8_t>> reachSaved;                        // memoire des cartes pour --near
    struct Sug { std::string map; int x, y; };
    std::vector<Sug> wanted;
    for ( int i = 3; i + 3 < argc; i += 4 ) if ( !strcmp( argv[i], "--near" ) ) wanted.push_back( { argv[i + 1], atoi( argv[i + 2] ), atoi( argv[i + 3] ) } );
    for ( int mi = 0; mi < (int)w.mapNames.size(); mi++ ) {
        std::vector<std::pair<int,int>> entries;                         // arrivees possibles sur cette carte
        if ( mi == w.startMap ) entries.push_back( { w.startX, w.startY } );
        for ( int mj = 0; mj < (int)w.mapNames.size(); mj++ ) {
            g.debugTeleport( mj, 0, 0 );
            for ( const DoorObj& d : g.map().objects().doors ) if ( d.destMap == mi ) entries.push_back( { d.dx, d.dy } );
        }
        g.debugTeleport( mi, entries[0].first, entries[0].second );
        const GameMap& gm = g.map();
        const int W = gm.mapWidth() * 32, H = gm.mapHeight() * 32;
        std::vector<uint8_t> seen( (size_t)W * H, 0 );
        std::vector<Rect> npcRects; for ( const Npc& n : g.npcsRef() ) npcRects.push_back( n.rect() );
        auto free = [&]( int cx, int cy ) {
            if ( cx < 12 || cy < 10 || cx >= W - 12 || cy >= H - 10 ) return false;
            Rect pr{ cx - 12, cy - 10, kPlayerHitW, kPlayerHitH };
            if ( gm.collidesWorld( pr ) ) return false;
            for ( const Rect& nr : npcRects ) if ( rectsOverlap( pr, nr ) ) return false;
            return true;
        };
        std::queue<int> q;
        for ( auto& e : entries ) if ( free( e.first, e.second ) && !seen[(size_t)e.second * W + e.first] ) { seen[(size_t)e.second * W + e.first] = 1; q.push( e.second * W + e.first ); }
        size_t count = 0;
        while ( !q.empty() ) {
            int cur = q.front(); q.pop(); count++;
            int x = cur % W, y = cur / W;
            const int dx[4] = { 3, -3, 0, 0 }, dy[4] = { 0, 0, 3, -3 };
            for ( int k = 0; k < 4; k++ ) {
                int nx = x + dx[k], ny = y + dy[k];
                if ( nx < 0 || ny < 0 || nx >= W || ny >= H || seen[(size_t)ny * W + nx] || !free( nx, ny ) ) continue;
                seen[(size_t)ny * W + nx] = 1; q.push( ny * W + nx );
            }
        }
        auto reach = [&]( int cx, int cy, int slack ) {
            for ( int ddx = -slack; ddx <= slack; ddx++ ) for ( int ddy = -slack; ddy <= slack; ddy++ ) {
                int x = cx + ddx, y = cy + ddy;
                if ( x >= 0 && y >= 0 && x < W && y < H && seen[(size_t)y * W + x] ) return true;
            }
            return false;
        };
        printf( "  carte %-9s : %zu positions atteignables (%zu points d'arrivee)\n", w.mapNames[mi].c_str(), count, entries.size() );
        const MapObjects& ob = gm.objects();
        for ( const ItemObj& it : ob.items ) { auto c = center( Rect{ it.x, it.y, 32, 32 } ); if ( !reach( c.first, c.second, 10 ) ) { printf( "    INATTEIGNABLE : objet %d en (%d,%d)\n", it.def, it.x, it.y ); unreachable++; } }
        for ( const DoorObj& d : ob.doors ) {
            bool ok = false;                                           // un point de la porte doit etre atteignable
            for ( int yy = d.r.y; yy < d.r.y + d.r.h && !ok; yy++ ) for ( int xx = d.r.x; xx < d.r.x + d.r.w; xx++ ) if ( xx >= 0 && yy >= 0 && xx < W && yy < H && seen[(size_t)yy * W + xx] ) { ok = true; break; }
            if ( !ok ) { printf( "    INATTEIGNABLE : porte en (%d,%d,%dx%d)\n", d.r.x, d.r.y, d.r.w, d.r.h ); unreachable++; }
        }
        for ( const NpcObj& n : ob.npcs ) {
            auto c = center( Rect{ n.x, n.y, 32, 32 } );
            if ( !reach( c.first, c.second, 36 ) ) { printf( "    INATTEIGNABLE : PNJ en (%d,%d)\n", n.x, n.y ); unreachable++; }
            if ( gm.collidesWorld( Rect{ n.x, n.y, 32, 32 } ) ) { printf( "    PNJ dans un mur en (%d,%d)\n", n.x, n.y ); unreachable++; }
        }
        for ( const SpawnObj& s : ob.spawns ) if ( gm.collidesWorld( Rect{ s.x, s.y, 32, 32 } ) ) { printf( "    monstre dans un mur en (%d,%d)\n", s.x, s.y ); unreachable++; }
        for ( const Sug& sg : wanted ) {                                // --near : plus proche position atteignable
            if ( sg.map != w.mapNames[mi] ) continue;
            int best = 1 << 30, bx = -1, by = -1;           // position libre pour un objet/PNJ/monstre 32x32 ET a portee du joueur
            for ( int y = 16; y < H - 16; y += 2 ) for ( int x = 16; x < W - 16; x += 2 ) {
                int d = ( x - sg.x ) * ( x - sg.x ) + ( y - sg.y ) * ( y - sg.y );
                if ( d >= best || gm.collidesWorld( Rect{ x - 16, y - 16, 32, 32 } ) || !reach( x, y, 8 ) ) continue;
                best = d; bx = x; by = y;
            }
            printf( "    suggestion %s (%d,%d) -> coin haut-gauche libre (%d,%d)\n", sg.map.c_str(), sg.x, sg.y, bx - 16, by - 16 );
        }
    }
    CHECK( unreachable == 0, "tous les objets, portes et PNJ sont atteignables, rien n'est dans un mur" );

    printf( failures ? "\n%d ECHEC(S)\n" : "\nTOUT EST OK\n", failures );
    return failures ? 1 : 0;
}
