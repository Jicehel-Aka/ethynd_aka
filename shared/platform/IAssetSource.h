// IAssetSource.h -- lecture des fichiers d'assets convertis (tools/convert_assets.py).
// Chemins relatifs au dossier de sortie du convertisseur ("tiles.bin", "maps/maison.map"...).
// AKA : /sdcard/Ethynd/<chemin> (tampon en PSRAM) ; PC : dossier disque.
#pragma once
#include <cstdint>
#include <vector>

class IAssetSource {
  public:
    virtual ~IAssetSource() = default;
    virtual bool read( const char* relPath, std::vector<uint8_t>& out ) = 0;
};
