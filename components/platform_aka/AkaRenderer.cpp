#include "AkaRenderer.h"
#include "fonts/simple5x8_font.h"
#include "aka_font/gb_text_render.h"

static constexpr int16_t kNarrowCharAdvance = 6;   // 5 px de glyphe + 1 px d'espace

ImageId AkaRenderer::createImage( const uint16_t* pixels, uint16_t w, uint16_t h )
{
    if ( assets.size() >= kInvalidImageId ) return kInvalidImageId;
    assets.push_back( { pixels, w, h } );
    return (ImageId)( assets.size() - 1 );
}

void AkaRenderer::fillRect( int16_t x, int16_t y, int16_t w, int16_t h, RGBColor c )
{
    uint16_t color = gfx.makeColor( c.r, c.g, c.b );
    if ( x <= 0 && y <= 0 && w >= kScreenW && h >= kScreenH ) { gfx.clear( color ); return; }   // fond plein ecran
    gfx.setColor( color );
    gfx.fillRect( x, y, w, h );
}

void AkaRenderer::drawImage( int16_t x, int16_t y, ImageId id )
{
    if ( id >= assets.size() ) return;
    const Asset& a = assets[id];
    if ( a.w == kScreenW && a.h == kScreenH )          // ecrans de menu : opaques, copie rapide
        gfx.drawImage( x, y, a.pixels, a.w, a.h );
    else
        gfx.drawImage( x, y, a.pixels, a.w, a.h, kColorKey );
}

void AkaRenderer::drawText( int16_t x, int16_t y, const char* text, RGBColor c, FontSize size )
{
    uint16_t pen = gfx.makeColor( c.r, c.g, c.b );
    if ( size == FontSize::Wide ) {                    // 8x8 UTF-8 avec accents (composant aka_font)
        gb_text::draw_utf8( x, y, text, [&]( int px, int py ) { gfx.drawPixel( px, py, pen ); } );
        return;
    }
    int16_t cx = x;
    for ( const char* p = text; *p; ++p ) {
        uint8_t code = (uint8_t)*p;
        if ( code < 32 || code > 126 ) { cx += kNarrowCharAdvance; continue; }
        const uint8_t* glyph = simple5x8_font[code - 32];
        for ( uint8_t dy = 0; dy < kSimple5x8Height; ++dy ) {
            uint8_t line = glyph[dy];
            for ( uint8_t dx = 0; dx < kSimple5x8Width; ++dx ) {
                if ( line & 1 ) gfx.drawPixel( cx + dx, y + dy, pen );
                line >>= 1;
            }
        }
        cx += kNarrowCharAdvance;
    }
}

void AkaRenderer::getImageSize( ImageId id, int16_t& w, int16_t& h ) const
{
    if ( id >= assets.size() ) { w = h = 0; return; }
    w = (int16_t)assets[id].w;
    h = (int16_t)assets[id].h;
}

void AkaRenderer::present() { gfx.update(); }
