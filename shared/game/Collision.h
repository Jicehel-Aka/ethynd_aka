// Collision.h -- collisions pixel-perfect entre un rectangle et un masque de tuile.
// Tout est en pixels LOGIQUES (32 px par tuile), comme dans Ethynd.
#pragma once
#include <cstdint>

struct Rect { int x, y, w, h; };

inline bool rectsOverlap( const Rect& a, const Rect& b ) {
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

// Masque d'une tuile : 32 lignes de 4 octets (32 bits, pixel 0 = bit de poids fort du 1er octet).
constexpr int kMaskBytes = 128;

// true si au moins un pixel plein du masque se trouve dans le rectangle (rx, ry, rw, rh),
// exprime dans le repere de la tuile (0..31) ; le rectangle peut deborder de la tuile.
inline bool maskHitsRect( const uint8_t* mask, int rx, int ry, int rw, int rh ) {
    int x0 = rx < 0 ? 0 : rx;
    int y0 = ry < 0 ? 0 : ry;
    int x1 = rx + rw - 1; if ( x1 > 31 ) x1 = 31;
    int y1 = ry + rh - 1; if ( y1 > 31 ) y1 = 31;
    if ( x0 > x1 || y0 > y1 ) return false;
    // bits x0..x1 dans un mot de 32 bits (bit 31 = pixel 0)
    uint64_t cols = ( 0xFFFFFFFFull >> x0 ) & ~( 0xFFFFFFFFull >> ( x1 + 1 ) );
    for ( int y = y0; y <= y1; y++ ) {
        const uint8_t* r = mask + y * 4;
        uint32_t row = ( (uint32_t)r[0] << 24 ) | ( (uint32_t)r[1] << 16 ) | ( (uint32_t)r[2] << 8 ) | r[3];
        if ( row & (uint32_t)cols ) return true;
    }
    return false;
}

inline int floorDiv( int a, int b ) { int q = a / b; return ( a % b != 0 && ( a < 0 ) != ( b < 0 ) ) ? q - 1 : q; }
