#include "AkaAssetSource.h"
#include <stdio.h>
#include "esp_log.h"

static const char* TAG = "assets";

bool AkaAssetSource::read( const char* relPath, std::vector<uint8_t>& out )
{
    std::string path = base + "/" + relPath;
    FILE* f = fopen( path.c_str(), "rb" );
    if ( !f ) { ESP_LOGE( TAG, "introuvable : %s", path.c_str() ); return false; }
    fseek( f, 0, SEEK_END );
    long n = ftell( f );
    fseek( f, 0, SEEK_SET );
    if ( n <= 0 ) { fclose( f ); return false; }
    out.resize( (size_t)n );
    bool ok = fread( out.data(), 1, (size_t)n, f ) == (size_t)n;
    fclose( f );
    if ( !ok ) ESP_LOGE( TAG, "lecture incomplete : %s", path.c_str() );
    return ok;
}
