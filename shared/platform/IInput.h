// IInput.h -- entrees, identique a Dark & Under (le jeu ne lit jamais les touches).
#pragma once

struct InputState {
    bool up = false, down = false, left = false, right = false;
    bool actionA = false;   // attaquer / valider
    bool actionB = false;   // retour
    bool actionC = false;   // aide (menu)
    bool actionD = false;
    bool l1 = false, r1 = false;
};

class IInput {
  public:
    virtual ~IInput() = default;
    virtual void poll() = 0;
    virtual const InputState& state() const = 0;
    virtual bool justPressed( bool InputState::*button ) const = 0;
};
