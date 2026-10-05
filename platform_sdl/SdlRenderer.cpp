#include "SdlRenderer.h"
#include <cstdio>

SdlRenderer::SdlRenderer( int zoom )
{
    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) { std::fprintf( stderr, "SDL_Init : %s\n", SDL_GetError() ); return; }
    window = SDL_CreateWindow( "Ethynd (build PC)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               kScreenW * zoom, kScreenH * zoom, SDL_WINDOW_SHOWN );
    if ( !window ) { std::fprintf( stderr, "SDL_CreateWindow : %s\n", SDL_GetError() ); return; }
    renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
    if ( !renderer ) renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_SOFTWARE );
    if ( !renderer ) { std::fprintf( stderr, "SDL_CreateRenderer : %s\n", SDL_GetError() ); return; }
    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "nearest" );              // pixels nets
    SDL_RenderSetLogicalSize( renderer, kScreenW, kScreenH );             // garde le 4:3 si la fenetre change
    // BGR565 SDL = bleu dans les bits hauts, rouge dans les bits bas : l'ordre natif de l'AKA
    texture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_BGR565, SDL_TEXTUREACCESS_STREAMING, kScreenW, kScreenH );
    if ( !texture ) std::fprintf( stderr, "SDL_CreateTexture : %s\n", SDL_GetError() );
}

SdlRenderer::~SdlRenderer()
{
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
    SDL_RenderCopy( renderer, texture, nullptr, nullptr );
    SDL_RenderPresent( renderer );
}
