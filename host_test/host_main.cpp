// host_main.cpp -- test PC de shared/game : joue l'histoire complete sans ecran (entrees scriptees),
// verifie le monde (positions atteignables) et enregistre des captures PPM.
#ifndef ETHYND_HOST_TEST
#define ETHYND_HOST_TEST
#endif
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <queue>
#include <set>
#include <string>
#include "SoftRenderer.h"
#include "FileAssetSource.h"
#include "Game.h"
#include "FixedStep.h"

uint32_t platformMillis() { return 0; }

struct ScriptedInput : IInput {
    InputState cur, prev;
    void poll() override {}
    const InputState& state() const override { return cur; }
    bool justPressed( bool InputState::*b ) const override { return cur.*b && !( prev.*b ); }
    void set( InputState s ) { prev = cur; cur = s; }
};

struct LogAudio : NullAudio {
    int sfx[6] = {0}; Music last = Music::None;
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
    CHECK( g.monstersRef().size() == 5, "4 chauves-souris et le dragon rouge" );
    { const Monster& dr = g.monstersRef()[4];
      CHECK( dr.boss() && dr.size == 56 && dr.radius == 170 && dr.vie == 60, "le dragon : gardien de zone, hitbox 56 px, rayon 170, 60 points de vie" ); }
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
    CHECK( g.quest().done( 2 ), "objectif 'vaincre 4 chauves-souris' termine" );
    CHECK( g.currentObjective() == 3, "objectif suivant : vaincre le dragon rouge" );
    printf( "  (vie : %d -> %d)\n", lifeBefore, g.playerRef().vie );
    CHECK( g.playerRef().vie >= 1, "le joueur a survecu" );

    // ------------------------------------------------------------------ le dragon, gardien du Cristal
    printf( "[le dragon]\n" );
    int dragonKind = 0; for ( int t = 0; t < kEntityDefCount; t++ ) if ( std::string( kEntityNames[t] ) == "dragon_rouge" ) dragonKind = t;
    // Le dragon est isole des chauves-souris (qui reapparaissent a chaque entree) pour ne tester que lui.
    auto boss = [&]() -> const Monster& { return g.monstersRef()[0]; };
    auto enterAt = [&]( int px, int py ) { g.debugTeleport( mapIdx( "grotte" ), px, py ); g.debugKeepOnlyBosses(); };
    // zone : le dragon ne s'eloigne jamais de plus de 170 px de son point d'attache quand le joueur est loin
    enterAt( 736, 433 );
    CHECK( g.monstersRef().size() == 1 && boss().boss(), "le dragon est seul dans le test" );
    { const Monster& dr = boss(); int hx = dr.homeX + 28, hy = dr.homeY + 28; double worst = 0; int moved = 0;
      for ( int i = 0; i < 3000; i++ ) {
          g.debugSetCamera( kLogicCx - 736, kPlayerHitCy - 433 );     // le joueur reste loin
          tick( keys( 0, 0, 0, 0 ) );
          double d = std::sqrt( (double)( ( dr.wx + 28 - hx ) * ( dr.wx + 28 - hx ) + ( dr.wy + 28 - hy ) * ( dr.wy + 28 - hy ) ) );
          if ( d > worst ) worst = d;
          if ( dr.wx != dr.homeX || dr.wy != dr.homeY ) moved = 1;
      }
      printf( "    (ecart maximal au point d'attache en 3000 ticks : %.0f px)\n", worst );
      CHECK( moved && worst <= 170 + 4, "le dragon rode mais reste dans sa zone (rayon 170)" ); }
    // poursuite : le joueur entre dans la zone, le dragon s'approche
    enterAt( 400, 650 );
    { const Monster& dr = boss(); int x0 = dr.wx;
      for ( int i = 0; i < 40; i++ ) { g.debugSetCamera( kLogicCx - 400, kPlayerHitCy - 650 ); tick( keys( 0, 0, 0, 0 ) ); }
      printf( "    (poursuite : x %d -> %d en 40 ticks)\n", x0, dr.wx );
      shot( "11a_dragon_vivant" );
      CHECK( dr.wx > x0 + 20, "le joueur dans la zone : le dragon se rapproche" ); }
    // le Cristal est cache tant que le dragon vit
    enterAt( 96 + 16, 640 + 16 );
    for ( int i = 0; i < 4; i++ ) tick( press( false ) );
    CHECK( g.quest().items[1] == 0, "le Cristal n'est pas ramassable tant que le dragon n'est pas vaincu" );
    // le combat : camera calee pour que le dragon passe sous l'epee, A maintenu
    enterAt( 400, 650 );
    g.debugHeal( 10 );                                                // le joueur boit une potion avant d'entrer dans la salle
    tick( keys( 0, 1, 0, 0 ) );                                       // face au bas : l'epee part vers le bas
    int fightTicks = 0, lifeFight = g.playerRef().vie;
    for ( ; fightTicks < 900 && g.monstersRef().size() == 1 && boss().alive() && g.screen() == Game::Screen::Playing; fightTicks++ ) {
        const Monster& dr = boss();
        g.debugSetCamera( 285 - dr.wx, 235 - dr.wy );
        tick( keys( 0, 0, 0, 0, true ) );
    }
    printf( "    (combat : %d ticks, vie %d -> %d, cris du dragon : %d)\n", fightTicks, lifeFight, g.playerRef().vie, audio.sfx[(int)Sfx::BossHit] );
    CHECK( g.screen() == Game::Screen::Playing && g.playerRef().vie >= 1, "le joueur gagne le combat" );
    CHECK( g.quest().kills[dragonKind] >= 1 && g.quest().done( 3 ), "dragon vaincu : objectif termine" );
    CHECK( fightTicks > 40, "le dragon n'est pas un ennemi de 10 ticks (60 points de vie)" );
    CHECK( audio.sfx[(int)Sfx::BossHit] > 0, "cri du dragon joue" );
    shot( "11b_dragon" );
    g.debugTeleport( mapIdx( "grotte" ), 400, 650 );
    CHECK( g.monstersRef().size() == 4, "le boss vaincu ne reapparait pas en revenant dans la carte" );

    // le Cristal, maintenant disponible
    g.debugTeleport( mapIdx( "grotte" ), 96 + 16, 640 + 16 );
    for ( int i = 0; i < 4; i++ ) tick( press( false ) );
    CHECK( g.quest().items[1] == 1, "Cristal ramasse" );
    CHECK( g.quest().done( 4 ), "objectif 'retrouver le Cristal' termine" );
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
    CHECK( g.quest().done( 5 ), "dernier objectif termine" );
    shot( "13_victoire_fr" );
    g.setLanguage( "en" ); shot( "14_victoire_en" ); g.setLanguage( "fr" );
    tapA();
    CHECK( g.screen() == Game::Screen::Menu, "A ramene au menu" );

    // ------------------------------------------------------------------ potions, mort
    printf( "[potions et mort]\n" );
    tapA();                                                              // nouvelle partie : l'etat est remis a zero
    CHECK( !g.quest().flag( 0 ) && g.quest().objDone == 0, "nouvelle partie : quete remise a zero" );
    { g.debugTeleport( mapIdx( "grotte" ), 230, 650 );                       // sur le dragon, sans attaquer : le contact fait mal
      int v0 = g.playerRef().vie;
      tick( keys( 0, 0, 0, 0 ) ); tick( keys( 0, 0, 0, 0 ) );
      CHECK( g.playerRef().vie == v0 - 2, "contact avec le dragon : 2 points de degats (attaque 4)" );
      for ( int i = 0; i < kHurtRepeat; i++ ) { g.debugSetCamera( kLogicCx - 230, kPlayerHitCy - 650 ); tick( keys( 0, 0, 0, 0 ) ); }
      CHECK( g.playerRef().vie == v0 - 4, "contact prolonge : un nouveau coup toutes les 1,5 s" ); }
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
    struct Sug { std::string map; int x, y, size; };
    std::vector<Sug> wanted;
    for ( int i = 3; i + 3 < argc; i += 4 ) {                     // --near carte x y (objet 32x32) | --near56 carte x y (dragon 56x56)
        if ( !strcmp( argv[i], "--near" ) ) wanted.push_back( { argv[i + 1], atoi( argv[i + 2] ), atoi( argv[i + 3] ), 32 } );
        if ( !strcmp( argv[i], "--near56" ) ) wanted.push_back( { argv[i + 1], atoi( argv[i + 2] ), atoi( argv[i + 3] ), 56 } );
    }
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
        for ( const SpawnObj& s : ob.spawns ) {
            int sz = s.type == "dragon_rouge" ? 56 : 32;                     // hitbox = taille du sprite
            if ( gm.collidesWorld( Rect{ s.x, s.y, sz, sz } ) ) { printf( "    monstre dans un mur en (%d,%d)\n", s.x, s.y ); unreachable++; }
            if ( s.radius > 0 && !reach( s.x + sz / 2, s.y + sz / 2, 70 ) ) { printf( "    INATTEIGNABLE : boss en (%d,%d)\n", s.x, s.y ); unreachable++; }
        }
        for ( const Sug& sg : wanted ) {                                // --near : plus proche position atteignable
            if ( sg.map != w.mapNames[mi] ) continue;
            int best = 1 << 30, bx = -1, by = -1;           // position libre pour un objet/PNJ/monstre 32x32 ET a portee du joueur
            const int hs = sg.size / 2;
            for ( int y = hs; y < H - hs; y += 2 ) for ( int x = hs; x < W - hs; x += 2 ) {
                int d = ( x - sg.x ) * ( x - sg.x ) + ( y - sg.y ) * ( y - sg.y );
                if ( d >= best || gm.collidesWorld( Rect{ x - hs, y - hs, sg.size, sg.size } ) || !reach( x, y, 8 ) ) continue;
                best = d; bx = x; by = y;
            }
            printf( "    suggestion %s (%d,%d) taille %d -> coin haut-gauche libre (%d,%d)\n", sg.map.c_str(), sg.x, sg.y, sg.size, bx - hs, by - hs );
        }
    }
    CHECK( unreachable == 0, "tous les objets, portes et PNJ sont atteignables, rien n'est dans un mur" );

    // ------------------------------------------------------------------ bord de carte, diagnostic, budget de rendu
    printf( "[bord de carte]\n" );
    g.debugTeleport( mapIdx( "maison" ), 60, 300 );
    for ( int i = 0; i < 120; i++ ) tick( keys( 0, 0, 1, 0 ) );             // marche vers la gauche, longtemps
    { Rect p{ Player::hitbox().x - g.map().camX, Player::hitbox().y - g.map().camY, kPlayerHitW, kPlayerHitH };
      CHECK( p.x >= 0 && std::string( g.map().name() ) == "maison", "le joueur ne sort pas de la carte par la gauche" ); }
    for ( int i = 0; i < 400; i++ ) tick( keys( 1, 0, 0, 0 ) );             // ... ni par le haut
    { Rect p{ Player::hitbox().x - g.map().camX, Player::hitbox().y - g.map().camY, kPlayerHitW, kPlayerHitH };
      CHECK( p.y >= 0, "le joueur ne sort pas de la carte par le haut" ); }

    printf( "[diagnostic d'un fichier manquant]\n" );
    { SoftRenderer r3; NullAudio a3; FileAssetSource bad( "/tmp/dossier_inexistant_ethynd" ); Game g3( r3, a3, bad );
      CHECK( !g3.init(), "init echoue si les assets manquent" );
      CHECK( std::string( g3.initError() ).find( "tiles.bin" ) != std::string::npos, "le message nomme le fichier manquant" );
      printf( "    message : %s\n", g3.initError() ); }

    printf( "[budget de rendu : pire cas par carte (appels drawImage / pixels par image)]\n" );
    for ( int mi = 0; mi < (int)w.mapNames.size(); mi++ ) {
        g.debugTeleport( mi, 300, 300 );
        const GameMap& gm = g.map();
        uint64_t worstCalls = 0, worstPix = 0; int wx = 0, wy = 0;
        for ( int y = 160; y < gm.mapHeight() * 32; y += 96 ) for ( int x = 160; x < gm.mapWidth() * 32; x += 96 ) {
            g.debugSetCamera( kLogicCx - x, kPlayerHitCy - y );
            r.resetStats(); r.fb.assign( r.fb.size(), 0 ); g.render();
            if ( r.imageCalls > worstCalls ) { worstCalls = r.imageCalls; worstPix = r.imagePixels + r.fillPixels; wx = x; wy = y; }
        }
        printf( "  %-9s : %4llu appels, %6llu pixels parcourus (en (%d,%d)) -> %.1fx l'ecran\n", w.mapNames[mi].c_str(),
                (unsigned long long)worstCalls, (unsigned long long)worstPix, wx, wy, worstPix / (double)( kScreenW * kScreenH ) );
    }

    printf( "[affichage de diagnostic (bouton D)]\n" );
    { InputState dd; dd.actionD = true; tick( dd ); tick( InputState() ); }
    CHECK( g.debugVisible(), "D affiche le diagnostic" );
    g.setDebugProvider( []( char* b, size_t n ) { snprintf( b, n, "H123k P4567k" ); } );
    g.debugTeleport( mapIdx( "maison" ), 306, 160 ); tick( InputState() );
    shot( "17_diagnostic" );
    { InputState dd; dd.actionD = true; tick( dd ); tick( InputState() ); }
    CHECK( !g.debugVisible(), "D le masque" );

    printf( "[boucle a pas fixe : un appui = un seul front montant, quelle que soit la cadence du rendu]\n" );
    {
        struct Hw : IInput {                                        // "materiel" : l'etat de la touche A a l'instant du tick
            bool hw = false; InputState cur, prev;
            void poll() override { prev = cur; cur.actionA = hw; }
            const InputState& state() const override { return cur; }
            bool justPressed( bool InputState::*b ) const override { return cur.*b && !( prev.*b ); }
        };
        auto simulate = [&]( int maxDt, bool pollPerTick, int& fronts ) {
            Hw h; FixedStep fs( 33 ); fs.reset( 0 ); fronts = 0;
            uint32_t rng = 12345, t = 0;
            auto rnd = [&]() { rng = rng * 1664525u + 1013904223u; return rng >> 16; };
            std::vector<std::pair<uint32_t,uint32_t>> presses;      // 40 appuis de 100 a 200 ms
            for ( uint32_t st = 200, i = 0; i < 40; i++ ) { uint32_t len = 100 + rnd() % 100; presses.push_back( { st, st + len } ); st += len + 150 + rnd() % 200; }
            uint32_t end = presses.back().second + 500;
            while ( t < end ) {
                t += 1 + rnd() % maxDt;                              // duree d'un tour de boucle (rendu inclus)
                h.hw = false; for ( auto& p : presses ) if ( t >= p.first && t < p.second ) h.hw = true;
                if ( pollPerTick ) fs.advance( t, [&]() { h.poll(); if ( h.justPressed( &InputState::actionA ) ) fronts++; } );
                else { h.poll(); fs.advance( t, [&]() { if ( h.justPressed( &InputState::actionA ) ) fronts++; } ); }   // ancienne boucle (defectueuse)
            }
            return (int)presses.size();
        };
        int f = 0, expected = 0;
        for ( int maxDt : { 5, 33, 70 } ) {
            expected = simulate( maxDt, true, f );
            char msg[96]; snprintf( msg, sizeof msg, "lecture a chaque tick, tours de 1 a %d ms : %d appuis -> %d fronts", maxDt, expected, f );
            CHECK( f == expected, msg );
        }
        int bad = 0; simulate( 5, false, bad ); int bad2 = 0; simulate( 70, false, bad2 );
        printf( "    (temoin : l'ancienne lecture une fois par tour donne %d fronts a 5 ms et %d a 70 ms au lieu de %d)\n", bad, bad2, expected );
        CHECK( bad != expected || bad2 != expected, "le test detecte bien l'ancienne boucle defectueuse" );
    }

    printf( failures ? "\n%d ECHEC(S)\n" : "\nTOUT EST OK\n", failures );
    return failures ? 1 : 0;
}
