// VirtualPad.h -- boutons a l'ecran : suit souris et doigts (plusieurs a la fois) et calcule les boutons enfonces.
// Aucune dependance a SDL (testable sans fenetre) : la fenetre lui envoie des coordonnees, il rend un masque de bits.
#pragma once
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

class VirtualPad {
  public:
    using HitFn = std::function<int( int, int )>;           // (x, y) -> numero de bouton, ou -1
    explicit VirtualPad( HitFn hit ) : hitFn( std::move( hit ) ) {}

    void down( int64_t id, int x, int y ) { set( id, hitFn( x, y ), true ); }
    void move( int64_t id, int x, int y ) { for ( auto& p : ptrs ) if ( p.first == id ) { p.second = hitFn( x, y ); return; } }   // glisser d'un bouton a l'autre
    void up( int64_t id ) { for ( size_t i = 0; i < ptrs.size(); i++ ) if ( ptrs[i].first == id ) { ptrs.erase( ptrs.begin() + (long)i ); return; } }
    void clear() { ptrs.clear(); }

    // bit n = bouton n enfonce par au moins un pointeur
    uint32_t mask() const { uint32_t m = 0; for ( const auto& p : ptrs ) if ( p.second >= 0 ) m |= 1u << p.second; return m; }

  private:
    HitFn hitFn;
    std::vector<std::pair<int64_t, int>> ptrs;               // pointeur -> bouton sous lui (-1 : aucun)
    void set( int64_t id, int button, bool add ) {
        for ( auto& p : ptrs ) if ( p.first == id ) { p.second = button; return; }
        if ( add ) ptrs.push_back( { id, button } );
    }
};
