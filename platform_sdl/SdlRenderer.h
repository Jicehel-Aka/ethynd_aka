// SdlRenderer -- build PC (Windows/Linux). Dessine dans le framebuffer logiciel de SoftRenderer
// (320x240, BGR565 natif AKA -> memes fichiers convertis que la console), puis l'envoie dans
// une texture SDL a l'echelle entiere choisie (zoom x2, x3 ou x4).
#pragma once
#include <SDL2/SDL.h>
#include "SoftRenderer.h"

class SdlRenderer : public SoftRenderer {
  public:
    explicit SdlRenderer( int zoom );
    ~SdlRenderer() override;
    bool ok() const { return window && renderer && texture; }
    void present() override;
  private:
    SDL_Window*   window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture*  texture = nullptr;
};
