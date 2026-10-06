#include "EditorCore.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <sstream>
#include "Defs.h"
#include "EthyndData.h"            // kEntityNames / kNpcNames : types de monstres et de PNJ valides

static const RGBColor kBg     = { 28, 28, 34 };
static const RGBColor kPanelC = { 40, 42, 52 };
static const RGBColor kText   = { 210, 225, 215 };
static const RGBColor kDim    = { 130, 140, 135 };
static const RGBColor kYellow = { 255, 224, 130 };
static const RGBColor kRed    = { 230, 70, 70 };
static const RGBColor kCyan   = { 90, 210, 230 };
static const RGBColor kGreen  = { 110, 220, 140 };

static const char* const kMusics[] = { "none", "menu", "maison", "aventure", "grotte" };
static const char* const kDirs[]   = { "bas", "haut", "gauche", "droite" };

static bool contains( const std::vector<std::string>& v, const std::string& s ) { return std::find( v.begin(), v.end(), s ) != v.end(); }
static std::vector<std::string> tokens( const std::string& s ) { std::vector<std::string> t; std::istringstream is( s ); std::string x; while ( is >> x ) t.push_back( x ); return t; }
static bool isInt( const std::string& s ) { if ( s.empty() ) return false; size_t i = s[0] == '-' ? 1 : 0; if ( i >= s.size() ) return false; for ( ; i < s.size(); i++ ) if ( s[i] < '0' || s[i] > '9' ) return false; return true; }

// ------------------------------------------------------------------ textes
std::string EditorCore::tr( const std::string& key ) const
{
    auto it = L.find( key ); if ( it != L.end() ) return it->second;
    it = Lfr.find( key ); return it != Lfr.end() ? it->second : key;
}
std::string EditorCore::tr( const std::string& key, const std::string& a, const std::string& b ) const
{
    std::string s = tr( key );
    for ( int i = 0; i < 2; i++ ) {                      // {0} {1}
        std::string tag = "{" + std::to_string( i ) + "}";
        size_t p = s.find( tag );
        if ( p != std::string::npos ) s.replace( p, tag.size(), i == 0 ? a : b );
    }
    return s;
}

// ------------------------------------------------------------------ ouverture
bool EditorCore::open( const std::string& worldDir, const std::string& assets, const std::string& langCode, std::string& err )
{
    if ( !loadWorld( worldDir, world, err ) ) return false;
    if ( world.maps.empty() ) { err = "aucune carte dans world.txt"; return false; }
    assetsDir = assets;
    loadLangFile( worldDir + "/lang/fr.txt", Lfr );
    if ( langCode != "fr" ) loadLangFile( worldDir + "/lang/" + langCode + ".txt", L ); else L = Lfr;

    auto readAll = [&]( const std::string& path, std::vector<uint8_t>& out ) {
        FILE* f = fopen( path.c_str(), "rb" ); if ( !f ) return false;
        fseek( f, 0, SEEK_END ); long n = ftell( f ); fseek( f, 0, SEEK_SET );
        out.resize( (size_t)n ); bool ok = fread( out.data(), 1, (size_t)n, f ) == (size_t)n; fclose( f ); return ok;
    };
    if ( !readAll( assets + "/tileset_full.bin", tilesetBlob ) || tilesetBlob.size() < 10 || memcmp( tilesetBlob.data(), "ETSF", 4 ) ) {
        err = assets + "/tileset_full.bin introuvable (convert_assets.py --editor-out)"; return false;
    }
    uint16_t tp, cnt, cols; memcpy( &tp, &tilesetBlob[4], 2 ); memcpy( &cnt, &tilesetBlob[6], 2 ); memcpy( &cols, &tilesetBlob[8], 2 );
    if ( tp != kTile ) { err = "tileset_full.bin : tuiles de " + std::to_string( tp ) + " px (24 attendus)"; return false; }
    tilesetCount = cnt; tilesetCols = cols;
    // sprites (meme fichier que le jeu) : pour dessiner monstres, PNJ et objets
    if ( readAll( assets + "/sprites.bin", spritesBlob ) && spritesBlob.size() >= 8 && !memcmp( spritesBlob.data(), "ESPR", 4 ) ) {
        uint32_t n; memcpy( &n, &spritesBlob[4], 4 );
        for ( uint32_t i = 0; i < n && 8 + ( i + 1 ) * 32 <= spritesBlob.size(); i++ ) {
            char name[24] = {0}; memcpy( name, &spritesBlob[8 + i * 32], 23 );
            spriteNames.push_back( name );
        }
    }
    undoStack.assign( world.maps.size(), {} );
    redoStack.assign( world.maps.size(), {} );
    curMap = std::max( 0, world.mapIndex( world.startMap ) );
    selTile = 0;
    setStatus( tr( "ed.title" ) );
    return true;
}

void EditorCore::ensureImages( SoftRenderer& r )
{
    if ( imagesReady ) return;
    imagesReady = true;
    const size_t tb = (size_t)kTile * kTile * 2;
    for ( int i = 0; i < tilesetCount; i++ )
        tileImg.push_back( r.createImage( (const uint16_t*)( tilesetBlob.data() + 10 + (size_t)i * tb ), kTile, kTile ) );
    uint32_t n; memcpy( &n, &spritesBlob[4], 4 );
    size_t blob = 8 + (size_t)n * 32;
    for ( size_t i = 0; i < spriteNames.size(); i++ ) {
        const uint8_t* e = &spritesBlob[8 + i * 32];
        uint16_t w, h; uint32_t off; memcpy( &w, e + 24, 2 ); memcpy( &h, e + 26, 2 ); memcpy( &off, e + 28, 4 );
        spriteImg[spriteNames[i]] = r.createImage( (const uint16_t*)( spritesBlob.data() + blob + off ), w, h );
    }
}

ImageId EditorCore::spriteFor( const std::string& type ) const
{
    auto it = spriteImg.find( type + "_00" );
    if ( it != spriteImg.end() ) return it->second;
    if ( type == "joueur" ) { it = spriteImg.find( "personnage_00" ); if ( it != spriteImg.end() ) return it->second; }
    for ( const std::string& n : spriteNames ) if ( n.compare( 0, type.size() + 1, type + "_" ) == 0 ) return spriteImg.at( n );
    return kInvalidImageId;
}

// ------------------------------------------------------------------ annuler / retablir
void EditorCore::snapshot()
{
    auto& u = undoStack[curMap];
    u.push_back( cur() );
    if ( u.size() > 60 ) u.erase( u.begin() );
    redoStack[curMap].clear();
    dirty = true;
}

void EditorCore::doUndo( bool redo )
{
    auto& from = redo ? redoStack[curMap] : undoStack[curMap];
    auto& to   = redo ? undoStack[curMap] : redoStack[curMap];
    if ( from.empty() ) { setStatus( tr( "ed.status.nothing" ) ); return; }
    to.push_back( cur() );
    cur() = from.back();
    from.pop_back();
    selKind = EdSel::None; selIdx = -1;
    dirty = true;
    setStatus( tr( redo ? "ed.status.redo" : "ed.status.undo" ) );
}

// ------------------------------------------------------------------ peinture
void EditorCore::paintCell( int cx, int cy, int value )
{
    EdMap& m = cur();
    if ( cx < 0 || cy < 0 || cx >= m.w || cy >= m.h ) return;
    m.at( layer, cx, cy ) = value;
}

void EditorCore::paintLine( int x0, int y0, int x1, int y1, int value )     // Bresenham : pas de trou si la souris va vite
{
    int dx = std::abs( x1 - x0 ), dy = -std::abs( y1 - y0 ), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy;
    for ( ;; ) {
        paintCell( x0, y0, value );
        if ( x0 == x1 && y0 == y1 ) break;
        int e2 = 2 * err;
        if ( e2 >= dy ) { err += dy; x0 += sx; }
        if ( e2 <= dx ) { err += dx; y0 += sy; }
    }
}

void EditorCore::floodFill( int cx, int cy, int value )
{
    EdMap& m = cur();
    if ( cx < 0 || cy < 0 || cx >= m.w || cy >= m.h ) return;
    int target = m.at( layer, cx, cy );
    if ( target == value ) return;
    std::queue<std::pair<int,int>> q; q.push( { cx, cy } ); m.at( layer, cx, cy ) = value;
    while ( !q.empty() ) {
        auto [x, y] = q.front(); q.pop();
        const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
        for ( int k = 0; k < 4; k++ ) {
            int nx = x + dx[k], ny = y + dy[k];
            if ( nx < 0 || ny < 0 || nx >= m.w || ny >= m.h || m.at( layer, nx, ny ) != target ) continue;
            m.at( layer, nx, ny ) = value; q.push( { nx, ny } );
        }
    }
}

void EditorCore::fillRect( int x0, int y0, int x1, int y1, int value )
{
    if ( x0 > x1 ) std::swap( x0, x1 );
    if ( y0 > y1 ) std::swap( y0, y1 );
    for ( int y = y0; y <= y1; y++ ) for ( int x = x0; x <= x1; x++ ) paintCell( x, y, value );
}

void EditorCore::pickTile( int cx, int cy )
{
    EdMap& m = cur();
    if ( cx < 0 || cy < 0 || cx >= m.w || cy >= m.h ) return;
    int v = m.at( layer, cx, cy );
    for ( int l = 3; l >= 0 && v < 0; l-- ) v = m.at( l, cx, cy );          // sinon : la tuile du dessus
    if ( v >= 0 ) { selTile = v; setStatus( tr( "ed.status.picked", std::to_string( v ) ) ); gotoTile( v ); }
}

void EditorCore::gotoTile( int id )
{
    if ( id < 0 || id >= tilesetCount ) return;
    selTile = id;
    int row = id / palCols();
    if ( row < palScroll || row >= palScroll + palRows() ) palScroll = std::max( 0, row - palRows() / 2 );
}

// ------------------------------------------------------------------ objets
bool EditorCore::selectAt( int px, int py )
{
    EdMap& m = cur();
    int lx = logicX( px ), ly = logicY( py );
    selKind = EdSel::None; selIdx = -1;
    auto inBox = [&]( int x, int y, int w, int h ) { return lx >= x && lx < x + w && ly >= y && ly < y + h; };
    for ( size_t i = 0; i < m.items.size(); i++ )  if ( inBox( m.items[i].x, m.items[i].y, 32, 32 ) )  { selKind = EdSel::Item;  selIdx = (int)i; return true; }
    for ( size_t i = 0; i < m.npcs.size(); i++ )   if ( inBox( m.npcs[i].x, m.npcs[i].y, 32, 32 ) )   { selKind = EdSel::Npc;   selIdx = (int)i; return true; }
    for ( size_t i = 0; i < m.spawns.size(); i++ ) if ( inBox( m.spawns[i].x, m.spawns[i].y, 32, 32 ) ) { selKind = EdSel::Spawn; selIdx = (int)i; return true; }
    if ( world.startMap == m.name && inBox( world.startX - 16, world.startY - 16, 32, 32 ) ) { selKind = EdSel::Start; selIdx = 0; return true; }
    for ( size_t i = 0; i < m.doors.size(); i++ )  if ( inBox( m.doors[i].x, m.doors[i].y, std::max( 12, m.doors[i].w ), std::max( 12, m.doors[i].h ) ) ) { selKind = EdSel::Door; selIdx = (int)i; return true; }
    return false;
}

static void objectOrigin( EditorCore& e, int& x, int& y )
{
    EdMap& m = e.cur();
    switch ( e.selKind ) {
        case EdSel::Spawn: x = m.spawns[e.selIdx].x; y = m.spawns[e.selIdx].y; break;
        case EdSel::Npc:   x = m.npcs[e.selIdx].x;   y = m.npcs[e.selIdx].y; break;
        case EdSel::Door:  x = m.doors[e.selIdx].x;  y = m.doors[e.selIdx].y; break;
        case EdSel::Item:  x = m.items[e.selIdx].x;  y = m.items[e.selIdx].y; break;
        case EdSel::Start: x = e.world.startX;       y = e.world.startY; break;
        default: x = y = 0;
    }
}

void EditorCore::nudgeSelected( int dx, int dy )
{
    if ( selKind == EdSel::None ) return;
    EdMap& m = cur();
    switch ( selKind ) {
        case EdSel::Spawn: m.spawns[selIdx].x += dx; m.spawns[selIdx].y += dy; break;
        case EdSel::Npc:   m.npcs[selIdx].x += dx;   m.npcs[selIdx].y += dy; break;
        case EdSel::Door:  m.doors[selIdx].x += dx;  m.doors[selIdx].y += dy; break;
        case EdSel::Item:  m.items[selIdx].x += dx;  m.items[selIdx].y += dy; break;
        case EdSel::Start: world.startX += dx;       world.startY += dy; break;
        default: break;
    }
    dirty = true;
}

void EditorCore::placeAt( int cx, int cy )
{
    EdMap& m = cur();
    int x = cx * 32, y = cy * 32;
    switch ( tool ) {
        case EdTool::Spawn: m.spawns.push_back( { kEntityNames[0], x, y, 10, 1 } ); selKind = EdSel::Spawn; selIdx = (int)m.spawns.size() - 1; break;
        case EdTool::Npc: {
            EdNpc n; n.id = "pnj" + std::to_string( m.npcs.size() + 1 ); n.type = kNpcNames[0]; n.x = x; n.y = y; n.rules = "-";
            m.npcs.push_back( n ); selKind = EdSel::Npc; selIdx = (int)m.npcs.size() - 1; break;
        }
        case EdTool::Item: {
            EdItem it; it.id = world.story.itemIds.empty() ? "potion" : world.story.itemIds[0]; it.x = x; it.y = y;
            m.items.push_back( it ); selKind = EdSel::Item; selIdx = (int)m.items.size() - 1; break;
        }
        default: return;
    }
    setStatus( tr( "ed.status.placed" ) );
}

void EditorCore::deleteSelected()
{
    if ( selKind == EdSel::None || selKind == EdSel::Start ) return;
    snapshot();
    EdMap& m = cur();
    switch ( selKind ) {
        case EdSel::Spawn: m.spawns.erase( m.spawns.begin() + selIdx ); break;
        case EdSel::Npc:   m.npcs.erase( m.npcs.begin() + selIdx ); break;
        case EdSel::Door:  m.doors.erase( m.doors.begin() + selIdx ); break;
        case EdSel::Item:  m.items.erase( m.items.begin() + selIdx ); break;
        default: break;
    }
    selKind = EdSel::None; selIdx = -1;
    setStatus( tr( "ed.status.deleted" ) );
}

void EditorCore::cycleType( int dir )
{
    if ( selKind == EdSel::None ) return;
    EdMap& m = cur();
    auto next = [&]( const std::vector<std::string>& list, std::string& cur_ ) {
        if ( list.empty() ) return;
        int i = (int)( std::find( list.begin(), list.end(), cur_ ) - list.begin() );
        cur_ = list[( ( i + dir ) % (int)list.size() + (int)list.size() ) % (int)list.size()];
    };
    snapshot();
    if ( selKind == EdSel::Spawn ) { std::vector<std::string> l( kEntityNames, kEntityNames + kEntityDefCount ); next( l, m.spawns[selIdx].type ); }
    else if ( selKind == EdSel::Npc ) { std::vector<std::string> l( kNpcNames, kNpcNames + kNpcDefCount ); next( l, m.npcs[selIdx].type ); }
    else if ( selKind == EdSel::Item ) next( world.story.itemIds, m.items[selIdx].id );
    else if ( selKind == EdSel::Door ) next( world.mapNames, m.doors[selIdx].map );
}

// ------------------------------------------------------------------ proprietes (une ligne de texte)
std::string EditorCore::propsLine() const
{
    const EdMap& m = world.maps[curMap];
    std::ostringstream o;
    switch ( selKind ) {
        case EdSel::Spawn: { auto& s = m.spawns[selIdx]; o << s.type << ' ' << s.x << ' ' << s.y << ' ' << s.vie << ' ' << s.attaque; break; }
        case EdSel::Npc:   { auto& s = m.npcs[selIdx]; o << s.id << ' ' << s.type << ' ' << s.x << ' ' << s.y << ' ' << s.dir << ' ' << s.rules; break; }
        case EdSel::Door:  { auto& d = m.doors[selIdx]; o << d.x << ' ' << d.y << ' ' << d.w << ' ' << d.h << ' ' << d.map << ' ' << d.dx << ' ' << d.dy << ' ' << d.requires_ << ' ' << d.deny; break; }
        case EdSel::Item:  { auto& s = m.items[selIdx]; o << s.id << ' ' << s.x << ' ' << s.y; break; }
        case EdSel::Start: o << world.startX << ' ' << world.startY << ' ' << world.startDir; break;
        default: break;
    }
    return o.str();
}

bool EditorCore::validateRules( const std::string& rules, std::string& why ) const
{
    if ( rules == "-" ) return true;
    std::stringstream ss( rules ); std::string rule;
    while ( std::getline( ss, rule, ';' ) ) {
        size_t q = rule.rfind( '?' );
        std::string cond = q == std::string::npos ? "" : rule.substr( 0, q ), dl = q == std::string::npos ? rule : rule.substr( q + 1 );
        if ( !contains( world.story.dialogues, dl ) ) { why = "dialogue " + dl; return false; }
        std::string c = !cond.empty() && cond[0] == '!' ? cond.substr( 1 ) : cond;
        if ( c.compare( 0, 4, "has:" ) == 0 ) { if ( !contains( world.story.itemIds, c.substr( 4 ) ) ) { why = "objet " + c.substr( 4 ); return false; } }
        else if ( !c.empty() && !contains( world.story.flags, c ) ) { why = "drapeau " + c; return false; }
    }
    return true;
}

bool EditorCore::applyProps( const std::string& line, std::string& why )
{
    auto t = tokens( line );
    EdMap copy = cur();
    EdWorld wcopy;                                           // seul le depart est modifiable ici
    switch ( selKind ) {
        case EdSel::Spawn: {
            if ( t.size() != 5 || !isInt( t[1] ) || !isInt( t[2] ) || !isInt( t[3] ) || !isInt( t[4] ) ) { why = "type x y vie attaque"; return false; }
            if ( std::find( kEntityNames, kEntityNames + kEntityDefCount, t[0] ) == kEntityNames + kEntityDefCount ) { why = "monstre " + t[0]; return false; }
            copy.spawns[selIdx] = { t[0], atoi( t[1].c_str() ), atoi( t[2].c_str() ), atoi( t[3].c_str() ), atoi( t[4].c_str() ) }; break;
        }
        case EdSel::Npc: {
            if ( t.size() != 6 || !isInt( t[2] ) || !isInt( t[3] ) ) { why = "id type x y dir regles"; return false; }
            if ( std::find( kNpcNames, kNpcNames + kNpcDefCount, t[1] ) == kNpcNames + kNpcDefCount ) { why = "type de PNJ " + t[1]; return false; }
            if ( std::find( kDirs, kDirs + 4, t[4] ) == kDirs + 4 ) { why = "direction " + t[4]; return false; }
            if ( !validateRules( t[5], why ) ) return false;
            EdNpc n; n.id = t[0]; n.type = t[1]; n.x = atoi( t[2].c_str() ); n.y = atoi( t[3].c_str() ); n.dir = t[4]; n.rules = t[5];
            copy.npcs[selIdx] = n; break;
        }
        case EdSel::Door: {
            if ( t.size() != 9 || !isInt( t[0] ) || !isInt( t[1] ) || !isInt( t[2] ) || !isInt( t[3] ) || !isInt( t[5] ) || !isInt( t[6] ) ) { why = "x y l h carte dx dy drapeau|- dialogue|-"; return false; }
            if ( atoi( t[2].c_str() ) < 1 || atoi( t[3].c_str() ) < 1 ) { why = "largeur/hauteur > 0"; return false; }
            if ( !contains( world.mapNames, t[4] ) ) { why = "carte " + t[4]; return false; }
            if ( t[7] != "-" && !contains( world.story.flags, t[7] ) ) { why = "drapeau " + t[7]; return false; }
            if ( t[8] != "-" && !contains( world.story.dialogues, t[8] ) ) { why = "dialogue " + t[8]; return false; }
            EdDoor d; d.x = atoi( t[0].c_str() ); d.y = atoi( t[1].c_str() ); d.w = atoi( t[2].c_str() ); d.h = atoi( t[3].c_str() ); d.map = t[4];
            d.dx = atoi( t[5].c_str() ); d.dy = atoi( t[6].c_str() ); d.requires_ = t[7]; d.deny = t[8];
            copy.doors[selIdx] = d; break;
        }
        case EdSel::Item: {
            if ( t.size() != 3 || !isInt( t[1] ) || !isInt( t[2] ) ) { why = "objet x y"; return false; }
            if ( !contains( world.story.itemIds, t[0] ) ) { why = "objet " + t[0]; return false; }
            copy.items[selIdx] = { t[0], atoi( t[1].c_str() ), atoi( t[2].c_str() ) }; break;
        }
        case EdSel::Start: {
            if ( t.size() != 3 || !isInt( t[0] ) || !isInt( t[1] ) || std::find( kDirs, kDirs + 4, t[2] ) == kDirs + 4 ) { why = "x y direction"; return false; }
            snapshot(); world.startX = atoi( t[0].c_str() ); world.startY = atoi( t[1].c_str() ); world.startDir = t[2]; return true;
        }
        default: why = "rien de selectionne"; return false;
    }
    snapshot();
    cur() = copy;
    return true;
}

void EditorCore::startPrompt( Prompt k, const std::string& label, const std::string& initial )
{
    promptKind = k; promptLabel = label; promptText = initial;
}

bool EditorCore::newMap( const std::string& line )
{
    auto t = tokens( line );
    if ( t.size() != 3 || !isInt( t[1] ) || !isInt( t[2] ) ) return false;
    int w = atoi( t[1].c_str() ), h = atoi( t[2].c_str() );
    if ( w < 4 || h < 4 || w > 200 || h > 200 ) return false;
    for ( char c : t[0] ) if ( !( ( c >= 'a' && c <= 'z' ) || ( c >= '0' && c <= '9' ) || c == '_' ) ) return false;
    if ( contains( world.mapNames, t[0] ) ) return false;
    EdMap m; m.name = t[0]; m.resetLayers( w, h );
    world.maps.push_back( m ); world.mapNames.push_back( t[0] );
    undoStack.emplace_back(); redoStack.emplace_back();
    curMap = (int)world.maps.size() - 1; scrollX = scrollY = 0; selKind = EdSel::None; dirty = true;
    return true;
}

void EditorCore::switchMap( int delta )
{
    int n = (int)world.maps.size();
    curMap = ( ( curMap + delta ) % n + n ) % n;
    scrollX = scrollY = 0; selKind = EdSel::None; selIdx = -1;
}

void EditorCore::commitPrompt()
{
    Prompt k = promptKind; promptKind = Prompt::None;
    std::string why;
    if ( k == Prompt::Props ) { if ( applyProps( promptText, why ) ) setStatus( tr( "ed.status.applied" ) ); else setStatus( tr( "ed.status.bad_value", why ) ); }
    else if ( k == Prompt::NewMap ) { if ( newMap( promptText ) ) setStatus( tr( "ed.status.newmap", cur().name ) ); else setStatus( tr( "ed.status.bad_value", promptText ) ); }
    else if ( k == Prompt::Goto ) { if ( isInt( promptText ) && atoi( promptText.c_str() ) < tilesetCount ) gotoTile( atoi( promptText.c_str() ) ); else setStatus( tr( "ed.status.bad_value", promptText ) ); }
    else if ( k == Prompt::Music ) {
        if ( std::find( kMusics, kMusics + 5, promptText ) != kMusics + 5 ) { snapshot(); cur().music = promptText; setStatus( tr( "ed.status.applied" ) ); }
        else setStatus( tr( "ed.status.bad_value", promptText ) );
    }
}

bool EditorCore::saveAll()
{
    std::string err;
    int n = 0;
    for ( const EdMap& m : world.maps ) { if ( !saveMapFile( world.dir, m, err ) ) { setStatus( tr( "ed.status.save_failed", err ) ); return false; } n++; }
    if ( !saveWorldFile( world.dir, world, err ) || !saveTilesFile( world.dir, world, err ) ) { setStatus( tr( "ed.status.save_failed", err ) ); return false; }
    // avertissements simples : references de portes et d'objets
    std::vector<std::string> warn;
    for ( const EdMap& m : world.maps ) {
        for ( const EdDoor& d : m.doors ) if ( !contains( world.mapNames, d.map ) ) warn.push_back( m.name + ": porte -> " + d.map );
        for ( const EdItem& i : m.items ) if ( !contains( world.story.itemIds, i.id ) ) warn.push_back( m.name + ": objet " + i.id );
    }
    dirty = false;
    setStatus( warn.empty() ? tr( "ed.status.saved", std::to_string( n ) ) : tr( "ed.status.warn", std::to_string( warn.size() ), warn[0] ) );
    return true;
}

void EditorCore::runBuild()
{
    if ( buildCmd.empty() ) { setStatus( tr( "ed.build.none" ) ); return; }
    if ( dirty ) saveAll();
    setStatus( tr( "ed.build.run" ) );
    int rc = system( buildCmd.c_str() );
    setStatus( tr( "ed.build.done", std::to_string( rc ) ) );
}

void EditorCore::clampScroll()
{
    EdMap& m = cur();
    int maxX = std::max( 0, m.w * kTile - ( vw - kPanel ) / 2 ), maxY = std::max( 0, m.h * kTile - ( vh - kStatusH ) / 2 );
    scrollX = std::max( -( vw - kPanel ) / 2, std::min( scrollX, maxX ) );
    scrollY = std::max( -( vh - kStatusH ) / 2, std::min( scrollY, maxY ) );
}

// ------------------------------------------------------------------ evenements
void EditorCore::onKey( const EdEvent& e )
{
    int k = e.key;
    if ( promptKind != Prompt::None ) {
        if ( k == ED_ESC ) promptKind = Prompt::None;
        else if ( k == ED_ENTER ) commitPrompt();
        else if ( k == ED_BACKSPACE && !promptText.empty() ) {
            size_t i = promptText.size() - 1; while ( i > 0 && ( (uint8_t)promptText[i] & 0xC0 ) == 0x80 ) i--;
            promptText.erase( i );
        }
        return;
    }
    if ( e.ctrl ) {
        switch ( k ) {
            case 'z': doUndo( false ); break;
            case 'y': doUndo( true ); break;
            case 's': saveAll(); break;
            case 'n': startPrompt( Prompt::NewMap, tr( "ed.prompt.newmap" ), "" ); break;
            case 'm': startPrompt( Prompt::Music, tr( "ed.prompt.music" ), cur().music ); break;
            default: break;
        }
        return;
    }
    switch ( k ) {
        case 'b': tool = EdTool::Brush; break;
        case 'f': tool = EdTool::Fill; break;
        case 'r': tool = EdTool::Rect; break;
        case 'e': tool = EdTool::Erase; break;
        case 'i': tool = EdTool::Pick; break;
        case 'v': tool = EdTool::Select; break;
        case 'm': tool = EdTool::Spawn; break;
        case 'n': tool = EdTool::Npc; break;
        case 'd': tool = EdTool::Door; break;
        case 't': tool = EdTool::Item; break;
        case 's': tool = EdTool::Start; break;
        case 'g': showGrid = !showGrid; break;
        case 'c': showColl = !showColl; break;
        case 'h': showHelp = !showHelp; break;
        case 'j': startPrompt( Prompt::Goto, tr( "ed.prompt.goto" ), "" ); break;
        case 'p': if ( selKind != EdSel::None ) startPrompt( Prompt::Props, tr( "ed.prompt.props" ), propsLine() ); break;
        case 'x':
            if ( world.collide.count( selTile ) ) { world.collide.erase( selTile ); setStatus( tr( "ed.status.collide_off", std::to_string( selTile ) ) ); }
            else { world.collide.insert( selTile ); setStatus( tr( "ed.status.collide_on", std::to_string( selTile ) ) ); }
            dirty = true; break;
        case '1': case '2': case '3': case '4': layer = k - '1'; break;
        case ',': cycleType( -1 ); break;
        case '.': cycleType( 1 ); break;
        case ED_TAB: switchMap( e.shift ? -1 : 1 ); break;
        case ED_DELETE: deleteSelected(); break;
        case ED_PGUP: palScroll = std::max( 0, palScroll - palRows() ); break;
        case ED_PGDN: palScroll = std::min( std::max( 0, ( tilesetCount + palCols() - 1 ) / palCols() - palRows() ), palScroll + palRows() ); break;
        case ED_UP: nudgeSelected( 0, e.shift ? -32 : -8 ); break;
        case ED_DOWN: nudgeSelected( 0, e.shift ? 32 : 8 ); break;
        case ED_LEFT: nudgeSelected( e.shift ? -32 : -8, 0 ); break;
        case ED_RIGHT: nudgeSelected( e.shift ? 32 : 8, 0 ); break;
        case ED_SPACE: panHeld = true; break;
        case ED_ESC: if ( showHelp ) showHelp = false; else { selKind = EdSel::None; selIdx = -1; } break;
        default:
            if ( k >= ED_F1 && k < ED_F1 + 9 ) {
                int n = k - ED_F1;
                if ( n < 4 ) layerVisible[n] = !layerVisible[n];
                else if ( n == 8 ) runBuild();
            }
            break;
    }
}

void EditorCore::handle( const EdEvent& e )
{
    switch ( e.type ) {
        case EdEvent::KeyDown: onKey( e ); return;
        case EdEvent::KeyUp: if ( e.key == ED_SPACE ) panHeld = false; return;
        case EdEvent::Text:
            if ( promptKind != Prompt::None ) promptText += e.text;
            return;
        case EdEvent::Wheel:
            if ( e.x >= vw - kPanel ) {                              // palette
                int maxRow = std::max( 0, ( tilesetCount + palCols() - 1 ) / palCols() - palRows() );
                palScroll = std::max( 0, std::min( maxRow, palScroll - e.dy * 3 ) );
            } else if ( e.shift ) { scrollX -= e.dy * kTile * 2; clampScroll(); }
            else { scrollY -= e.dy * kTile * 2; clampScroll(); }
            return;
        case EdEvent::MouseMove: {
            mouseX = e.x; mouseY = e.y;
            if ( panning ) { scrollX = panScrollX - ( e.x - panStartX ); scrollY = panScrollY - ( e.y - panStartY ); clampScroll(); return; }
            if ( !mouseDown || !inCanvas( e.x, e.y ) ) return;
            int cx = cellX( e.x ), cy = cellY( e.y );
            if ( tool == EdTool::Brush || tool == EdTool::Erase ) {
                if ( cx != lastCellX || cy != lastCellY ) { paintLine( lastCellX, lastCellY, cx, cy, tool == EdTool::Brush ? selTile : -1 ); lastCellX = cx; lastCellY = cy; }
            } else if ( tool == EdTool::Select && selKind != EdSel::None ) {
                int ox, oy; objectOrigin( *this, ox, oy );
                int nx = ( logicX( e.x ) - dragOffX ) / 8 * 8, ny = ( logicY( e.y ) - dragOffY ) / 8 * 8;
                if ( nx != ox || ny != oy ) { if ( !dragMoved ) { snapshot(); dragMoved = true; } nudgeSelected( nx - ox, ny - oy ); }
            }
            return;
        }
        case EdEvent::MouseDown: {
            mouseX = e.x; mouseY = e.y;
            if ( promptKind != Prompt::None ) return;
            if ( e.button == 2 || ( e.button == 1 && panHeld ) ) { panning = true; panStartX = e.x; panStartY = e.y; panScrollX = scrollX; panScrollY = scrollY; return; }
            if ( e.x >= vw - kPanel ) {                              // palette : choisir une tuile
                int col = ( e.x - ( vw - kPanel + 8 ) ) / kTile, row = ( e.y - palTop() ) / kTile;
                if ( e.y >= palTop() && col >= 0 && col < palCols() && row >= 0 && row < palRows() ) {
                    int id = ( palScroll + row ) * palCols() + col;
                    if ( id < tilesetCount ) { selTile = id; if ( tool != EdTool::Brush && tool != EdTool::Fill && tool != EdTool::Rect ) tool = EdTool::Brush; }
                }
                return;
            }
            if ( !inCanvas( e.x, e.y ) ) return;
            int cx = cellX( e.x ), cy = cellY( e.y );
            if ( e.button == 3 ) { pickTile( cx, cy ); return; }
            mouseDown = true; lastCellX = startCellX = cx; lastCellY = startCellY = cy; dragMoved = false;
            switch ( tool ) {
                case EdTool::Brush: snapshot(); paintCell( cx, cy, selTile ); break;
                case EdTool::Erase: snapshot(); paintCell( cx, cy, -1 ); break;
                case EdTool::Fill: snapshot(); floodFill( cx, cy, selTile ); break;
                case EdTool::Pick: pickTile( cx, cy ); break;
                case EdTool::Select:
                    if ( selectAt( e.x, e.y ) ) {
                        int ox, oy; objectOrigin( *this, ox, oy );
                        dragOffX = logicX( e.x ) - ox; dragOffY = logicY( e.y ) - oy;
                        setStatus( tr( "ed.status.selected", propsLine() ) );
                    }
                    break;
                case EdTool::Spawn: case EdTool::Npc: case EdTool::Item: snapshot(); placeAt( cx, cy ); break;
                case EdTool::Start:
                    snapshot();
                    world.startMap = cur().name; world.startX = cx * 32 + 16; world.startY = cy * 32 + 16;
                    selKind = EdSel::Start; selIdx = 0; setStatus( tr( "ed.status.start_set" ) );
                    break;
                default: break;                                      // Rect, Door : au relachement
            }
            return;
        }
        case EdEvent::MouseUp: {
            if ( panning ) { panning = false; return; }
            if ( !mouseDown ) return;
            mouseDown = false;
            int cx = cellX( e.x ), cy = cellY( e.y );
            if ( tool == EdTool::Rect ) { snapshot(); fillRect( startCellX, startCellY, cx, cy, selTile ); }
            else if ( tool == EdTool::Door && cx >= 0 && cy >= 0 ) {
                snapshot();
                EdDoor d; int x0 = std::min( startCellX, cx ), x1 = std::max( startCellX, cx ), y0 = std::min( startCellY, cy ), y1 = std::max( startCellY, cy );
                d.x = x0 * 32; d.y = y0 * 32; d.w = ( x1 - x0 + 1 ) * 32; d.h = ( y1 - y0 + 1 ) * 32;
                d.map = world.mapNames[( curMap + 1 ) % world.mapNames.size()]; d.dx = 64; d.dy = 64;
                cur().doors.push_back( d ); selKind = EdSel::Door; selIdx = (int)cur().doors.size() - 1;
                setStatus( tr( "ed.status.placed" ) );
            }
            dragMoved = false;
            return;
        }
    }
}

// ------------------------------------------------------------------ rendu
std::string EditorCore::toolName() const
{
    static const char* keys[] = { "ed.tool.brush", "ed.tool.fill", "ed.tool.rect", "ed.tool.erase", "ed.tool.pick", "ed.tool.select",
                                  "ed.tool.spawn", "ed.tool.npc", "ed.tool.door", "ed.tool.item", "ed.tool.start" };
    return tr( keys[(int)tool] );
}

void EditorCore::drawCanvas( SoftRenderer& r )
{
    r.fillRect( 0, 0, vw - kPanel, vh - kStatusH, kBg );
    EdMap& m = cur();
    int cx0 = std::max( 0, scrollX / kTile ), cy0 = std::max( 0, scrollY / kTile );
    int cx1 = std::min( m.w - 1, ( scrollX + vw - kPanel ) / kTile ), cy1 = std::min( m.h - 1, ( scrollY + vh - kStatusH ) / kTile );
    for ( int cy = cy0; cy <= cy1; cy++ )
        for ( int cx = cx0; cx <= cx1; cx++ ) {
            int px = cx * kTile - scrollX, py = cy * kTile - scrollY;
            r.fillRect( px, py, kTile, kTile, ( ( cx + cy ) & 1 ) ? RGBColor{ 34, 34, 42 } : RGBColor{ 38, 38, 46 } );
            for ( int l = 0; l < 4; l++ ) {
                if ( !layerVisible[l] ) continue;
                int t = m.at( l, cx, cy );
                if ( t >= 0 && t < tilesetCount ) r.drawImage( px, py, tileImg[t] );
            }
            if ( showColl )
                for ( int l = 0; l < 3; l++ ) {
                    int t = m.at( l, cx, cy );
                    if ( t >= 0 && world.collide.count( t ) ) { r.fillRect( px, py, 5, 5, kRed ); break; }
                }
        }
    if ( showGrid ) {
        for ( int cx = cx0; cx <= cx1 + 1; cx++ ) r.fillRect( cx * kTile - scrollX, std::max( 0, -scrollY ), 1, std::min( vh - kStatusH, m.h * kTile - scrollY ), RGBColor{ 55, 55, 66 } );
        for ( int cy = cy0; cy <= cy1 + 1; cy++ ) r.fillRect( std::max( 0, -scrollX ), cy * kTile - scrollY, std::min( vw - kPanel, m.w * kTile - scrollX ), 1, RGBColor{ 55, 55, 66 } );
    }
    drawObjects( r );
    if ( mouseDown && ( tool == EdTool::Rect || tool == EdTool::Door ) ) {          // apercu du rectangle en cours
        int x0 = std::min( startCellX, cellX( mouseX ) ), x1 = std::max( startCellX, cellX( mouseX ) ), y0 = std::min( startCellY, cellY( mouseY ) ), y1 = std::max( startCellY, cellY( mouseY ) );
        int px = x0 * kTile - scrollX, py = y0 * kTile - scrollY, w = ( x1 - x0 + 1 ) * kTile, h = ( y1 - y0 + 1 ) * kTile;
        RGBColor c = tool == EdTool::Door ? kCyan : kYellow;
        r.fillRect( px, py, w, 2, c ); r.fillRect( px, py + h - 2, w, 2, c ); r.fillRect( px, py, 2, h, c ); r.fillRect( px + w - 2, py, 2, h, c );
    }
    if ( inCanvas( mouseX, mouseY ) && !panning ) {                                  // curseur : case survolee
        int cx = cellX( mouseX ), cy = cellY( mouseY );
        if ( cx >= 0 && cy >= 0 && cx < m.w && cy < m.h ) {
            int px = cx * kTile - scrollX, py = cy * kTile - scrollY;
            r.fillRect( px, py, kTile, 1, kYellow ); r.fillRect( px, py + kTile - 1, kTile, 1, kYellow ); r.fillRect( px, py, 1, kTile, kYellow ); r.fillRect( px + kTile - 1, py, 1, kTile, kYellow );
        }
    }
}

void EditorCore::drawObjects( SoftRenderer& r )
{
    EdMap& m = cur();
    auto frame = [&]( int x, int y, int w, int h, RGBColor c ) { r.fillRect( x, y, w, 1, c ); r.fillRect( x, y + h - 1, w, 1, c ); r.fillRect( x, y, 1, h, c ); r.fillRect( x + w - 1, y, 1, h, c ); };
    auto sprite = [&]( ImageId id, int lx, int ly, const char* letter, RGBColor c ) {
        int px = toDispX( lx ), py = toDispY( ly );
        if ( id != kInvalidImageId ) r.drawImage( px, py, id ); else { frame( px, py, kTile, kTile, c ); r.drawText( px + 8, py + 8, letter, c, FontSize::Narrow ); }
    };
    for ( size_t i = 0; i < m.spawns.size(); i++ ) {
        sprite( spriteFor( m.spawns[i].type ), m.spawns[i].x, m.spawns[i].y, "M", kRed );
        frame( toDispX( m.spawns[i].x ), toDispY( m.spawns[i].y ), kTile, kTile, selKind == EdSel::Spawn && selIdx == (int)i ? kYellow : kRed );
    }
    for ( size_t i = 0; i < m.npcs.size(); i++ ) {
        sprite( spriteFor( m.npcs[i].type ), m.npcs[i].x, m.npcs[i].y, "N", kGreen );
        frame( toDispX( m.npcs[i].x ), toDispY( m.npcs[i].y ), kTile, kTile, selKind == EdSel::Npc && selIdx == (int)i ? kYellow : kGreen );
    }
    for ( size_t i = 0; i < m.items.size(); i++ ) {
        ImageId id = kInvalidImageId;
        for ( size_t k = 0; k < world.story.itemIds.size(); k++ ) if ( world.story.itemIds[k] == m.items[i].id ) { auto it = spriteImg.find( world.story.itemIcons[k] ); if ( it != spriteImg.end() ) id = it->second; }
        sprite( id, m.items[i].x, m.items[i].y, "I", kYellow );
        if ( selKind == EdSel::Item && selIdx == (int)i ) frame( toDispX( m.items[i].x ), toDispY( m.items[i].y ), kTile, kTile, kYellow );
    }
    for ( size_t i = 0; i < m.doors.size(); i++ ) {
        const EdDoor& d = m.doors[i];
        int px = toDispX( d.x ), py = toDispY( d.y ), w = std::max( 4, d.w * kTile / 32 ), h = std::max( 4, d.h * kTile / 32 );
        RGBColor c = selKind == EdSel::Door && selIdx == (int)i ? kYellow : kCyan;
        frame( px, py, w, h, c ); frame( px + 1, py + 1, w - 2, h - 2, c );
        r.drawText( px + 2, py + h + 1, ( "-> " + d.map ).c_str(), c );
    }
    if ( world.startMap == m.name ) {                                                  // depart du joueur (centre de la hitbox)
        int px = toDispX( world.startX ), py = toDispY( world.startY );
        r.fillRect( px - 6, py - 1, 13, 3, kGreen ); r.fillRect( px - 1, py - 6, 3, 13, kGreen );
        if ( selKind == EdSel::Start ) { r.fillRect( px - 9, py - 9, 19, 1, kYellow ); r.fillRect( px - 9, py + 9, 19, 1, kYellow ); r.fillRect( px - 9, py - 9, 1, 19, kYellow ); r.fillRect( px + 9, py - 9, 1, 19, kYellow ); }
    }
}

void EditorCore::drawPanel( SoftRenderer& r )
{
    int x0 = vw - kPanel;
    r.fillRect( x0, 0, kPanel, vh - kStatusH, kPanelC );
    EdMap& m = cur();
    r.drawText( x0 + 6, 4, tr( "ed.title" ).c_str(), kYellow );
    r.drawText( x0 + 6, 16, ( m.name + "  " + std::to_string( m.w ) + "x" + std::to_string( m.h ) + "  [" + m.music + "]" ).c_str(), kText );
    r.drawText( x0 + 6, 28, ( toolName() + "   " + tr( "ed.layer" ) + " " + std::to_string( layer + 1 ) ).c_str(), kText );
    std::string vis; for ( int l = 0; l < 4; l++ ) vis += layerVisible[l] ? std::to_string( l + 1 ) : "-";
    r.drawText( x0 + 6, 40, ( tr( "ed.tile" ) + " " + std::to_string( selTile ) + ( world.collide.count( selTile ) ? " [X]" : "" ) + "  " + vis ).c_str(), kText );
    if ( selKind != EdSel::None ) {
        std::string d = propsLine();
        r.drawText( x0 + 6, 52, d.substr( 0, ( kPanel - 12 ) / 8 ).c_str(), kCyan );
        if ( d.size() > (size_t)( kPanel - 12 ) / 8 ) r.drawText( x0 + 6, 62, d.substr( ( kPanel - 12 ) / 8, ( kPanel - 12 ) / 8 ).c_str(), kCyan );
    }
    // palette de tuiles
    int cols = palCols(), rows = palRows();
    for ( int row = 0; row < rows; row++ )
        for ( int col = 0; col < cols; col++ ) {
            int id = ( palScroll + row ) * cols + col;
            if ( id >= tilesetCount ) break;
            int px = x0 + 8 + col * kTile, py = palTop() + row * kTile;
            r.fillRect( px, py, kTile, kTile, RGBColor{ 52, 52, 62 } );
            r.drawImage( px, py, tileImg[id] );
            if ( world.collide.count( id ) ) r.fillRect( px, py, kTile, 1, kRed );
            if ( world.anim.count( id ) ) r.fillRect( px + kTile - 3, py, 3, 3, kYellow );
            if ( id == selTile ) { r.fillRect( px, py, kTile, 1, kText ); r.fillRect( px, py + kTile - 1, kTile, 1, kText ); r.fillRect( px, py, 1, kTile, kText ); r.fillRect( px + kTile - 1, py, 1, kTile, kText ); }
        }
}

void EditorCore::drawStatus( SoftRenderer& r )
{
    int y = vh - kStatusH;
    r.fillRect( 0, y, vw, kStatusH, RGBColor{ 20, 20, 26 } );
    if ( promptKind != Prompt::None ) {
        r.drawText( 6, y + 4, ( promptLabel + " :" ).c_str(), kYellow );
        r.drawText( 6, y + 18, ( promptText + "_" ).c_str(), kText );
        return;
    }
    r.drawText( 6, y + 4, status.c_str(), kText );
    std::string info;
    if ( inCanvas( mouseX, mouseY ) ) {
        int cx = cellX( mouseX ), cy = cellY( mouseY );
        info = "(" + std::to_string( logicX( mouseX ) ) + "," + std::to_string( logicY( mouseY ) ) + ") ";
        EdMap& m = cur();
        if ( cx >= 0 && cy >= 0 && cx < m.w && cy < m.h ) { info += "case " + std::to_string( cx ) + "," + std::to_string( cy ) + " :"; for ( int l = 0; l < 4; l++ ) info += " " + std::to_string( m.at( l, cx, cy ) ); }
    }
    if ( dirty ) info += "  * " + tr( "ed.status.dirty" );
    r.drawText( 6, y + 18, info.c_str(), kDim );
    r.drawText( 6, y + 29, "B F R E I V | M N D T S | H ?", kDim );
}

void EditorCore::drawHelp( SoftRenderer& r )
{
    int w = std::min( vw - 40, 640 ), h = 14 * 11 + 22, x = ( vw - kPanel - w ) / 2 + 10, y = 24;
    r.fillRect( x - 2, y - 2, w + 4, h + 4, kYellow );
    r.fillRect( x, y, w, h, RGBColor{ 18, 20, 30 } );
    r.drawText( x + 8, y + 6, tr( "ed.help.title" ).c_str(), kYellow );
    for ( int i = 1; i <= 10; i++ ) r.drawText( x + 8, y + 10 + i * 12, tr( "ed.help." + std::to_string( i ) ).c_str(), kText );
}

void EditorCore::draw( SoftRenderer& r )
{
    ensureImages( r );
    vw = r.W; vh = r.H;
    drawCanvas( r );
    drawPanel( r );
    drawStatus( r );
    if ( showHelp ) drawHelp( r );
}
