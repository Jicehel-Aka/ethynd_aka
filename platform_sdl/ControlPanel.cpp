#include "ControlPanel.h"
#include <algorithm>
#include <cmath>
#include <vector>

static const RGBColor kBg      = { 22, 24, 32 };
static const RGBColor kBar     = { 34, 38, 52 };
static const RGBColor kAccent  = { 148, 191, 167 };     // vert des textes du jeu
static const RGBColor kText    = { 214, 226, 218 };
static const RGBColor kDim     = { 128, 140, 134 };
static const RGBColor kFace    = { 58, 64, 86 };
static const RGBColor kFaceHi  = { 78, 86, 112 };
static const RGBColor kRim     = { 104, 114, 146 };
static const RGBColor kShadow  = { 10, 11, 16 };
static const RGBColor kDark    = { 20, 24, 30 };

static int utf8Len( const std::string& s ) { int n = 0; for ( unsigned char c : s ) if ( ( c & 0xC0 ) != 0x80 ) n++; return n; }

ControlPanel::ControlPanel( int z ) : SoftRenderer( 320 * z, kHeightUnits * kUnit ), zoom( z ), widthUnits( 320 * z / kUnit )
{
    dpadX = u( 52 ); dpadY = u( 56 );
    btnR = u( 14 ); btnY = u( 46 );
    const int right = u( widthUnits - 34 );
    const int step = u( std::min( 56, ( widthUnits - 34 - 118 ) / 3 ) );
    for ( int i = 0; i < 4; i++ ) btnX[i] = right - ( 3 - i ) * step;      // A B C D de gauche a droite
}

void ControlPanel::centerOf( int b, int& cx, int& cy ) const
{
    if ( b >= BTN_A ) { cx = btnX[b - BTN_A]; cy = btnY; return; }
    cx = dpadX; cy = dpadY;
    const int reach = u( 17 );
    if ( b == BTN_UP ) cy -= reach; else if ( b == BTN_DOWN ) cy += reach; else if ( b == BTN_LEFT ) cx -= reach; else cx += reach;
}

int ControlPanel::hit( int x, int y ) const
{
    for ( int b = BTN_A; b <= BTN_D; b++ ) {
        int cx, cy; centerOf( b, cx, cy );
        const int r = btnR + u( 3 );
        if ( ( x - cx ) * ( x - cx ) + ( y - cy ) * ( y - cy ) <= r * r ) return b;
    }
    const int dx = x - dpadX, dy = y - dpadY, reach = u( 27 );
    if ( std::abs( dx ) <= reach && std::abs( dy ) <= reach && std::max( std::abs( dx ), std::abs( dy ) ) >= u( 5 ) )
        return std::abs( dx ) > std::abs( dy ) ? ( dx < 0 ? BTN_LEFT : BTN_RIGHT ) : ( dy < 0 ? BTN_UP : BTN_DOWN );
    return -1;
}

// ------------------------------------------------------------------ primitives
void ControlPanel::circle( int cx, int cy, int r, RGBColor c )
{
    for ( int y = -r; y <= r; y++ ) {
        int half = (int)std::lround( std::sqrt( (double)( r * r - y * y ) ) );
        fillRect( (int16_t)( cx - half ), (int16_t)( cy + y ), (int16_t)( 2 * half + 1 ), 1, c );
    }
}

void ControlPanel::triangle( int x0, int y0, int x1, int y1, int x2, int y2, RGBColor c )
{
    const int ymin = std::min( { y0, y1, y2 } ), ymax = std::max( { y0, y1, y2 } );
    for ( int y = ymin; y <= ymax; y++ ) {
        int xl = 1 << 30, xr = -( 1 << 30 );
        const int xs[3][2] = { { x0, y0 }, { x1, y1 }, { x2, y2 } };
        for ( int i = 0; i < 3; i++ ) {
            int ax = xs[i][0], ay = xs[i][1], bx = xs[( i + 1 ) % 3][0], by = xs[( i + 1 ) % 3][1];
            if ( ( y < std::min( ay, by ) ) || ( y > std::max( ay, by ) ) || ay == by ) continue;
            int x = ax + ( bx - ax ) * ( y - ay ) / ( by - ay );
            xl = std::min( xl, x ); xr = std::max( xr, x );
        }
        if ( xl <= xr ) fillRect( (int16_t)xl, (int16_t)y, (int16_t)( xr - xl + 1 ), 1, c );
    }
}

void ControlPanel::roundButton( int cx, int cy, int r, bool pressed, const char* letter )
{
    const int off = pressed ? 1 : 3;                       // enfonce : l'ombre disparait, le bouton descend
    if ( !pressed ) circle( cx, cy + off, r, kShadow );
    circle( cx, cy + ( pressed ? 2 : 0 ), r, kRim );
    circle( cx, cy + ( pressed ? 2 : 0 ), r - u( 1 ), pressed ? kAccent : kFace );
    if ( !pressed ) circle( cx - u( 2 ), cy - u( 3 ), r - u( 7 ), kFaceHi );            // reflet
    drawText( (int16_t)( cx - 8 ), (int16_t)( cy + ( pressed ? 2 : 0 ) - 8 ), letter, pressed ? kDark : kText, FontSize::Wide, 2 );
}

void ControlPanel::dpad( const InputState& s )
{
    const int arm = u( 9 ), reach = u( 24 );
    const bool on[4] = { s.up, s.down, s.left, s.right };
    // ombre puis plaque de la croix
    fillRect( (int16_t)( dpadX - reach ), (int16_t)( dpadY - arm + 3 ), (int16_t)( 2 * reach ), (int16_t)( 2 * arm ), kShadow );
    fillRect( (int16_t)( dpadX - arm ), (int16_t)( dpadY - reach + 3 ), (int16_t)( 2 * arm ), (int16_t)( 2 * reach ), kShadow );
    fillRect( (int16_t)( dpadX - reach ), (int16_t)( dpadY - arm ), (int16_t)( 2 * reach ), (int16_t)( 2 * arm ), kRim );
    fillRect( (int16_t)( dpadX - arm ), (int16_t)( dpadY - reach ), (int16_t)( 2 * arm ), (int16_t)( 2 * reach ), kRim );
    const int in = 2;                                       // liseré
    fillRect( (int16_t)( dpadX - reach + in ), (int16_t)( dpadY - arm + in ), (int16_t)( 2 * reach - 2 * in ), (int16_t)( 2 * arm - 2 * in ), kFace );
    fillRect( (int16_t)( dpadX - arm + in ), (int16_t)( dpadY - reach + in ), (int16_t)( 2 * arm - 2 * in ), (int16_t)( 2 * reach - 2 * in ), kFace );
    // bras allumes
    if ( on[BTN_UP] )    fillRect( (int16_t)( dpadX - arm + in ), (int16_t)( dpadY - reach + in ), (int16_t)( 2 * arm - 2 * in ), (int16_t)( reach - arm ), kAccent );
    if ( on[BTN_DOWN] )  fillRect( (int16_t)( dpadX - arm + in ), (int16_t)( dpadY + arm ), (int16_t)( 2 * arm - 2 * in ), (int16_t)( reach - arm - in ), kAccent );
    if ( on[BTN_LEFT] )  fillRect( (int16_t)( dpadX - reach + in ), (int16_t)( dpadY - arm + in ), (int16_t)( reach - arm ), (int16_t)( 2 * arm - 2 * in ), kAccent );
    if ( on[BTN_RIGHT] ) fillRect( (int16_t)( dpadX + arm ), (int16_t)( dpadY - arm + in ), (int16_t)( reach - arm - in ), (int16_t)( 2 * arm - 2 * in ), kAccent );
    // fleches
    const int t = u( 5 ), d = u( 17 );
    triangle( dpadX, dpadY - d - t, dpadX - t, dpadY - d + t, dpadX + t, dpadY - d + t, on[BTN_UP] ? kDark : kText );
    triangle( dpadX, dpadY + d + t, dpadX - t, dpadY + d - t, dpadX + t, dpadY + d - t, on[BTN_DOWN] ? kDark : kText );
    triangle( dpadX - d - t, dpadY, dpadX - d + t, dpadY - t, dpadX - d + t, dpadY + t, on[BTN_LEFT] ? kDark : kText );
    triangle( dpadX + d + t, dpadY, dpadX + d - t, dpadY - t, dpadX + d - t, dpadY + t, on[BTN_RIGHT] ? kDark : kText );
    fillRect( (int16_t)( dpadX - u( 3 ) ), (int16_t)( dpadY - u( 3 ) ), (int16_t)u( 6 ), (int16_t)u( 6 ), kRim );             // centre
}

// ------------------------------------------------------------------ texte
int ControlPanel::wrapped( int x, int y, int widthPx, const std::string& text, RGBColor c )
{
    const int cols = std::max( 1, widthPx / 8 );
    std::vector<std::string> rows;
    std::string line, word;
    auto flush = [&]() {
        if ( word.empty() ) return;
        if ( !line.empty() && utf8Len( line ) + 1 + utf8Len( word ) > cols ) { rows.push_back( line ); line.clear(); }
        if ( !line.empty() ) line += ' ';
        line += word; word.clear();
    };
    for ( char ch : text ) { if ( ch == ' ' ) flush(); else word += ch; }
    flush();
    if ( !line.empty() ) rows.push_back( line );
    for ( size_t i = 0; i < rows.size(); i++ ) drawText( (int16_t)x, (int16_t)( y + (int)i * 10 ), rows[i].c_str(), c );
    return (int)rows.size();
}

// legende centree sous un bouton : les touches (couleur d'accent) puis ce qu'elles font
void ControlPanel::caption( int cx, int y, int widthPx, const std::string& keys, const std::string& role )
{
    auto centered = [&]( int yy, const std::string& t, RGBColor c ) {
        const int cols = std::max( 1, widthPx / 8 );
        std::vector<std::string> rows; std::string line, word;
        auto flush = [&]() { if ( word.empty() ) return; if ( !line.empty() && utf8Len( line ) + 1 + utf8Len( word ) > cols ) { rows.push_back( line ); line.clear(); } if ( !line.empty() ) line += ' '; line += word; word.clear(); };
        for ( char ch : t ) { if ( ch == ' ' ) flush(); else word += ch; }
        flush(); if ( !line.empty() ) rows.push_back( line );
        for ( size_t i = 0; i < rows.size(); i++ ) drawText( (int16_t)( cx - utf8Len( rows[i] ) * 4 ), (int16_t)( yy + (int)i * 10 ), rows[i].c_str(), c );
        return (int)rows.size();
    };
    int n = centered( y, keys, kAccent );
    centered( y + n * 10 + 2, role, kDim );
}

void ControlPanel::draw( const InputState& s, const std::function<std::string( const std::string& )>& tr )
{
    const int pw = 320 * zoom, ph = height();
    fillRect( 0, 0, (int16_t)pw, (int16_t)ph, kBg );
    // barre de titre : nom du jeu a gauche, "Version PC" a droite
    fillRect( 0, 0, (int16_t)pw, (int16_t)u( 20 ), kBar );
    fillRect( 0, (int16_t)u( 20 ), (int16_t)pw, 2, kAccent );
    drawText( (int16_t)u( 8 ), (int16_t)( u( 10 ) - 8 ), "ETHYND", kAccent, FontSize::Wide, 2 );
    const std::string sub = tr( "pc.subtitle" );
    drawText( (int16_t)( pw - u( 8 ) - utf8Len( sub ) * 8 ), (int16_t)( u( 10 ) - 4 ), sub.c_str(), kDim );

    // croix + boutons A B C D
    dpad( s );
    const bool on[4] = { s.actionA, s.actionB, s.actionC, s.actionD };
    static const char* const letters[4] = { "A", "B", "C", "D" };
    static const char* const ids[4] = { "a", "b", "c", "d" };
    const int colW = u( 52 ) - 4;
    for ( int i = 0; i < 4; i++ ) {
        roundButton( btnX[i], btnY, btnR, on[i], letters[i] );
        caption( btnX[i], btnY + btnR + u( 5 ), colW, tr( std::string( "pc." ) + ids[i] + ".keys" ), tr( std::string( "pc." ) + ids[i] + ".role" ) );
    }
    caption( dpadX, dpadY + u( 24 ) + u( 6 ), u( 70 ), tr( "pc.move.keys" ), tr( "pc.move.role" ) );

    // pied : touches generales
    fillRect( 0, (int16_t)u( 106 ), (int16_t)pw, 1, kBar );
    const std::string misc = tr( "pc.misc" );
    drawText( (int16_t)( ( pw - utf8Len( misc ) * 8 ) / 2 ), (int16_t)( u( 113 ) - 4 ), misc.c_str(), kDim );
}
