// SoftRenderer.h -- renderer logiciel : framebuffer 320x240 dans l'ordre NATIF de l'AKA
// (BGR565 : rouge dans les bits bas, comme lcd_color_rgb de gb_ll_lcd.h). Les memes fichiers
// convertis servent donc sur la console, dans le test PC et dans la build SDL.
// Utilise par host_test (captures PPM) et par platform_sdl (SdlRenderer en derive).
#pragma once
#include <vector>
#include <cstdio>
#include <cstring>
#include "IRenderer.h"
#include "fonts/simple5x8_font.h"
#if defined( __has_include )
#  if __has_include( "aka_font/gb_text_render.h" )
#    include "aka_font/gb_text_render.h"
#    define SOFT_HAVE_AKA_FONT 1
#  endif
#endif

class SoftRenderer : public IRenderer {
  public:
    SoftRenderer() : fb( kScreenW * kScreenH, 0 ) {}
    struct Img { const uint16_t* px; uint16_t w, h; };
    std::vector<Img> images;
    std::vector<uint16_t> fb;                 // kScreenW * kScreenH, BGR565

    ImageId createImage( const uint16_t* p, uint16_t w, uint16_t h ) override {
        if ( images.size() >= kInvalidImageId ) return kInvalidImageId;
        images.push_back( { p, w, h } );
        return (ImageId)( images.size() - 1 );
    }
    void fillRect( int16_t x, int16_t y, int16_t w, int16_t h, RGBColor c ) override {
        uint16_t v = nativeColor( c.r, c.g, c.b );
        for ( int yy = y; yy < y + h; yy++ ) for ( int xx = x; xx < x + w; xx++ ) put( xx, yy, v );
    }
    void drawImage( int16_t x, int16_t y, ImageId id ) override {
        if ( id >= images.size() ) return;
        const Img& im = images[id];
        for ( int yy = 0; yy < im.h; yy++ ) {
            int dy = y + yy; if ( dy < 0 || dy >= kScreenH ) continue;
            for ( int xx = 0; xx < im.w; xx++ ) {
                uint16_t v = im.px[yy * im.w + xx];
                if ( v != kColorKey ) put( x + xx, dy, v );
            }
        }
    }
    void drawText( int16_t x, int16_t y, const char* t, RGBColor c, FontSize size ) override {
        uint16_t pen = nativeColor( c.r, c.g, c.b );
#ifdef SOFT_HAVE_AKA_FONT
        if ( size == FontSize::Wide ) {       // meme rendu UTF-8 + accents que sur la console
            gb_text::draw_utf8( x, y, t, [&]( int px, int py ) { put( px, py, pen ); } );
            return;
        }
#else
        (void)size;
#endif
        int cx = x;
        for ( const char* p = t; *p; ++p ) {  // police 5x8, 6 px par caractere
            uint8_t code = (uint8_t)*p;
            if ( code < 32 || code > 126 ) { cx += 6; continue; }
            const uint8_t* g = simple5x8_font[code - 32];
            for ( int dy = 0; dy < kSimple5x8Height; dy++ ) {
                uint8_t line = g[dy];
                for ( int dx = 0; dx < kSimple5x8Width; dx++, line >>= 1 )
                    if ( line & 1 ) put( cx + dx, y + dy, pen );
            }
            cx += 6;
        }
    }
    void getImageSize( ImageId id, int16_t& w, int16_t& h ) const override {
        if ( id >= images.size() ) { w = h = 0; return; }
        w = images[id].w; h = images[id].h;
    }
    void present() override {}

    bool savePPM( const char* path ) const {
        FILE* f = fopen( path, "wb" ); if ( !f ) return false;
        fprintf( f, "P6\n%d %d\n255\n", kScreenW, kScreenH );
        for ( uint16_t v : fb ) {
            uint8_t r5 = v & 31, g6 = ( v >> 5 ) & 63, b5 = v >> 11;
            uint8_t rgb[3] = { (uint8_t)( r5 << 3 | r5 >> 2 ), (uint8_t)( g6 << 2 | g6 >> 4 ), (uint8_t)( b5 << 3 | b5 >> 2 ) };
            fwrite( rgb, 1, 3, f );
        }
        fclose( f ); return true;
    }

  protected:
    static uint16_t nativeColor( uint8_t r, uint8_t g, uint8_t b ) { return ( r >> 3 ) | ( ( g >> 2 ) << 5 ) | ( ( b >> 3 ) << 11 ); }
    void put( int x, int y, uint16_t v ) { if ( x >= 0 && y >= 0 && x < kScreenW && y < kScreenH ) fb[y * kScreenW + x] = v; }
};
