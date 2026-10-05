// AkaInput -- IInput pour l'AKA (D-pad + A/B/C/D/L1/R1 via core/input.h), identique a Dark & Under.
#pragma once
#include "IInput.h"
#include "core/input.h"

class AkaInput : public IInput {
  public:
    void poll() override;
    const InputState& state() const override { return current; }
    bool justPressed( bool InputState::*button ) const override;
  private:
    InputState current, previous;
};
