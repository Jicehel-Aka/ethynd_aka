// SdlInput -- clavier PC -> InputState (meme role que AkaInput).
//   Deplacement : fleches ou ZQSD/WASD (scancodes : la position physique des touches)
#pragma once
#include <SDL2/SDL.h>
#include "IInput.h"

class SdlInput : public IInput {
  public:
    void poll() override;
    const InputState& state() const override { return current; }
    bool justPressed( bool InputState::*button ) const override;
    bool quitRequested() const { return quit; }
    bool toggleMuteRequested() const { return muteToggle; }     // touche M, vrai un seul tour
  private:
    InputState current, previous;
    bool quit = false;
    bool muteToggle = false;
};
