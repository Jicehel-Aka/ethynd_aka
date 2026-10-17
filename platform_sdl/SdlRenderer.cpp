#include "SdlRenderer.h"
#include <cstdio>

SdlRenderer::SdlRenderer( int zoom, int panelHeightPx ) : zoomFactor( zoom ), panelH( panelHeightPx )
{
    winW = kScreenW * zoom;
    winH = kScreenH * zoom + panelH;
    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) { std::fprintf( stderr, "SDL_Init : %s\n", SDL_GetError() ); return; }
    window = SDL_CreateWindow( "Ethynd (build PC)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               winW, winH, SDL_WINDOW_SHOWN );
    if ( !window ) { std::fprintf( stderr, "SDL_CreateWindow : %s\n", SDL_GetError() ); return; }
    renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
    if ( !renderer ) renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_SOFTWARE );
    if ( !renderer ) { std::fprintf( stderr, "SDL_CreateRenderer : %s\n", SDL_GetError() ); return; }
    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "nearest" );              // pixels nets
    if ( panelH == 0 ) SDL_RenderSetLogicalSize( renderer, kScreenW, kScreenH );    // sans panneau : garde le 4:3 ; avec : mise en page en pixels
    // BGR565 SDL = bleu dans les bits hauts, rouge dans les bits bas : l'ordre natif de l'AKA
    texture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_BGR565, SDL_TEXTUREACCESS_STREAMING, kScreenW, kScreenH );
    if ( !texture ) std::fprintf( stderr, "SDL_CreateTexture : %s\n", SDL_GetError() );
    if ( panelH > 0 ) {
        panelTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_BGR565, SDL_TEXTUREACCESS_STREAMING, winW, panelH );
        if ( !panelTexture ) std::fprintf( stderr, "SDL_CreateTexture (panneau) : %s\n", SDL_GetError() );
    }
}

SdlRenderer::~SdlRenderer()
{
    if ( panelTexture ) SDL_DestroyTexture( panelTexture );
    if ( texture ) SDL_DestroyTexture( texture );
    if ( renderer ) SDL_DestroyRenderer( renderer );
    if ( window ) SDL_DestroyWindow( window );
    SDL_Quit();
}

void SdlRenderer::present()
{
    if ( !ok() ) return;
    SDL_UpdateTexture( texture, nullptr, fb.data(), kScreenW * (int)sizeof( uint16_t ) );
    SDL_RenderClear( renderer );
    if ( panelH > 0 ) {
        SDL_Rect gameRect = { 0, 0, winW, gameHeightPx() };
        SDL_RenderCopy( renderer, texture, nullptr, &gameRect );
        if ( panelTexture && panelPixels ) {
            SDL_UpdateTexture( panelTexture, nullptr, panelPixels, winW * (int)sizeof( uint16_t ) );
            SDL_Rect panelRect = { 0, gameHeightPx(), winW, panelH };
            SDL_RenderCopy( renderer, panelTexture, nullptr, &panelRect );
        }
    } else {
        SDL_RenderCopy( renderer, texture, nullptr, nullptr );
    }
    SDL_RenderPresent( renderer );
}
