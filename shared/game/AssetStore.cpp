#include "AssetStore.h"
#include "Config.h"
#include <cstring>
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
    static const char* const menuFiles[MENU_COUNT] = { "menu/menu.raw", "menu/aide.raw", "menu/chargement.raw", "menu/mort.raw" };
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
    if ( !src.read( path, d ) || d.size() < 12 || memcmp( d.data(), "EMAP", 4 ) ) return false;
    int w = rd16( &d[4] ), h = rd16( &d[6] ), layers = rd16( &d[8] );
    if ( layers != 4 || d.size() < 12 + (size_t)4 * w * h * 2 ) return false;
    out.w = w; out.h = h;
    for ( int l = 0; l < 4; l++ ) {
        out.layer[l].resize( (size_t)w * h );
        memcpy( out.layer[l].data(), &d[12 + (size_t)l * w * h * 2], (size_t)w * h * 2 );
    }
    return true;
}
