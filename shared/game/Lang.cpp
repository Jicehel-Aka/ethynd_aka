#include "Lang.h"
#include "ByteReader.h"
#include <cstdio>

bool Lang::readFile( IAssetSource& src, const char* code, std::unordered_map<std::string, std::string>& out )
{
    char path[48];
    snprintf( path, sizeof path, "lang/%s.bin", code );
    std::vector<uint8_t> d;
    if ( !src.read( path, d ) ) return false;
    ByteReader r( d.data(), d.size() );
    if ( !r.tag( "ELNG" ) ) return false;
    int n = r.u16();
    out.clear();
    for ( int i = 0; i < n; i++ ) { std::string k = r.sstr(); out[k] = r.lstr(); }
    return r.ok();
}

bool Lang::load( IAssetSource& src, const char* code )
{
    if ( base.empty() && !readFile( src, kDefault, base ) ) return false;
    over.clear();
    current = kDefault;
    if ( code && std::string( code ) != kDefault && readFile( src, code, over ) ) current = code;
    return true;
}

const char* Lang::get( const std::string& key ) const
{
    auto it = over.find( key );
    if ( it != over.end() ) return it->second.c_str();
    it = base.find( key );
    return it != base.end() ? it->second.c_str() : key.c_str();
}
