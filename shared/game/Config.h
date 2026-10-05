// Config.h -- constantes de jeu.
//
// LOGIQUE en pixels de 32 px (comme Ethynd, ecran logique 640x480, joueur au centre) ;
// AFFICHAGE : 24 px par case (echelle 3/4) sur l'ecran 320x240 de l'AKA.
#pragma once

constexpr int kTick        = 30;          // ticks logiques par seconde
constexpr int kLogicW      = 640;
constexpr int kLogicH      = 480;
constexpr int kLogicCx     = kLogicW / 2; // 320 : centre du sprite joueur
constexpr int kLogicCy     = kLogicH / 2; // 240
constexpr int kTileLogic   = 32;
constexpr int kTileShow    = 24;          // doit correspondre a "convert_assets.py --tile 24"
constexpr int kPlayerSpeed = 3;
constexpr int kEntitySpeed = 2;
constexpr int kPlayerLife  = 10;
constexpr int kEnemyDamage = 1;           // vie perdue par contact (Ethynd : toujours 1)
constexpr int kTileAnimTicks = 5;         // les tuiles animees avancent tous les 6 ticks

// Hitbox des jambes du joueur (en coordonnees ecran logique, centree en (320, 253))
constexpr int kPlayerHitW  = 25;
constexpr int kPlayerHitH  = 20;
constexpr int kPlayerHitCy = kLogicCy + 13;
constexpr int kPlayerSpriteLogic = 64;    // sprite joueur 64x64 (logique)
