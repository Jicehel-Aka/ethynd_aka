// AkaRenderer.h/.cpp -- IRenderer pour l'AKA, par-dessus gb_graphics.
// Difference avec Dark & Under : les images ne sont pas en flash. Elles sont chargees depuis
// la SD (tampons en PSRAM) puis enregistrees avec createImage() ; un ImageId indexe une table
// dynamique. Ecran plein 320x240, pas de decalage.
#pragma once
#include <vector>
#include "IRenderer.h"
#include "gb_graphics.h"

class AkaRenderer : public IRenderer {
  public:
    explicit AkaRenderer( gb_graphics& gfx ) : gfx( gfx ) { assets.reserve( 768 ); }

    ImageId createImage( const uint16_t* pixels, uint16_t w, uint16_t h ) override;
    void fillRect( int16_t x, int16_t y, int16_t w, int16_t h, RGBColor color ) override;
    void drawImage( int16_t x, int16_t y, ImageId image ) override;
    void drawText( int16_t x, int16_t y, const char* text, RGBColor color, FontSize size = FontSize::Wide, int scale = 1 ) override;
    void getImageSize( ImageId image, int16_t& outW, int16_t& outH ) const override;
    void present() override;

  private:
    struct Asset { const uint16_t* pixels; uint16_t w, h; };
    gb_graphics& gfx;
    std::vector<Asset> assets;
};
