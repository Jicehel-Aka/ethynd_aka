#include "SdlInput.h"
#include "PlatformTime.h"

void SdlInput::pump()
{
    muteToggle = false;
    SDL_Event e;
    while ( SDL_PollEvent( &e ) ) {
        if ( e.type == SDL_QUIT ) quit = true;
        if ( e.type == SDL_KEYDOWN && !e.key.repeat && e.key.keysym.scancode == SDL_SCANCODE_M ) muteToggle = true;
        if ( !pad ) continue;
        // Boutons a l'ecran. Les evenements souris synthetises par un ecran tactile sont ignores (le doigt est traite a part).
        switch ( e.type ) {
            case SDL_MOUSEBUTTONDOWN: if ( e.button.button == SDL_BUTTON_LEFT && e.button.which != SDL_TOUCH_MOUSEID ) pad->down( -1, e.button.x, e.button.y - top ); break;
            case SDL_MOUSEMOTION:     if ( ( e.motion.state & SDL_BUTTON_LMASK ) && e.motion.which != SDL_TOUCH_MOUSEID ) pad->move( -1, e.motion.x, e.motion.y - top ); break;
            case SDL_MOUSEBUTTONUP:   if ( e.button.button == SDL_BUTTON_LEFT && e.button.which != SDL_TOUCH_MOUSEID ) pad->up( -1 ); break;
            case SDL_FINGERDOWN:      pad->down( (int64_t)e.tfinger.fingerId, (int)( e.tfinger.x * ww ), (int)( e.tfinger.y * wh ) - top ); break;
            case SDL_FINGERMOTION:    pad->move( (int64_t)e.tfinger.fingerId, (int)( e.tfinger.x * ww ), (int)( e.tfinger.y * wh ) - top ); break;
            case SDL_FINGERUP:        pad->up( (int64_t)e.tfinger.fingerId ); break;
            case SDL_WINDOWEVENT:     if ( e.window.event == SDL_WINDOWEVENT_FOCUS_LOST ) pad->clear(); break;      // pas de bouton "colle"
            default: break;
        }
    }
}

void SdlInput::poll()
{
    previous = current;
    const Uint8* k = SDL_GetKeyboardState( nullptr );
    // Scancodes = position PHYSIQUE : W/A/S/D designent les touches Z/Q/S/D d'un clavier AZERTY.
    current.up    = k[SDL_SCANCODE_UP]    || k[SDL_SCANCODE_W];
    current.down  = k[SDL_SCANCODE_DOWN]  || k[SDL_SCANCODE_S];
    current.left  = k[SDL_SCANCODE_LEFT]  || k[SDL_SCANCODE_A];
    current.right = k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D];
    // Boutons : comme Ethynd, X attaque ; Espace et Entree aussi.
    current.actionA = k[SDL_SCANCODE_X] || k[SDL_SCANCODE_SPACE] || k[SDL_SCANCODE_RETURN];
    current.actionB = k[SDL_SCANCODE_BACKSPACE] || k[SDL_SCANCODE_B];
    current.actionC = k[SDL_SCANCODE_C] || k[SDL_SCANCODE_H];            // aide (menu)
    current.actionD = k[SDL_SCANCODE_F3] || k[SDL_SCANCODE_TAB];            // diagnostic (FPS)
    current.l1 = current.r1 = false;
    if ( pad ) {                                                          // boutons a l'ecran : meme effet qu'une touche
        const uint32_t m = pad->mask();
        current.up |= ( m >> 0 ) & 1;  current.down |= ( m >> 1 ) & 1;  current.left |= ( m >> 2 ) & 1;  current.right |= ( m >> 3 ) & 1;
        current.actionA |= ( m >> 4 ) & 1;  current.actionB |= ( m >> 5 ) & 1;  current.actionC |= ( m >> 6 ) & 1;  current.actionD |= ( m >> 7 ) & 1;
    }
    if ( k[SDL_SCANCODE_ESCAPE] ) quit = true;
}

bool SdlInput::justPressed( bool InputState::*b ) const { return ( current.*b ) && !( previous.*b ); }

uint32_t platformMillis() { return SDL_GetTicks(); }
