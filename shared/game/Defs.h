// Defs.h -- types de donnees communs (animations, directions).
#pragma once
#include <cstdint>

enum Dir : uint8_t { DIR_BAS = 0, DIR_HAUT = 1, DIR_GAUCHE = 2, DIR_DROITE = 3 };
enum Mov : uint8_t { MOV_BASE = 0, MOV_MARCHE = 1, MOV_ATTAQUE = 2 };

// timings Ethynd : [ticks entre frames, derniere frame, liberer apres, revenir a base]
struct AnimTiming { int16_t tick; uint8_t last; bool libre; bool reset; };  // tick < 0 : pas d'animation

struct SpriteAnim { uint8_t count; const char* const* names; };

struct CharDef {
    const char* id;
    AnimTiming  timing[3];
    SpriteAnim  anim[4][3];     // [direction][mouvement]
};

constexpr int kMaxFrames = 4;
