#include "SdlInput.h"
#include "PlatformTime.h"

void SdlInput::poll()
{
    previous = current;
    muteToggle = false;
    SDL_Event e;
    while ( SDL_PollEvent( &e ) ) {
        if ( e.type == SDL_QUIT ) quit = true;
        if ( e.type == SDL_KEYDOWN && !e.key.repeat && e.key.keysym.scancode == SDL_SCANCODE_M ) muteToggle = true;
    }
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
    current.actionD = false;
    current.l1 = current.r1 = false;
    if ( k[SDL_SCANCODE_ESCAPE] ) quit = true;
}

bool SdlInput::justPressed( bool InputState::*b ) const { return ( current.*b ) && !( previous.*b ); }

uint32_t platformMillis() { return SDL_GetTicks(); }
