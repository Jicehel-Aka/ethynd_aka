// SdlRenderer -- build PC (Windows/Linux). Dessine dans le framebuffer logiciel de SoftRenderer
// (320x240, BGR565 natif AKA -> memes fichiers convertis que la console), puis l'envoie dans
// une texture SDL a l'echelle entiere choisie (zoom x2, x3 ou x4). Un panneau de boutons peut etre ajoute SOUS la
// zone de jeu (ControlPanel) : la fenetre fait alors 320*zoom x (240*zoom + hauteur du panneau) pixels.
#pragma once
#include <SDL2/SDL.h>
#include "SoftRenderer.h"

class SdlRenderer : public SoftRenderer {
  public:
    explicit SdlRenderer( int zoom, int panelHeightPx = 0 );
    ~SdlRenderer() override;
    bool ok() const { return window && renderer && texture; }
    void present() override;
    // pixels du panneau (BGR565, 320*zoom x panelHeightPx), relus a chaque present() ; le tampon appartient a l'appelant
    void attachPanel( const uint16_t* pixels ) { panelPixels = pixels; }
    int windowWidth() const { return winW; }
    int windowHeight() const { return winH; }
    int gameHeightPx() const { return kScreenH * zoomFactor; }          // le panneau commence a cette ordonnee
  private:
    int zoomFactor = 1, panelH = 0, winW = 0, winH = 0;
    const uint16_t* panelPixels = nullptr;
    SDL_Texture*  panelTexture = nullptr;
    SDL_Window*   window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture*  texture = nullptr;
};
