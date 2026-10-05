// IAudio.h -- sons du jeu. Cote AKA : gb_audio_track_mp3 (musiques) + gb_audio_track_wav
// (effets courts). Cote PC : au choix (muet pour les tests).
#pragma once

enum class Music { None, Menu, Maison, Aventure, Grotte };
enum class Sfx   { MenuSelect, Walk, Attack, Hurt, MonsterHit };

class IAudio {
  public:
    virtual ~IAudio() = default;
    virtual void playMusic( Music m ) = 0;      // en boucle ; Music::None arrete
    virtual void playSfx( Sfx s ) = 0;
    // true tant que le canal "joueur" (pas, attaque, blessure) joue encore :
    // sert a ne relancer le bruit de pas qu'une fois le precedent termine.
    virtual bool playerSfxBusy() = 0;
};

// Implementation muette (tests, builds sans son).
class NullAudio : public IAudio {
  public:
    void playMusic( Music ) override {}
    void playSfx( Sfx ) override {}
    bool playerSfxBusy() override { return false; }
};
