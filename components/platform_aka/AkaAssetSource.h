// AkaAssetSource -- lit les assets convertis sur la SD : /sdcard/Ethynd/<chemin relatif>.
// Les gros tampons (> 16 Ko) vont automatiquement en PSRAM (CONFIG_SPIRAM_USE_MALLOC).
#pragma once
#include <string>
#include "IAssetSource.h"

class AkaAssetSource : public IAssetSource {
  public:
    explicit AkaAssetSource( const char* baseDir ) : base( baseDir ) {}
    bool read( const char* relPath, std::vector<uint8_t>& out ) override;
  private:
    std::string base;
};
