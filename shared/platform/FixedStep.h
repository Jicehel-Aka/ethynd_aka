// FixedStep.h -- boucle a pas fixe partagee par la console et le PC.
//
// Le jeu avance par ticks de 33 ms (30 par seconde) ; le rendu, lui, peut etre plus lent ou plus rapide.
// RECETTE a respecter : l'entree se lit A CHAQUE TICK (dans onTick), jamais une fois par tour de boucle.
//  - un tour sans tick ne doit pas "consommer" un appui (sinon justPressed() ne le verrait jamais) ;
//  - un tour qui rattrape 2 ticks ne doit pas montrer 2 fois le meme front montant (sinon un dialogue saute 2 pages).
#pragma once
#include <cstdint>

class FixedStep {
  public:
    explicit FixedStep( uint32_t tickMs, int maxCatchUp = 4 ) : tick( tickMs ), maxCatch( maxCatchUp ) {}
    void reset( uint32_t now ) { last = now; acc = 0; }                 // apres une pause (menu systeme)
    // Execute onTick() autant de fois que le temps ecoule le demande ; renvoie le nombre de ticks.
    template <class F> int advance( uint32_t now, F&& onTick )
    {
        acc += now - last;
        last = now;
        if ( acc > (uint32_t)maxCatch * tick ) acc = (uint32_t)maxCatch * tick;   // pas de rattrapage infini
        int n = 0;
        while ( acc >= tick ) { onTick(); acc -= tick; n++; }
        return n;
    }
  private:
    uint32_t tick, last = 0, acc = 0;
    int maxCatch;
};
