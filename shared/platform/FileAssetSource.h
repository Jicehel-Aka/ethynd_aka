// FileAssetSource.h -- lit les assets convertis dans un dossier du disque (PC : tests et SDL).
#pragma once
#include <string>
#include <cstdio>
#include "IAssetSource.h"

class FileAssetSource : public IAssetSource {
  public:
    explicit FileAssetSource( std::string d ) : dir( d ) {}
    bool read( const char* rel, std::vector<uint8_t>& out ) override {
        FILE* f = fopen( ( dir + "/" + rel ).c_str(), "rb" );
        if ( !f ) { fprintf( stderr, "introuvable : %s/%s\n", dir.c_str(), rel ); return false; }
        fseek( f, 0, SEEK_END ); long n = ftell( f ); fseek( f, 0, SEEK_SET );
        if ( n <= 0 ) { fclose( f ); return false; }
        out.resize( (size_t)n );
        bool ok = fread( out.data(), 1, (size_t)n, f ) == (size_t)n;
        fclose( f ); return ok;
    }
  private:
    std::string dir;
};
