#include "GameMap.h"
#include <cstring>

bool GameMap::load( const AssetStore& a, IAssetSource& src, const char* name, int cx, int cy )
{
    assets = &a;
    strncpy( mapName, name, sizeof mapName - 1 );
    camX = cx; camY = cy; counter = 0;
    if ( !a.loadMap( src, name, data ) ) return false;
    // Comme Ethynd (charger_hitboxs) : les collisions sont fixees au chargement, d'apres les
    // tuiles de depart ; l'animation d'une tuile ne change pas son masque.
    for ( int l = 0; l < 3; l++ ) {
        coll[l].assign( (size_t)data.w * data.h, kNoTile );
        for ( size_t i = 0; i < coll[l].size(); i++ ) {
            uint16_t t = data.layer[l][i];
            if ( t == kNoTile ) continue;
            const uint8_t* m = a.tileMask( t );
            for ( int b = 0; b < kMaskBytes; b++ ) if ( m[b] ) { coll[l][i] = t; break; }
        }
    }
    return true;
}

void GameMap::update()
{
    if ( counter < kTileAnimTicks ) { counter++; return; }
    counter = 0;
    for ( int l = 0; l < 4; l++ )
        for ( uint16_t& t : data.layer[l] )
            if ( t != kNoTile ) {
                uint16_t n = assets->tileNext( t );
                if ( n != kNoTile ) t = n;
            }
}

bool GameMap::collidesWorld( const Rect& r ) const
{
    int tx0 = floorDiv( r.x, kTileLogic ), tx1 = floorDiv( r.x + r.w - 1, kTileLogic );
    int ty0 = floorDiv( r.y, kTileLogic ), ty1 = floorDiv( r.y + r.h - 1, kTileLogic );
    if ( tx0 < 0 ) tx0 = 0;
    if ( ty0 < 0 ) ty0 = 0;
    if ( tx1 >= data.w ) tx1 = data.w - 1;
    if ( ty1 >= data.h ) ty1 = data.h - 1;
    for ( int ty = ty0; ty <= ty1; ty++ )
        for ( int tx = tx0; tx <= tx1; tx++ )
            for ( int l = 0; l < 3; l++ ) {
                uint16_t t = coll[l][(size_t)ty * data.w + tx];
                if ( t != kNoTile && maskHitsRect( assets->tileMask( t ), r.x - tx * kTileLogic, r.y - ty * kTileLogic, r.w, r.h ) )
                    return true;
            }
    return false;
}

bool GameMap::collidesScreen( const Rect& s, int cx, int cy ) const
{
    return collidesWorld( Rect{ s.x - cx, s.y - cy, s.w, s.h } );
}

void GameMap::drawLayers( IRenderer& r, int first, int last ) const
{
    const int ox = originX(), oy = originY();
    int tx0 = floorDiv( -ox, kTileShow ), tx1 = floorDiv( kScreenW - 1 - ox, kTileShow );
    int ty0 = floorDiv( -oy, kTileShow ), ty1 = floorDiv( kScreenH - 1 - oy, kTileShow );
    if ( tx0 < 0 ) tx0 = 0;
    if ( ty0 < 0 ) ty0 = 0;
    if ( tx1 >= data.w ) tx1 = data.w - 1;
    if ( ty1 >= data.h ) ty1 = data.h - 1;
    for ( int ty = ty0; ty <= ty1; ty++ )
        for ( int tx = tx0; tx <= tx1; tx++ )
            for ( int l = first; l <= last; l++ ) {
                uint16_t t = data.layer[l][(size_t)ty * data.w + tx];
                if ( t != kNoTile )
                    r.drawImage( ox + tx * kTileShow, oy + ty * kTileShow, assets->tileImage( t ) );
            }
}

void GameMap::drawBackground( IRenderer& r ) const
{
    r.fillRect( 0, 0, kScreenW, kScreenH, RGBColor{ 32, 23, 41 } );
    drawLayers( r, 0, 2 );
}

void GameMap::drawForeground( IRenderer& r ) const { drawLayers( r, 3, 3 ); }
