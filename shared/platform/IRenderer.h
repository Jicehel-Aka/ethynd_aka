// IRenderer.h -- interface de rendu commune AKA / PC (meme esprit que Dark & Under).
// Difference avec Dark & Under : les images ne sont pas embarquees en flash, elles
// sont chargees depuis la SD (AKA) ou le disque (PC) puis ENREGISTREES ici.
// Ecran logique fixe : 320x240.
#pragma once
#include <cstdint>

using ImageId = uint16_t;
constexpr ImageId kInvalidImageId = 0xFFFF;

constexpr int kScreenW = 320;
constexpr int kScreenH = 240;

// Couleur-cle de transparence des images RGB565 (magenta). Doit rester egale a
// KEY dans tools/convert_assets.py et a la cle passee a gb_graphics cote AKA.
constexpr uint16_t kColorKey = 0xF81F;

struct RGBColor { uint8_t r, g, b; };

enum class FontSize { Wide, Narrow };

class IRenderer {
  public:
    virtual ~IRenderer() = default;

    // Enregistre une image 16 bits dans l'ordre NATIF du framebuffer AKA (BGR565 : rouge dans
    // les bits bas, voir lcd_color_rgb) -- c'est ce que produit tools/convert_assets.py. Le tampon n'est PAS copie : il doit
    // rester valide tant que l'image est utilisee. Pixels == kColorKey : transparents.
    // @return l'identifiant de l'image, ou kInvalidImageId si la table est pleine.
    virtual ImageId createImage( const uint16_t* pixels, uint16_t w, uint16_t h ) = 0;

    virtual void fillRect( int16_t x, int16_t y, int16_t w, int16_t h, RGBColor color ) = 0;
    virtual void drawImage( int16_t x, int16_t y, ImageId image ) = 0;
    // texte UTF-8 ; scale >= 1 agrandit chaque pixel de la police (titres)
    virtual void drawText( int16_t x, int16_t y, const char* text, RGBColor color,
                           FontSize size = FontSize::Wide, int scale = 1 ) = 0;
    virtual void getImageSize( ImageId image, int16_t& outW, int16_t& outH ) const = 0;
    virtual void present() = 0;
};
