#include "AssetStore.h"
#include "Config.h"
#include <cstring>
#include "ByteReader.h"
#include <cstdio>

static uint16_t rd16( const uint8_t* p ) { uint16_t v; memcpy( &v, p, 2 ); return v; }
static uint32_t rd32( const uint8_t* p ) { uint32_t v; memcpy( &v, p, 4 ); return v; }

bool AssetStore::load( IAssetSource& src, IRenderer& r )
{
    // ---- tuiles ("ETIL", u16 tile, u16 count, u32 0, puis count*tile*tile RGB565)
    if ( !src.read( "tiles.bin", tilesBlob ) || tilesBlob.size() < 12 || memcmp( tilesBlob.data(), "ETIL", 4 ) ) return false;
    int tilePx = rd16( &tilesBlob[4] ), count = rd16( &tilesBlob[6] );
    if ( tilePx != kTileShow ) { printf( "tiles.bin : tuiles de %d px, %d attendus\n", tilePx, kTileShow ); return false; }
    size_t tileBytes = (size_t)tilePx * tilePx * 2;
    if ( tilesBlob.size() < 12 + count * tileBytes ) return false;
    tileIds.resize( count );
    for ( int i = 0; i < count; i++ )
        tileIds[i] = r.createImage( (const uint16_t*)( tilesBlob.data() + 12 + i * tileBytes ), tilePx, tilePx );

    // ---- masques de collision ("ECOL", u16 32, u16 count, u32 128, puis count*128 octets)
    if ( !src.read( "collide.bin", collide ) || collide.size() < 12 + (size_t)count * kMaskBytes || memcmp( collide.data(), "ECOL", 4 ) ) return false;

    // ---- animations de tuiles ("EANI", u32 count, u16[count])
    if ( !src.read( "anim.bin", animBlob ) || animBlob.size() < 8 || memcmp( animBlob.data(), "EANI", 4 ) ) return false;
    animNext.resize( count );
    for ( int i = 0; i < count; i++ ) animNext[i] = rd16( &animBlob[8 + i * 2] );

    // ---- sprites ("ESPR", u32 n, n * { char[24], u16 w, u16 h, u32 offset }, puis les pixels)
    if ( !src.read( "sprites.bin", spritesBlob ) || spritesBlob.size() < 8 || memcmp( spritesBlob.data(), "ESPR", 4 ) ) return false;
    uint32_t n = rd32( &spritesBlob[4] );
    size_t blob = 8 + (size_t)n * 32;
    for ( uint32_t i = 0; i < n; i++ ) {
        const uint8_t* e = &spritesBlob[8 + i * 32];
        uint16_t w = rd16( e + 24 ), h = rd16( e + 26 );
        uint32_t off = rd32( e + 28 );
        SpriteEntry se; memset( &se, 0, sizeof se ); memcpy( se.name, e, 23 );
        se.id = r.createImage( (const uint16_t*)( spritesBlob.data() + blob + off ), w, h );
        sprites.push_back( se );
    }

    // ---- ecrans 320x240 (menu/<nom>.raw, RGB565 sans en-tete)
    static const char* const menuFiles[MENU_COUNT] = { "menu/menu.raw", "menu/aide.raw", "menu/chargement.raw", "menu/mort.raw", "menu/fin.raw" };
    for ( int m = 0; m < MENU_COUNT; m++ ) {
        if ( !src.read( menuFiles[m], menuBlob[m] ) || menuBlob[m].size() < 320 * 240 * 2 ) return false;
        menus[m] = r.createImage( (const uint16_t*)menuBlob[m].data(), 320, 240 );
    }
    return true;
}

ImageId AssetStore::sprite( const char* name ) const
{
    for ( const SpriteEntry& s : sprites )
        if ( !strncmp( s.name, name, 23 ) ) return s.id;
    return kInvalidImageId;
}

bool AssetStore::loadMap( IAssetSource& src, const char* name, MapData& out ) const
{
    char path[64];
    snprintf( path, sizeof path, "maps/%s.map", name );
    std::vector<uint8_t> d;
    if ( !src.read( path, d ) ) return false;
    ByteReader r( d.data(), d.size() );
    if ( !r.tag( "EMA2" ) ) { fprintf( stderr, "%s : format de carte inconnu\n", path ); return false; }
    int w = r.u16(), h = r.u16(), layers = r.u16();
    r.u16();                                              // taille de tuile d'affichage (deja verifiee)
    if ( layers != 4 || !r.ok() ) return false;
    out = MapData();
    out.w = w; out.h = h;
    out.objects.music = r.u8(); r.u8();
    for ( int l = 0; l < 4; l++ ) {
        out.layer[l].resize( (size_t)w * h );
        for ( uint16_t& t : out.layer[l] ) t = r.u16();
    }
    int n = r.u16();
    for ( int i = 0; i < n; i++ ) {
        SpawnObj s; s.type = r.sstr(); s.x = r.s16(); s.y = r.s16(); s.vie = r.s16(); s.attaque = r.s16();
        out.objects.spawns.push_back( s );
    }
    n = r.u16();
    for ( int i = 0; i < n; i++ ) {
        NpcObj o; o.type = r.sstr(); o.x = r.s16(); o.y = r.s16(); o.dir = (Dir)( r.u8() & 3 );
        int nr = r.u8();
        for ( int k = 0; k < nr; k++ ) { TalkRule t; t.kind = r.u8(); t.arg = r.s16(); t.dialogue = r.s16(); o.rules.push_back( t ); }
        out.objects.npcs.push_back( o );
    }
    n = r.u16();
    for ( int i = 0; i < n; i++ ) {
        DoorObj o; int x = r.s16(), y = r.s16(), dw = r.s16(), dh = r.s16();
        o.r = Rect{ x, y, dw, dh }; o.destMap = r.u8(); o.dx = r.s16(); o.dy = r.s16(); o.needFlag = r.s16(); o.deny = r.s16();
        out.objects.doors.push_back( o );
    }
    n = r.u16();
    for ( int i = 0; i < n; i++ ) { ItemObj o; o.def = r.u8(); o.x = r.s16(); o.y = r.s16(); out.objects.items.push_back( o ); }
    return r.ok();
}
