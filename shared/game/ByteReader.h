// ByteReader.h -- lecture binaire little-endian avec controle de debordement.
#pragma once
#include <cstdint>
#include <cstring>
#include <string>

class ByteReader {
  public:
    ByteReader( const uint8_t* d, size_t n ) : p( d ), size( n ) {}
    bool ok() const { return good; }
    size_t pos() const { return i; }
    uint8_t  u8()  { uint8_t v = 0; take( &v, 1 ); return v; }
    int8_t   s8()  { return (int8_t)u8(); }
    uint16_t u16() { uint16_t v = 0; take( &v, 2 ); return v; }
    int16_t  s16() { return (int16_t)u16(); }
    uint32_t u32() { uint32_t v = 0; take( &v, 4 ); return v; }
    std::string sstr() { return str( u8() ); }       // u8 longueur + octets
    std::string lstr() { return str( u16() ); }      // u16 longueur + octets
    bool tag( const char* t4 ) { char b[4] = {0}; take( b, 4 ); return good && !memcmp( b, t4, 4 ); }
  private:
    const uint8_t* p; size_t size, i = 0; bool good = true;
    void take( void* dst, size_t n ) {
        if ( !good || i + n > size ) { good = false; memset( dst, 0, n ); return; }
        memcpy( dst, p + i, n ); i += n;                // x86 et ESP32 sont little-endian
    }
    std::string str( size_t n ) {
        if ( !good || i + n > size ) { good = false; return std::string(); }
        std::string s( (const char*)p + i, n ); i += n; return s;
    }
};
