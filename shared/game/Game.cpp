#include "Game.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

// ---------------------------------------------------------------- utilitaires UTF-8
static int utf8Len( const char* s )
{
    int n = 0;
    for ( ; *s; ++s ) if ( ( (uint8_t)*s & 0xC0 ) != 0x80 ) n++;
    return n;
}

// offset (en octets) du n-ieme caractere
static size_t utf8Offset( const std::string& s, int n )
{
    size_t i = 0;
    while ( i < s.size() && n > 0 ) {
        i++;
        while ( i < s.size() && ( (uint8_t)s[i] & 0xC0 ) == 0x80 ) i++;
        n--;
    }
    return i;
}

// coupe un texte en lignes de `cols` caracteres au plus (aux espaces ; mot trop long : coupe franche)
static void wrapText( const std::string& text, int cols, std::vector<std::string>& out )
{
    out.clear();
    std::string line, word;
    auto flushWord = [&]() {
        if ( word.empty() ) return;
        int wl = utf8Len( word.c_str() ), ll = utf8Len( line.c_str() );
        if ( ll > 0 && ll + 1 + wl > cols ) { out.push_back( line ); line.clear(); }
        while ( wl > cols ) {                              // mot plus long qu'une ligne
            size_t cut = utf8Offset( word, cols );
            if ( !line.empty() ) { out.push_back( line ); line.clear(); }
            out.push_back( word.substr( 0, cut ) );
            word = word.substr( cut );
            wl = utf8Len( word.c_str() );
        }
        if ( !line.empty() ) line += ' ';
        line += word;
        word.clear();
    };
    for ( char c : text ) {
        if ( c == ' ' ) flushWord(); else word += c;
    }
    flushWord();
    if ( !line.empty() ) out.push_back( line );
}

static const RGBColor kText   = { 148, 191, 167 };       // vert des textes d'origine
static const RGBColor kWhite  = { 255, 255, 255 };
static const RGBColor kYellow = { 255, 224, 130 };
static const RGBColor kBoxBg  = { 20, 22, 40 };

static const int kCharW = 8;                              // police large : 8x8, avance 8
static const int kBoxCols = 38;                           // 38 * 8 = 304 px de texte dans la boite
static const int kBoxRows = 4;

// ---------------------------------------------------------------- texte
int Game::textWidth( const char* s, int scale ) const { return utf8Len( s ) * kCharW * scale; }

void Game::textAt( int x, int y, const char* s, RGBColor c, int scale )
{
    renderer.drawText( (int16_t)x, (int16_t)y, s, c, FontSize::Wide, scale );
}

void Game::textCentered( int cx, int y, const char* s, RGBColor c, int scale, int maxW )
{
    while ( scale > 1 && textWidth( s, scale ) > maxW ) scale--;
    textAt( cx - textWidth( s, scale ) / 2, y, s, c, scale );
}

int Game::textWrapped( int x, int y, int cols, const char* s, RGBColor c )
{
    std::vector<std::string> rows;
    wrapText( s, cols, rows );
    for ( size_t i = 0; i < rows.size(); i++ ) textAt( x, y + (int)i * 10, rows[i].c_str(), c );
    return (int)rows.size();
}

// ---------------------------------------------------------------- initialisation
void Game::setLanguage( const char* code )
{
    langCode = ( code && *code ) ? code : Lang::kDefault;
    if ( ready ) lang.load( source, langCode.c_str() );
}

bool Game::init()
{
    if ( !assets.load( source, renderer ) ) return false;
    if ( !world.load( source ) ) return false;
    if ( !lang.load( source, langCode.c_str() ) ) return false;

    playerChar.resolve( &kDef_joueur, assets );
    for ( int i = 0; i < kEntityDefCount && i < 8; i++ ) entityChars[i].resolve( kEntityDefs[i], assets );
    for ( int i = 0; i < kNpcDefCount && i < 4; i++ ) npcChars[i].resolve( kNpcDefs[i], assets );
    itemIcons.clear();
    for ( const ItemDef& it : world.items ) itemIcons.push_back( assets.sprite( it.icon.c_str() ) );

    gameRandomSeed( 0x2545F491u );
    ready = true;
    current = Screen::Menu;
    audio.playMusic( Music::Menu );
    audio.playSfx( Sfx::MenuSelect );
    return true;
}

// chargement() + Map(...) + charger_monstres() d'Ethynd, puis PNJ et objets
bool Game::enterMap( int idx, int camX, int camY )
{
    if ( idx < 0 || idx >= (int)world.mapNames.size() ) return false;
    renderer.drawImage( 0, 0, assets.menuImage( MENU_CHARGEMENT ) );
    textCentered( kScreenW / 2, 112, lang.get( "ui.loading" ), kText, 2, 300 );
    renderer.present();
    audio.playSfx( Sfx::MenuSelect );

    if ( !gmap.load( assets, source, world.mapNames[idx].c_str(), camX, camY ) ) return false;
    mapIdx = idx;
    const MapObjects& ob = gmap.objects();
    audio.playMusic( (Music)( ob.music <= 4 ? ob.music : 0 ) );

    monsters.clear();
    for ( const SpawnObj& s : ob.spawns ) {
        for ( int t = 0; t < kEntityDefCount && t < 8; t++ ) {
            if ( s.type != kEntityNames[t] ) continue;
            Monster m;
            m.rc = &entityChars[t]; m.typeIdx = t;
            m.wx = s.x; m.wy = s.y; m.vie = s.vie; m.attaque = s.attaque;
            monsters.push_back( m );
        }
    }
    npcs.clear();
    blockers.clear();
    for ( const NpcObj& o : ob.npcs ) {
        for ( int t = 0; t < kNpcDefCount && t < 4; t++ ) {
            if ( o.type != kNpcNames[t] ) continue;
            Npc n;
            n.rc = &npcChars[t]; n.wx = o.x; n.wy = o.y; n.dir = o.dir; n.rules = o.rules;
            n.sprite = staticSprite( n.rc, n.dir );
            npcs.push_back( n );
            blockers.push_back( n.rect() );
        }
    }
    if ( taken.size() < world.mapNames.size() ) taken.resize( world.mapNames.size() );
    taken[idx].resize( ob.items.size(), 0 );
    items.clear();
    for ( size_t i = 0; i < ob.items.size(); i++ ) {
        MapItem m; m.def = ob.items[i].def; m.wx = ob.items[i].x; m.wy = ob.items[i].y; m.taken = taken[idx][i] != 0;
        items.push_back( m );
    }
    doorsArmed = false;                                  // pas de rebond : la porte d'arrivee est inactive tant qu'on est dessus
    return true;
}

void Game::startGame()
{
    qs = QuestState();
    taken.clear();
    toastQueue.clear(); toast.clear(); toastTicks = 0;
    winPending = false;
    player.init( &playerChar );
    player.face( world.startDir );
    // la position de depart est le centre de la hitbox du joueur (monde) : camera = ecran - monde
    if ( enterMap( world.startMap, kLogicCx - world.startX, kPlayerHitCy - world.startY ) ) current = Screen::Playing;
}

// ---------------------------------------------------------------- quetes
bool Game::objectiveMet( const Objective& o ) const
{
    switch ( o.cond ) {
        case COND_FLAG: return qs.flag( o.p[0] );
        case COND_KILL:
            for ( int t = 0; t < kEntityDefCount && t < kMaxKillTypes; t++ )
                if ( o.s == kEntityNames[t] ) return qs.kills[t] >= o.p[0];
            return false;
        case COND_ITEM: return o.p[0] >= 0 && o.p[0] < kMaxItems && qs.items[o.p[0]] >= o.p[1];
        case COND_MAP:  return mapIdx == o.p[0];
        case COND_REACH: {
            if ( mapIdx != o.p[0] ) return false;
            Rect p = playerWorld();
            int dx = p.x + p.w / 2 - o.p[1], dy = p.y + p.h / 2 - o.p[2];
            return dx * dx + dy * dy <= o.p[3] * o.p[3];
        }
    }
    return false;
}

void Game::updateObjectives()
{
    bool changed;
    do {
        changed = false;
        for ( size_t i = 0; i < world.objectives.size() && i < (size_t)kMaxObjectives; i++ ) {
            const Objective& o = world.objectives[i];
            if ( qs.done( (int)i ) ) continue;
            if ( o.after >= 0 && !qs.done( o.after ) ) continue;       // pas encore visible
            if ( !objectiveMet( o ) ) continue;
            qs.objDone |= 1u << i;
            pushToast( std::string( lang.get( "ui.toast.done" ) ) + " " + lang.get( o.title ) );
            audio.playSfx( Sfx::MenuSelect );
            changed = true;
        }
    } while ( changed );
}

int Game::currentObjective() const
{
    for ( size_t i = 0; i < world.objectives.size() && i < (size_t)kMaxObjectives; i++ ) {
        if ( qs.done( (int)i ) ) continue;
        const Objective& o = world.objectives[i];
        if ( o.after >= 0 && !qs.done( o.after ) ) continue;
        return (int)i;
    }
    return -1;
}

std::string Game::objectiveLabel( int i ) const
{
    const Objective& o = world.objectives[i];
    std::string s = lang.get( o.title );
    int cur = -1, tot = 0;
    if ( o.cond == COND_KILL ) {
        for ( int t = 0; t < kEntityDefCount && t < kMaxKillTypes; t++ ) if ( o.s == kEntityNames[t] ) cur = qs.kills[t];
        tot = o.p[0];
    } else if ( o.cond == COND_ITEM && o.p[0] >= 0 && o.p[0] < kMaxItems ) {
        cur = qs.items[o.p[0]]; tot = o.p[1];
    }
    if ( cur >= 0 && tot > 1 ) {
        char b[24];
        snprintf( b, sizeof b, " (%d/%d)", cur > tot ? tot : cur, tot );
        s += b;
    }
    return s;
}

// ---------------------------------------------------------------- dialogues
int Game::chooseDialogue( const Npc& n ) const
{
    for ( const TalkRule& r : n.rules ) {
        bool ok = false;
        switch ( r.kind ) {
            case RULE_DEFAULT:  ok = true; break;
            case RULE_FLAG:     ok = qs.flag( r.arg ); break;
            case RULE_NOT_FLAG: ok = !qs.flag( r.arg ); break;
            case RULE_HAS_ITEM: ok = r.arg >= 0 && r.arg < kMaxItems && qs.items[r.arg] > 0; break;
        }
        if ( ok ) return r.dialogue;
    }
    return -1;
}

const Npc* Game::npcInReach() const
{
    Rect p = playerWorld();
    switch ( player.dir() ) {                            // zone de contact devant le joueur
        case DIR_BAS:    p.h += 18; break;
        case DIR_HAUT:   p.y -= 18; p.h += 18; break;
        case DIR_GAUCHE: p.x -= 18; p.w += 18; break;
        case DIR_DROITE: p.w += 18; break;
    }
    for ( const Npc& n : npcs ) if ( rectsOverlap( p, n.rect() ) ) return &n;
    return nullptr;
}

void Game::startDialogueLine()
{
    const Dialogue& d = world.dialogues[dlg];
    wrapText( lang.get( d.lines[dlgLine].text ), kBoxCols, dlgRows );
    dlgPage = 0;
    dlgShown = 0;
}

int Game::dialoguePageChars() const
{
    int n = 0;
    for ( int r = dlgPage * kBoxRows; r < (int)dlgRows.size() && r < ( dlgPage + 1 ) * kBoxRows; r++ )
        n += utf8Len( dlgRows[r].c_str() );
    return n;
}

void Game::startDialogue( int index )
{
    if ( index < 0 || index >= (int)world.dialogues.size() || world.dialogues[index].lines.empty() ) return;
    dlg = index;
    dlgLine = 0;
    startDialogueLine();
    player.idle();
    audio.playSfx( Sfx::MenuSelect );
    current = Screen::Dialogue;
}

void Game::finishDialogue()
{
    const Dialogue& d = world.dialogues[dlg];
    for ( const DlgAction& a : d.actions ) {
        switch ( a.type ) {
            case ACT_SET:  qs.setFlag( a.a ); break;
            case ACT_GIVE: if ( a.a >= 0 && a.a < kMaxItems ) qs.items[a.a] = (uint8_t)std::min( 99, qs.items[a.a] + a.b ); break;
            case ACT_TAKE: if ( a.a >= 0 && a.a < kMaxItems ) qs.items[a.a] = (uint8_t)std::max( 0, qs.items[a.a] - a.b ); break;
            case ACT_HEAL: player.heal( a.a ); break;
            case ACT_WIN:  winPending = true; break;
        }
    }
    dlg = -1;
    updateObjectives();
    if ( winPending ) {
        winPending = false;
        audio.playMusic( Music::None );
        current = Screen::Win;
    } else {
        current = Screen::Playing;
    }
}

void Game::updateDialogue( const IInput& input )
{
    int total = dialoguePageChars();
    bool a = input.justPressed( &InputState::actionA );
    if ( dlgShown < total ) {
        dlgShown = a ? total : std::min( total, dlgShown + 2 );     // machine a ecrire ; A = tout afficher
        return;
    }
    if ( !a ) return;
    audio.playSfx( Sfx::MenuSelect );
    if ( ( dlgPage + 1 ) * kBoxRows < (int)dlgRows.size() ) { dlgPage++; dlgShown = 0; return; }
    if ( dlgLine + 1 < (int)world.dialogues[dlg].lines.size() ) { dlgLine++; startDialogueLine(); return; }
    finishDialogue();
}

// ---------------------------------------------------------------- portes et objets
void Game::checkDoors()
{
    Rect p = playerWorld();
    int cx = p.x + p.w / 2, cy = p.y + p.h / 2;           // centre de la hitbox : repere des portes
    const DoorObj* hit = nullptr;
    for ( const DoorObj& d : gmap.objects().doors )
        if ( cx >= d.r.x && cx < d.r.x + d.r.w && cy >= d.r.y && cy < d.r.y + d.r.h ) { hit = &d; break; }
    if ( !hit ) { doorsArmed = true; return; }             // hors de toute porte : elles sont actives
    if ( !doorsArmed ) return;                             // encore sur la porte d'arrivee (ou refusee)
    doorsArmed = false;
    if ( hit->needFlag >= 0 && !qs.flag( hit->needFlag ) ) { startDialogue( hit->deny ); return; }
    enterMap( hit->destMap, kLogicCx - hit->dx, kPlayerHitCy - hit->dy );
}

void Game::checkItems()
{
    Rect p = playerWorld();
    for ( size_t i = 0; i < items.size(); i++ ) {
        MapItem& m = items[i];
        if ( m.taken || m.def < 0 || m.def >= (int)world.items.size() ) continue;
        if ( !rectsOverlap( p, m.rect() ) ) continue;
        const ItemDef& def = world.items[m.def];
        if ( def.heal > 0 ) {
            if ( player.vie >= kPlayerLife ) continue;   // pleine vie : la potion reste la pour plus tard
            player.heal( def.heal );
        } else if ( m.def < kMaxItems ) {
            qs.items[m.def] = (uint8_t)std::min( 99, qs.items[m.def] + 1 );
        }
        m.taken = true;
        taken[mapIdx][i] = 1;
        pushToast( std::string( lang.get( "ui.toast.item" ) ) + " " + lang.get( def.name ) );
        audio.playSfx( Sfx::MenuSelect );
    }
}

// ---------------------------------------------------------------- boucle
void Game::updatePlaying( const IInput& input )
{
    if ( input.justPressed( &InputState::actionB ) ) { audio.playSfx( Sfx::MenuSelect ); current = Screen::Log; player.idle(); return; }
    if ( input.justPressed( &InputState::actionA ) ) {
        if ( const Npc* n = npcInReach() ) {             // A devant un PNJ : parler, pas attaquer
            startDialogue( chooseDialogue( *n ) );
            if ( current == Screen::Dialogue ) return;
        }
    }

    // Meme ordre que boucle_de_jeu() : touches, tuiles animees, monstres, joueur.
    player.readKeys( input.state(), gmap, blockers );
    gmap.update();

    // gerer_monstres() : l'epee de l'attaque PRECEDENTE enleve 1 de vie par tick de contact
    for ( Monster& m : monsters ) {
        if ( !m.alive() ) continue;
        if ( player.swordActive() && rectsOverlap( player.sword(), Rect{ m.wx + gmap.camX, m.wy + gmap.camY, 32, 32 } ) ) {
            m.vie -= 1;
            audio.playSfx( Sfx::MonsterHit );
            if ( m.vie <= 0 && m.typeIdx >= 0 && m.typeIdx < kMaxKillTypes ) qs.kills[m.typeIdx]++;
        }
        monsterUpdate( m, gmap );
    }

    player.update( gmap, monsters, audio );
    checkItems();
    checkDoors();
    updateObjectives();
    if ( player.vie < 1 ) {
        audio.playMusic( Music::None );
        current = Screen::Dead;
    }
}

void Game::update( const IInput& input )
{
    if ( toastTicks > 0 ) toastTicks--;
    else if ( !toastQueue.empty() ) { toast = toastQueue.front(); toastQueue.erase( toastQueue.begin() ); toastTicks = 75; }

    switch ( current ) {
        case Screen::Menu:
            if ( input.justPressed( &InputState::actionA ) ) startGame();
            else if ( input.justPressed( &InputState::actionC ) ) { audio.playSfx( Sfx::MenuSelect ); current = Screen::Help; }
            break;
        case Screen::Help:
            if ( input.justPressed( &InputState::actionB ) ) { audio.playSfx( Sfx::MenuSelect ); current = Screen::Menu; }
            break;
        case Screen::Playing:  updatePlaying( input ); break;
        case Screen::Dialogue: updateDialogue( input ); break;
        case Screen::Log:
            if ( input.justPressed( &InputState::actionB ) || input.justPressed( &InputState::actionA ) ) {
                audio.playSfx( Sfx::MenuSelect ); current = Screen::Playing;
            }
            break;
        case Screen::Dead:
        case Screen::Win:
            if ( input.justPressed( &InputState::actionA ) || input.justPressed( &InputState::actionB ) ) {
                audio.playMusic( Music::Menu );
                audio.playSfx( Sfx::MenuSelect );
                current = Screen::Menu;
            }
            break;
    }
}

// ---------------------------------------------------------------- rendu
void Game::drawHud()
{
    // interface() d'Ethynd : barre rouge de 20 px (logiques) par point de vie, 10 px de haut
    int w = player.vie > 0 ? player.vie * 20 * kTileShow / kTileLogic : 0;
    renderer.fillRect( 0, 0, (int16_t)w, 8, RGBColor{ 105, 0, 0 } );
    textAt( 1, 0, lang.get( "ui.hud.life" ), kWhite );

    int o = currentObjective();
    if ( o >= 0 ) {                                      // objectif en cours, une ligne
        std::string s = "> " + objectiveLabel( o );
        const int maxCols = kScreenW / kCharW;
        if ( utf8Len( s.c_str() ) > maxCols ) s = s.substr( 0, utf8Offset( s, maxCols - 3 ) ) + "...";
        renderer.fillRect( 0, 9, (int16_t)( utf8Len( s.c_str() ) * kCharW + 2 ), 10, RGBColor{ 15, 15, 25 } );
        textAt( 1, 10, s.c_str(), kYellow );
    }
    if ( toastTicks > 0 && !toast.empty() ) {
        std::string s = toast;
        const int maxCols = kScreenW / kCharW - 1;
        if ( utf8Len( s.c_str() ) > maxCols ) s = s.substr( 0, utf8Offset( s, maxCols - 3 ) ) + "...";
        int w2 = utf8Len( s.c_str() ) * kCharW + 8;
        renderer.fillRect( (int16_t)( ( kScreenW - w2 ) / 2 ), 24, (int16_t)w2, 14, RGBColor{ 30, 60, 40 } );
        textAt( ( kScreenW - w2 ) / 2 + 4, 27, s.c_str(), kWhite );
    }
}

void Game::drawDialogue()
{
    const int bx = 4, by = 158, bw = kScreenW - 8, bh = 78;
    renderer.fillRect( (int16_t)( bx - 1 ), (int16_t)( by - 1 ), (int16_t)( bw + 2 ), (int16_t)( bh + 2 ), kText );
    renderer.fillRect( bx, by, bw, bh, kBoxBg );
    const DlgLine& ln = world.dialogues[dlg].lines[dlgLine];
    if ( !ln.speaker.empty() ) textAt( bx + 6, by + 4, lang.get( ln.speaker ), kYellow );
    int left = dlgShown;
    for ( int r = 0; r < kBoxRows; r++ ) {
        int idx = dlgPage * kBoxRows + r;
        if ( idx >= (int)dlgRows.size() || left <= 0 ) break;
        const std::string& row = dlgRows[idx];
        int n = std::min( left, utf8Len( row.c_str() ) );
        left -= n;
        std::string part = row.substr( 0, utf8Offset( row, n ) );
        textAt( bx + 6, by + 16 + r * 12, part.c_str(), kWhite );
    }
    if ( dlgShown >= dialoguePageChars() ) {             // invite clignotante : encore du texte ou fin
        static int blink = 0;
        if ( ( ++blink / 10 ) & 1 ) textAt( bx + bw - 12, by + bh - 11, "v", kYellow );
    }
}

void Game::drawLog()
{
    renderer.fillRect( 0, 0, kScreenW, kScreenH, RGBColor{ 20, 22, 30 } );
    textCentered( kScreenW / 2, 6, lang.get( "ui.log.title" ), kText, 2, 300 );
    int y = 30;
    for ( size_t i = 0; i < world.objectives.size() && i < (size_t)kMaxObjectives; i++ ) {
        const Objective& o = world.objectives[i];
        if ( o.after >= 0 && !qs.done( o.after ) ) continue;             // pas encore decouvert
        bool d = qs.done( (int)i );
        std::string s = objectiveLabel( (int)i );
        textAt( 6, y, d ? "[x]" : "[ ]", d ? kText : kYellow );
        int rows = textWrapped( 36, y, ( kScreenW - 42 ) / kCharW, s.c_str(), d ? kText : kWhite );
        y += rows * 10 + 4;
    }
    y = std::max( y + 6, 140 );
    textAt( 6, y, lang.get( "ui.log.items" ), kText ); y += 12;
    bool any = false;
    for ( size_t i = 0; i < world.items.size() && i < (size_t)kMaxItems; i++ ) {
        if ( qs.items[i] == 0 ) continue;
        any = true;
        if ( i < itemIcons.size() && itemIcons[i] != kInvalidImageId ) renderer.drawImage( 8, y - 4, itemIcons[i] );
        char b[16]; snprintf( b, sizeof b, " x%d", qs.items[i] );
        std::string s = std::string( lang.get( world.items[i].name ) ) + b;
        textAt( 30, y, s.c_str(), kWhite );
        y += 18;
    }
    if ( !any ) textAt( 12, y, lang.get( "ui.log.none" ), kWhite );
    textCentered( kScreenW / 2, 228, lang.get( "ui.log.close" ), kText, 1, 300 );
}

void Game::drawMenuScreens()
{
    switch ( current ) {
        case Screen::Menu:
            renderer.drawImage( 0, 0, assets.menuImage( MENU_MENU ) );
            textCentered( 160, 86, lang.get( "ui.menu.subtitle" ), kText, 1, 300 );
            textCentered( 84, 130, lang.get( "ui.menu.play" ), kText, 2, 108 );
            textCentered( 235, 130, lang.get( "ui.menu.help" ), kText, 2, 108 );
            textCentered( 84, 152, lang.get( "ui.cap.a" ), kText, 1, 112 );
            textCentered( 235, 152, lang.get( "ui.cap.c" ), kText, 1, 112 );
            break;
        case Screen::Help: {
            renderer.drawImage( 0, 0, assets.menuImage( MENU_AIDE ) );
            textCentered( 160, 18, lang.get( "ui.help.title" ), kText, 3, 250 );
            textAt( 126, 98, lang.get( "ui.help.move" ), kText );
            int y = 148;
            y += textWrapped( 30, y, 18, lang.get( "ui.help.attack" ), kText ) * 10 + 4;
            y += textWrapped( 30, y, 18, lang.get( "ui.help.log" ), kText ) * 10 + 4;
            if ( !quitKey.empty() ) textWrapped( 30, y, 18, lang.get( quitKey ), kText );
            textWrapped( 190, 138, 13, lang.get( "ui.help.credits" ), kText );
            textCentered( 160, 230, lang.get( "ui.help.back_menu" ), kText, 1, 310 );
            break;
        }
        case Screen::Dead:
            renderer.drawImage( 0, 0, assets.menuImage( MENU_MORT ) );
            textCentered( 160, 100, lang.get( "ui.dead.title" ), kText, 3, 300 );
            textCentered( 160, 146, lang.get( "ui.dead.sub" ), kText, 1, 300 );
            textCentered( 160, 204, lang.get( "ui.dead.hint" ), kText, 1, 300 );
            break;
        case Screen::Win:
            renderer.drawImage( 0, 0, assets.menuImage( MENU_FIN ) );
            textCentered( 160, 104, lang.get( "ui.win.title" ), kText, 2, 300 );
            textCentered( 160, 146, lang.get( "ui.win.sub" ), kText, 1, 300 );
            textCentered( 160, 204, lang.get( "ui.win.hint" ), kText, 1, 300 );
            break;
        default: break;
    }
}

void Game::render()
{
    switch ( current ) {
        case Screen::Menu: case Screen::Help: case Screen::Dead: case Screen::Win:
            drawMenuScreens();
            break;
        case Screen::Log:
            drawLog();
            break;
        case Screen::Playing:
        case Screen::Dialogue: {
            gmap.drawBackground( renderer );
            const int ox = gmap.originX(), oy = gmap.originY();
            auto sx = [&]( int wx ) { return ox + floorDiv( wx * kTileShow, kTileLogic ); };
            auto sy = [&]( int wy ) { return oy + floorDiv( wy * kTileShow, kTileLogic ); };
            for ( const MapItem& m : items ) {
                if ( m.taken || m.def < 0 || m.def >= (int)itemIcons.size() ) continue;
                renderer.drawImage( sx( m.wx ), sy( m.wy ), itemIcons[m.def] );
            }
            for ( const Npc& n : npcs ) renderer.drawImage( sx( n.wx ), sy( n.wy ), n.sprite );
            for ( const Monster& m : monsters ) monsterDraw( m, gmap, renderer );
            player.draw( renderer );
            gmap.drawForeground( renderer );
            if ( current == Screen::Playing ) {
                if ( const Npc* n = npcInReach() ) {      // bulle "A" au-dessus du PNJ qui peut parler
                    int x = sx( n->wx ) + 8, y = sy( n->wy ) - 12;
                    renderer.fillRect( (int16_t)x, (int16_t)y, 10, 11, kText );
                    textAt( x + 1, y + 1, "A", RGBColor{ 20, 22, 40 } );
                }
            }
            drawHud();
            if ( current == Screen::Dialogue ) drawDialogue();
            break;
        }
    }
}
