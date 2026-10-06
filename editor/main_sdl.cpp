// main_sdl.cpp -- fenetre SDL de l'editeur de cartes d'Ethynd (PC, Windows/Linux).
// Usage : ethynd_editor [--world <dossier world>] [--assets <dossier>] [--lang fr|en|...]
//                       [--size 960x600] [--build "<commande de reconstruction>"]
//   --assets : dossier contenant tileset_full.bin et sprites.bin (convert_assets.py --editor-out)
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "EditorCore.h"

static int mapKey( SDL_Keycode k )
{
    if ( k >= SDLK_a && k <= SDLK_z ) return 'a' + ( k - SDLK_a );
    if ( k >= SDLK_0 && k <= SDLK_9 ) return '0' + ( k - SDLK_0 );
    if ( k >= SDLK_F1 && k <= SDLK_F9 ) return ED_F1 + ( k - SDLK_F1 );
    switch ( k ) {
        case SDLK_TAB: return ED_TAB;       case SDLK_RETURN: case SDLK_KP_ENTER: return ED_ENTER;
        case SDLK_ESCAPE: return ED_ESC;    case SDLK_BACKSPACE: return ED_BACKSPACE;
        case SDLK_DELETE: return ED_DELETE; case SDLK_SPACE: return ED_SPACE;
        case SDLK_UP: return ED_UP;         case SDLK_DOWN: return ED_DOWN;
        case SDLK_LEFT: return ED_LEFT;     case SDLK_RIGHT: return ED_RIGHT;
        case SDLK_PAGEUP: return ED_PGUP;   case SDLK_PAGEDOWN: return ED_PGDN;
        case SDLK_COMMA: return ',';        case SDLK_PERIOD: return '.';
        default: return 0;
    }
}

int main( int argc, char** argv )
{
    std::string worldDir = "world", assetsDir = "editor_assets", lang = "fr", build;
    int W = 960, H = 600;
    for ( int i = 1; i < argc; i++ ) {
        if ( !strcmp( argv[i], "--world" ) && i + 1 < argc ) worldDir = argv[++i];
        else if ( !strcmp( argv[i], "--assets" ) && i + 1 < argc ) assetsDir = argv[++i];
        else if ( !strcmp( argv[i], "--lang" ) && i + 1 < argc ) lang = argv[++i];
        else if ( !strcmp( argv[i], "--build" ) && i + 1 < argc ) build = argv[++i];
        else if ( !strcmp( argv[i], "--size" ) && i + 1 < argc ) { if ( sscanf( argv[++i], "%dx%d", &W, &H ) != 2 ) { W = 960; H = 600; } }
        else fprintf( stderr, "argument ignore : %s\n", argv[i] );
    }
    EditorCore ed;
    std::string err;
    if ( !ed.open( worldDir, assetsDir, lang, err ) ) { fprintf( stderr, "Impossible d'ouvrir le monde : %s\n", err.c_str() ); return 1; }
    ed.setBuildCommand( build );
    ed.setViewSize( W, H );

    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) { fprintf( stderr, "SDL_Init : %s\n", SDL_GetError() ); return 1; }
    SDL_Window* win = SDL_CreateWindow( ed.tr( "ed.title" ).c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_SHOWN );
    SDL_Renderer* ren = win ? SDL_CreateRenderer( win, -1, SDL_RENDERER_ACCELERATED ) : nullptr;
    if ( win && !ren ) ren = SDL_CreateRenderer( win, -1, SDL_RENDERER_SOFTWARE );
    // BGR565 SDL = ordre natif du jeu (voir SdlRenderer) : les memes fichiers servent partout
    SDL_Texture* tex = ren ? SDL_CreateTexture( ren, SDL_PIXELFORMAT_BGR565, SDL_TEXTUREACCESS_STREAMING, W, H ) : nullptr;
    if ( !win || !ren || !tex ) { fprintf( stderr, "SDL : %s\n", SDL_GetError() ); return 1; }
    SDL_StartTextInput();

    SoftRenderer fb( W, H );
    bool running = true, warned = false, redraw = true;
    while ( running ) {
        SDL_Event se;
        if ( SDL_WaitEventTimeout( &se, 100 ) ) {
            do {
                EdEvent e; SDL_Keymod mod = SDL_GetModState();
                e.ctrl = ( mod & KMOD_CTRL ) != 0; e.shift = ( mod & KMOD_SHIFT ) != 0;
                switch ( se.type ) {
                    case SDL_QUIT:
                        if ( ed.dirty && !warned ) { warned = true; ed.status = ed.tr( "ed.status.unsaved" ); }
                        else running = false;
                        break;
                    case SDL_KEYDOWN: if ( !se.key.repeat || mapKey( se.key.keysym.sym ) >= ED_UP ) { e.type = EdEvent::KeyDown; e.key = mapKey( se.key.keysym.sym ); if ( e.key ) ed.handle( e ); } break;
                    case SDL_KEYUP: e.type = EdEvent::KeyUp; e.key = mapKey( se.key.keysym.sym ); ed.handle( e ); break;
                    case SDL_TEXTINPUT: e.type = EdEvent::Text; e.text = se.text.text; ed.handle( e ); break;
                    case SDL_MOUSEBUTTONDOWN: e.type = EdEvent::MouseDown; e.x = se.button.x; e.y = se.button.y; e.button = se.button.button; ed.handle( e ); break;
                    case SDL_MOUSEBUTTONUP: e.type = EdEvent::MouseUp; e.x = se.button.x; e.y = se.button.y; e.button = se.button.button; ed.handle( e ); break;
                    case SDL_MOUSEMOTION: e.type = EdEvent::MouseMove; e.x = se.motion.x; e.y = se.motion.y; ed.handle( e ); break;
                    case SDL_MOUSEWHEEL: { int mx, my; SDL_GetMouseState( &mx, &my ); e.type = EdEvent::Wheel; e.x = mx; e.y = my; e.dy = se.wheel.y; ed.handle( e ); break; }
                    default: break;
                }
                redraw = true;
            } while ( SDL_PollEvent( &se ) );
        }
        if ( redraw ) {
            ed.draw( fb );
            SDL_UpdateTexture( tex, nullptr, fb.fb.data(), W * (int)sizeof( uint16_t ) );
            SDL_RenderClear( ren ); SDL_RenderCopy( ren, tex, nullptr, nullptr ); SDL_RenderPresent( ren );
            redraw = false;
        }
    }
    SDL_DestroyTexture( tex ); SDL_DestroyRenderer( ren ); SDL_DestroyWindow( win ); SDL_Quit();
    return 0;
}
