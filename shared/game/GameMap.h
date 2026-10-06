// GameMap.h -- une carte : 4 couches de tuiles, animation des tuiles, collisions, rendu.
// La camera (camX, camY) est le decalage ecran->monde en pixels LOGIQUES, comme x_camera /
// y_camera d'Ethynd : la tuile (tx, ty) est a l'ecran logique (camX + 32*tx, camY + 32*ty).
// Le joueur reste fixe au centre ; ce sont les touches qui deplacent la camera.
#pragma once
#include <vector>
#include "AssetStore.h"

class GameMap {
  public:
    bool load( const AssetStore& assets, IAssetSource& src, const char* name, int camX, int camY );

    const char* name() const { return mapName; }
    const MapObjects& objects() const { return data.objects; }
    int mapWidth() const { return data.w; }
    int mapHeight() const { return data.h; }
    int camX = 0, camY = 0;

    void update();                                        // animation des tuiles (tous les 6 ticks)
    // collision d'un rectangle ECRAN logique avec les tuiles pour une camera donnee
    bool collidesScreen( const Rect& screenRect, int cx, int cy ) const;
    // collision d'un rectangle MONDE logique (utilise pour les monstres)
    bool collidesWorld( const Rect& worldRect ) const;

    void drawBackground( IRenderer& r ) const;            // couches 0, 1, 2
    void drawForeground( IRenderer& r ) const;            // couche 3 (devant le joueur)
    // origine d'affichage (pixels ecran 320x240) du monde, pour dessiner des entites
    int originX() const { return kScreenW / 2 + floorDiv( ( camX - kLogicCx ) * kTileShow, kTileLogic ); }
    int originY() const { return kScreenH / 2 + floorDiv( ( camY - kLogicCy ) * kTileShow, kTileLogic ); }

  private:
    const AssetStore* assets = nullptr;
    char mapName[16] = {0};
    MapData data;
    std::vector<uint16_t> coll[3];                        // tuile a collision des couches 0..2 AU CHARGEMENT
    int counter = 0;
    void drawLayers( IRenderer& r, int first, int last ) const;
};
