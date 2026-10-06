#include "WorldText.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

static std::vector<std::string> tokens( const std::string& s )
{
    std::vector<std::string> t; std::istringstream is( s ); std::string x;
    while ( is >> x ) t.push_back( x );
    return t;
}
static bool readLines( const std::string& path, std::vector<std::string>& out )
{
    std::ifstream f( path, std::ios::binary );
    if ( !f ) return false;
    std::string l;
    while ( std::getline( f, l ) ) {
        while ( !l.empty() && ( l.back() == '\r' || l.back() == '\n' ) ) l.pop_back();
        size_t a = l.find_first_not_of( " \t" );
        if ( a == std::string::npos || l[a] == '#' ) continue;
        out.push_back( l.substr( a ) );
    }
    return true;
}
static bool isCsvRow( const std::string& s )
{
    if ( s.empty() ) return false;
    for ( char c : s ) if ( !( ( c >= '0' && c <= '9' ) || c == ',' || c == '-' ) ) return false;
    return true;
}
static bool fail( std::string& err, const std::string& file, const std::string& msg ) { err = file + " : " + msg; return false; }

static bool parseMap( const std::string& path, EdMap& m, std::string& err )
{
    std::vector<std::string> lines;
    if ( !readLines( path, lines ) ) return fail( err, path, "illisible" );
    int layer = -1, row = 0; bool sized = false;
    std::string name;
    for ( const std::string& s : lines ) {
        if ( layer >= 0 && isCsvRow( s ) ) {
            std::vector<int> vals; std::stringstream ss( s ); std::string v;
            while ( std::getline( ss, v, ',' ) ) if ( !v.empty() ) vals.push_back( atoi( v.c_str() ) );
            if ( (int)vals.size() != m.w || row >= m.h ) return fail( err, path, "ligne de tuiles de longueur incorrecte" );
            std::copy( vals.begin(), vals.end(), m.layer[layer].begin() + (size_t)row * m.w );
            row++;
            continue;
        }
        auto t = tokens( s );
        const std::string& k = t[0];
        if ( k == "map" && t.size() >= 2 ) name = t[1];
        else if ( k == "size" && t.size() >= 3 ) { m.name = name; m.resetLayers( atoi( t[1].c_str() ), atoi( t[2].c_str() ) ); sized = true; }
        else if ( !sized ) return fail( err, path, "'size' doit preceder le reste" );
        else if ( k == "music" && t.size() >= 2 ) m.music = t[1];
        else if ( k == "layer" && t.size() >= 2 ) { layer = atoi( t[1].c_str() ); row = 0; if ( layer < 0 || layer > 3 ) return fail( err, path, "couche 0..3" ); }
        else if ( k == "spawn" && t.size() == 6 ) m.spawns.push_back( { t[1], atoi( t[2].c_str() ), atoi( t[3].c_str() ), atoi( t[4].c_str() ), atoi( t[5].c_str() ) } );
        else if ( k == "npc" && t.size() == 7 ) { EdNpc n; n.id = t[1]; n.type = t[2]; n.x = atoi( t[3].c_str() ); n.y = atoi( t[4].c_str() ); n.dir = t[5]; n.rules = t[6]; m.npcs.push_back( n ); }
        else if ( k == "door" && t.size() == 10 ) { EdDoor d; d.x = atoi( t[1].c_str() ); d.y = atoi( t[2].c_str() ); d.w = atoi( t[3].c_str() ); d.h = atoi( t[4].c_str() ); d.map = t[5]; d.dx = atoi( t[6].c_str() ); d.dy = atoi( t[7].c_str() ); d.requires_ = t[8]; d.deny = t[9]; m.doors.push_back( d ); }
        else if ( k == "item" && t.size() == 4 ) m.items.push_back( { t[1], atoi( t[2].c_str() ), atoi( t[3].c_str() ) } );
        else return fail( err, path, "ligne inconnue ou incomplete : " + s );
    }
    return sized;
}

bool loadWorld( const std::string& dir, EdWorld& w, std::string& err )
{
    w = EdWorld(); w.dir = dir;
    std::vector<std::string> lines;
    if ( !readLines( dir + "/world.txt", lines ) ) return fail( err, dir + "/world.txt", "introuvable" );
    for ( const std::string& s : lines ) {
        auto t = tokens( s );
        if ( t[0] == "start" && t.size() == 5 ) { w.startMap = t[1]; w.startX = atoi( t[2].c_str() ); w.startY = atoi( t[3].c_str() ); w.startDir = t[4]; }
        else if ( t[0] == "maps" ) w.mapNames.assign( t.begin() + 1, t.end() );
    }
    lines.clear();
    if ( readLines( dir + "/tiles.txt", lines ) )
        for ( const std::string& s : lines ) {
            auto t = tokens( s );
            if ( t[0] == "collide" ) for ( size_t i = 1; i < t.size(); i++ ) w.collide.insert( atoi( t[i].c_str() ) );
            else if ( t[0] == "anim" && t.size() == 3 ) w.anim[atoi( t[1].c_str() )] = atoi( t[2].c_str() );
        }
    lines.clear();
    if ( readLines( dir + "/story.txt", lines ) )
        for ( const std::string& s : lines ) {
            auto t = tokens( s );
            if ( t[0] == "flag" && t.size() == 2 ) w.story.flags.push_back( t[1] );
            else if ( t[0] == "itemdef" && t.size() == 5 ) { w.story.itemIds.push_back( t[1] ); w.story.itemIcons.push_back( t[2] ); }
            else if ( t[0] == "dialogue" && t.size() == 2 ) w.story.dialogues.push_back( t[1] );
            else if ( t[0] == "objective" && t.size() >= 2 ) w.story.objectives.push_back( t[1] );
        }
    for ( const std::string& n : w.mapNames ) {
        EdMap m;
        if ( !parseMap( dir + "/maps/" + n + ".txt", m, err ) ) return false;
        w.maps.push_back( std::move( m ) );
    }
    return true;
}

bool saveMapFile( const std::string& dir, const EdMap& m, std::string& err )
{
    std::string path = dir + "/maps/" + m.name + ".txt";
    FILE* f = fopen( path.c_str(), "wb" );
    if ( !f ) return fail( err, path, "ecriture impossible" );
    fprintf( f, "map %s\nsize %d %d\nmusic %s\n", m.name.c_str(), m.w, m.h, m.music.c_str() );
    for ( int l = 0; l < 4; l++ ) {
        fprintf( f, "layer %d\n", l );
        for ( int y = 0; y < m.h; y++ ) {
            for ( int x = 0; x < m.w; x++ ) fprintf( f, x ? ",%d" : "%d", m.layer[l][(size_t)y * m.w + x] );
            fputc( '\n', f );
        }
    }
    for ( const EdSpawn& s : m.spawns ) fprintf( f, "spawn %s %d %d %d %d\n", s.type.c_str(), s.x, s.y, s.vie, s.attaque );
    for ( const EdNpc& s : m.npcs ) fprintf( f, "npc %s %s %d %d %s %s\n", s.id.c_str(), s.type.c_str(), s.x, s.y, s.dir.c_str(), s.rules.c_str() );
    for ( const EdDoor& d : m.doors ) fprintf( f, "door %d %d %d %d %s %d %d %s %s\n", d.x, d.y, d.w, d.h, d.map.c_str(), d.dx, d.dy, d.requires_.c_str(), d.deny.c_str() );
    for ( const EdItem& s : m.items ) fprintf( f, "item %s %d %d\n", s.id.c_str(), s.x, s.y );
    fclose( f );
    return true;
}

bool saveWorldFile( const std::string& dir, const EdWorld& w, std::string& err )
{
    std::string path = dir + "/world.txt";
    FILE* f = fopen( path.c_str(), "wb" );
    if ( !f ) return fail( err, path, "ecriture impossible" );
    fprintf( f, "# Monde d'Ethynd -- voir tools/world_io.py pour le format\nstart %s %d %d %s\nmaps", w.startMap.c_str(), w.startX, w.startY, w.startDir.c_str() );
    for ( const std::string& n : w.mapNames ) fprintf( f, " %s", n.c_str() );
    fputc( '\n', f );
    fclose( f );
    return true;
}

bool saveTilesFile( const std::string& dir, const EdWorld& w, std::string& err )
{
    std::string path = dir + "/tiles.txt";
    FILE* f = fopen( path.c_str(), "wb" );
    if ( !f ) return fail( err, path, "ecriture impossible" );
    fprintf( f, "# Proprietes des tuiles (ids du tileset d'origine)\n" );
    std::vector<int> ids( w.collide.begin(), w.collide.end() );
    for ( size_t i = 0; i < ids.size(); i += 16 ) {
        fprintf( f, "collide" );
        for ( size_t j = i; j < ids.size() && j < i + 16; j++ ) fprintf( f, " %d", ids[j] );
        fputc( '\n', f );
    }
    for ( auto& kv : w.anim ) fprintf( f, "anim %d %d\n", kv.first, kv.second );
    fclose( f );
    return true;
}

bool loadLangFile( const std::string& path, std::map<std::string, std::string>& out )
{
    std::vector<std::string> lines;
    if ( !readLines( path, lines ) ) return false;
    for ( const std::string& s : lines ) {
        size_t eq = s.find( '=' );
        if ( eq == std::string::npos ) continue;
        auto trim = []( std::string x ) { size_t a = x.find_first_not_of( " \t" ), b = x.find_last_not_of( " \t" ); return a == std::string::npos ? std::string() : x.substr( a, b - a + 1 ); };
        out[trim( s.substr( 0, eq ) )] = trim( s.substr( eq + 1 ) );
    }
    return true;
}
