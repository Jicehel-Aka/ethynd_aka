// AssetStore.h -- charge les fichiers produits par tools/convert_assets.py et les
// enregistre dans le renderer. Les tampons restent possedes ici (createImage ne copie pas).
#pragma once
#include <cstdint>
#include <vector>
#include "IRenderer.h"
#include "IAssetSource.h"
#include "Collision.h"
#include "Config.h"

constexpr uint16_t kNoTile = 0xFFFF;

enum MenuImage { MENU_MENU = 0, MENU_AIDE, MENU_CHARGEMENT, MENU_MORT, MENU_COUNT };

struct MapData {
    int w = 0, h = 0;
    std::vector<uint16_t> layer[4];    // identifiants de tuiles compactes, kNoTile = vide
};

class AssetStore {
  public:
    bool load( IAssetSource& src, IRenderer& r );
    bool loadMap( IAssetSource& src, const char* name, MapData& out ) const;

    int        tileCount() const { return (int)tileIds.size(); }
    ImageId    tileImage( uint16_t t ) const { return tileIds[t]; }
    const uint8_t* tileMask( uint16_t t ) const { return collide.data() + 12 + (size_t)t * kMaskBytes; }
    uint16_t   tileNext( uint16_t t ) const { return animNext[t]; }       // kNoTile si fixe
    ImageId    sprite( const char* name ) const;                          // kInvalidImageId si absent
    ImageId    menuImage( MenuImage m ) const { return menus[m]; }

  private:
    std::vector<uint8_t>  tilesBlob, collide, animBlob, spritesBlob, menuBlob[MENU_COUNT];
    std::vector<ImageId>  tileIds;
    std::vector<uint16_t> animNext;
    struct SpriteEntry { char name[24]; ImageId id; };
    std::vector<SpriteEntry> sprites;
    ImageId menus[MENU_COUNT] = { kInvalidImageId, kInvalidImageId, kInvalidImageId, kInvalidImageId };
};
