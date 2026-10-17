// ControlPanel.h -- panneau sous la zone de jeu (version PC) : nom du jeu, boutons dessinés (croix + A B C D) qui
// s'allument quand la touche correspondante est pressée (clavier, souris ou doigt), et rappel des touches PC.
// Tous les textes viennent des fichiers de langue (clés pc.*) : le panneau suit la langue du jeu.
#pragma once
#include <functional>
#include <string>
#include "SoftRenderer.h"
#include "IInput.h"

class ControlPanel : public SoftRenderer {
  public:
    enum Button { BTN_UP = 0, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B, BTN_C, BTN_D, BTN_COUNT };
    static constexpr int kUnit = 2;                // pixels par unite de mise en page (le panneau est dessine en pixels natifs)
    static constexpr int kHeightUnits = 120;

    explicit ControlPanel( int zoom );             // largeur = 320 * zoom pixels
    int width() const { return W; }
    int height() const { return kHeightUnits * kUnit; }
    // translate : cle de traduction -> texte
    void draw( const InputState& pressed, const std::function<std::string( const std::string& )>& translate );
    // bouton sous le point (x, y) en pixels du panneau, ou -1
    int hit( int x, int y ) const;
    // centre d'un bouton en pixels du panneau (tests, mise en page)
    void centerOf( int button, int& cx, int& cy ) const;

  private:
    int zoom;
    int widthUnits;
    int dpadX = 0, dpadY = 0, btnY = 0, btnR = 0, btnX[4] = {0};
    int u( int v ) const { return v * kUnit; }

    void circle( int cx, int cy, int r, RGBColor c );
    void triangle( int x0, int y0, int x1, int y1, int x2, int y2, RGBColor c );
    void roundButton( int cx, int cy, int r, bool pressed, const char* letter );
    void dpad( const InputState& s );
    int  wrapped( int x, int y, int widthPx, const std::string& text, RGBColor c );   // renvoie le nombre de lignes
    void caption( int cx, int y, int widthPx, const std::string& keys, const std::string& role );
};
