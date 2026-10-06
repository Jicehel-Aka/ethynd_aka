#include "World.h"
#include "ByteReader.h"
#include <cstdio>

bool WorldData::load( IAssetSource& src )
{
    std::vector<uint8_t> d;
    if ( !src.read( "world.bin", d ) ) return false;
    ByteReader r( d.data(), d.size() );
    if ( !r.tag( "EWLD" ) ) return false;
    if ( r.u16() != 1 ) { fprintf( stderr, "world.bin : version inconnue\n" ); return false; }
    startMap = r.u8(); startX = r.s16(); startY = r.s16(); startDir = (Dir)( r.u8() & 3 );
    int nm = r.u8();
    mapNames.clear();
    for ( int i = 0; i < nm; i++ ) mapNames.push_back( r.sstr() );
    flagCount = r.u8();
    int ni = r.u8();
    items.assign( ni, ItemDef() );
    for ( ItemDef& it : items ) { it.icon = r.sstr(); it.heal = r.u8(); it.name = r.sstr(); }
    int nd = r.u16();
    dialogues.assign( nd, Dialogue() );
    for ( Dialogue& dl : dialogues ) {
        int nl = r.u8();
        dl.lines.assign( nl, DlgLine() );
        for ( DlgLine& l : dl.lines ) { l.speaker = r.sstr(); l.text = r.sstr(); }
        int na = r.u8();
        for ( int a = 0; a < na; a++ ) { DlgAction ac; ac.type = r.u8(); ac.a = r.s16(); ac.b = r.s16(); dl.actions.push_back( ac ); }
    }
    int no = r.u8();
    objectives.assign( no, Objective() );
    for ( Objective& o : objectives ) {
        o.title = r.sstr();
        o.after = r.s8(); o.cond = r.u8(); o.s = r.sstr();
        for ( int k = 0; k < 4; k++ ) o.p[k] = r.s16();
    }
    if ( !r.ok() || startMap >= mapNames.size() ) { fprintf( stderr, "world.bin : fichier invalide\n" ); return false; }
    return true;
}

int WorldData::mapIndex( const char* name ) const
{
    for ( size_t i = 0; i < mapNames.size(); i++ ) if ( mapNames[i] == name ) return (int)i;
    return -1;
}
