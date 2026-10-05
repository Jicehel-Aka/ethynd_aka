#include "Game.h"
#include <cstring>

static Music musicFor( const char* map )
{
    if ( !strcmp( map, "maison" ) ) return Music::Maison;
    if ( !strcmp( map, "aventure" ) ) return Music::Aventure;
    if ( !strcmp( map, "grotte" ) ) return Music::Grotte;
    return Music::None;
}

bool Game::init()
{
    if ( !assets.load( source, renderer ) ) return false;
    playerChar.resolve( &kDef_joueur, assets );
    for ( int i = 0; i < kEntityDefCount && i < 8; i++ ) entityChars[i].resolve( kEntityDefs[i], assets );
    gameRandomSeed( 0x2545F491u );
    current = Screen::Menu;
    audio.playMusic( Music::Menu );
    audio.playSfx( Sfx::MenuSelect );
    return true;
}

// chargement() + Map(...) + charger_monstres() d'Ethynd
bool Game::enterMap( const char* name, int camX, int camY )
{
    renderer.drawImage( 0, 0, assets.menuImage( MENU_CHARGEMENT ) );   // ecran de chargement
    renderer.present();
    audio.playSfx( Sfx::MenuSelect );

    if ( !gmap.load( assets, source, name, camX, camY ) ) return false;
    audio.playMusic( musicFor( name ) );

    monsters.clear();
    for ( int l = 0; l < kLevelSpawnsCount; l++ ) {
        if ( strcmp( kLevelSpawns[l].map, name ) ) continue;
        for ( int i = 0; i < kLevelSpawns[l].count; i++ ) {
            const SpawnDef& s = kLevelSpawns[l].spawns[i];
            for ( int t = 0; t < kEntityDefCount; t++ ) {
                if ( strcmp( kEntityNames[t], s.type ) ) continue;
                Monster m;
                m.rc = &entityChars[t];
                m.wx = s.x; m.wy = s.y;
                m.vie = s.vie; m.attaque = s.attaque;
                monsters.push_back( m );
            }
        }
    }
    return true;
}

void Game::startGame()
{
    player.init( &playerChar );
    if ( enterMap( "maison", 14, 93 ) ) current = Screen::Playing;
}

void Game::teleport()
{
    const char* n = gmap.name();
    const int x = gmap.camX, y = gmap.camY;
    if ( !strcmp( n, "maison" ) ) {
        if ( x >= 110 && x <= 146 && y >= -249 && y <= -235 )      enterMap( "aventure", -464, -261 );
        else if ( x >= -19 && x <= 26 && y >= -180 && y <= -168 )  enterMap( "grotte", -416, -180 );
    } else if ( !strcmp( n, "grotte" ) ) {
        if ( x >= 133 && x <= 220 && y >= -610 && y <= -600 )      enterMap( "aventure", -1103, -460 );   // sortie de la grotte
        else if ( x >= -434 && x <= -398 && y >= -138 && y <= -111 ) enterMap( "maison", 0, -125 );
    } else if ( !strcmp( n, "aventure" ) ) {
        if ( x >= -467 && x <= -461 && y >= -255 && y <= -237 )    enterMap( "maison", 128, -234 );
        else if ( x >= -1106 && x <= -1100 && y >= -451 && y <= -430 ) enterMap( "grotte", 175, -591 );
    }
}

void Game::updatePlaying( const IInput& input )
{
    // Meme ordre que boucle_de_jeu() : touches, tuiles animees, monstres, joueur, teleportation.
    player.readKeys( input.state(), gmap );
    gmap.update();

    // gerer_monstres() : l'epee de l'attaque PRECEDENTE enleve 1 de vie par tick de contact
    for ( Monster& m : monsters ) {
        if ( !m.alive() ) continue;
        if ( player.swordActive() && rectsOverlap( player.sword(), Rect{ m.wx + gmap.camX, m.wy + gmap.camY, 32, 32 } ) ) {
            m.vie -= 1;
            audio.playSfx( Sfx::MonsterHit );
        }
        monsterUpdate( m, gmap );
    }

    player.update( gmap, monsters, audio );
    teleport();
    if ( player.vie < 1 ) {
        audio.playMusic( Music::None );
        current = Screen::Dead;
    }
}

void Game::update( const IInput& input )
{
    switch ( current ) {
        case Screen::Menu:
            if ( input.justPressed( &InputState::actionA ) ) startGame();
            else if ( input.justPressed( &InputState::actionC ) ) { audio.playSfx( Sfx::MenuSelect ); current = Screen::Help; }
            break;
        case Screen::Help:
            if ( input.justPressed( &InputState::actionB ) ) { audio.playSfx( Sfx::MenuSelect ); current = Screen::Menu; }
            break;
        case Screen::Playing:
            updatePlaying( input );
            break;
        case Screen::Dead:
            if ( input.justPressed( &InputState::actionA ) || input.justPressed( &InputState::actionB ) ) {
                audio.playMusic( Music::Menu );
                audio.playSfx( Sfx::MenuSelect );
                current = Screen::Menu;
            }
            break;
    }
}

void Game::drawHud()
{
    // interface() : barre rouge de 20 px (logiques) par point de vie, 10 px de haut, + "Vie"
    int w = player.vie > 0 ? player.vie * 20 * kTileShow / kTileLogic : 0;
    renderer.fillRect( 0, 0, w, 8, RGBColor{ 105, 0, 0 } );
    renderer.drawText( 1, 0, "Vie", RGBColor{ 255, 255, 255 }, FontSize::Narrow );
}

// ---------------------------------------------------------------- textes de menu
struct UiText { const char *playCap, *helpCap, *helpAttack, *helpBack, *helpMenuBack, *deadHint; };
static const UiText kUi[2] = {
    { "(bouton A)", "(bouton C)", "A : attaquer", "B : retour", "B : retour au menu", "A : retour au menu" },
    { "(button A)", "(button C)", "A: attack",    "B: back",    "B: back to menu",    "A: back to menu" },
};
static const RGBColor kTextColor = { 148, 191, 167 };      // vert des textes d'origine

void Game::setLanguage( const char* code ) { lang = ( code && code[0] == 'e' && code[1] == 'n' ) ? 1 : 0; }
void Game::setQuitHint( const char* text ) { strncpy( quitHint, text ? text : "", sizeof quitHint - 1 ); }

void Game::drawCentered( int cx, int y, const char* text )
{
    renderer.drawText( cx - (int)strlen( text ) * 3, y, text, kTextColor, FontSize::Narrow );   // 6 px par caractere
}

void Game::render()
{
    switch ( current ) {
        case Screen::Menu:
            renderer.drawImage( 0, 0, assets.menuImage( MENU_MENU ) );
            drawCentered( 84,  155, kUi[lang].playCap );          // sous "Jouer"
            drawCentered( 235, 155, kUi[lang].helpCap );          // sous "Aide"
            if ( quitHint[0] ) drawCentered( 160, 214, quitHint );
            break;
        case Screen::Help:
            renderer.drawImage( 0, 0, assets.menuImage( MENU_AIDE ) );
            renderer.drawText( 32, 158, kUi[lang].helpAttack, kTextColor, FontSize::Narrow );
            renderer.drawText( 32, 176, kUi[lang].helpBack, kTextColor, FontSize::Narrow );
            if ( quitHint[0] ) renderer.drawText( 32, 194, quitHint, kTextColor, FontSize::Narrow );
            drawCentered( 160, 230, kUi[lang].helpMenuBack );
            break;
        case Screen::Dead:
            renderer.drawImage( 0, 0, assets.menuImage( MENU_MORT ) );
            drawCentered( 160, 203, kUi[lang].deadHint );
            break;
        case Screen::Playing:
            gmap.drawBackground( renderer );
            for ( const Monster& m : monsters ) monsterDraw( m, gmap, renderer );
            player.draw( renderer );
            gmap.drawForeground( renderer );
            drawHud();
            break;
    }
}
