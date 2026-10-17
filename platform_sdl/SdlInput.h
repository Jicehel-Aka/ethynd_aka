// SdlInput -- clavier PC -> InputState (meme role que AkaInput).
//   Deplacement : fleches ou ZQSD/WASD (scancodes : la position physique des touches)
#pragma once
#include <SDL2/SDL.h>
#include "IInput.h"
#include "VirtualPad.h"

class SdlInput : public IInput {
  public:
    void pump();                         // evenements SDL (fermeture, touche M) : a appeler a chaque tour de boucle
    void poll() override;                // echantillonne le clavier : a appeler A CHAQUE TICK (voir FixedStep.h)
    const InputState& state() const override { return current; }
    bool justPressed( bool InputState::*button ) const override;
    // boutons a l'ecran : hit(x, y) -> numero de bouton ou -1, en pixels DU PANNEAU ; (winW, winH) taille de la fenetre ;
    // panelTop : ordonnee du haut du panneau dans la fenetre. Les numeros suivent ControlPanel::Button.
    void attachPad( VirtualPad::HitFn hit, int panelTop, int winW, int winH ) { pad = new VirtualPad( std::move( hit ) ); top = panelTop; ww = winW; wh = winH; }
    ~SdlInput() { delete pad; }
    uint32_t virtualMask() const { return pad ? pad->mask() : 0; }
    bool quitRequested() const { return quit; }
    bool toggleMuteRequested() const { return muteToggle; }     // touche M, vrai un seul tour
  private:
    InputState current, previous;
    VirtualPad* pad = nullptr;
    int top = 0, ww = 0, wh = 0;
    bool quit = false;
    bool muteToggle = false;
};
