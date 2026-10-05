// SdlAudio -- IAudio pour la build PC, par-dessus SDL2_mixer. Memes fichiers que la console :
// audio/*.mp3 (musiques, en boucle) et audio/*.wav (effets). Memes canaux que sur l'AKA :
// 1 = joueur, 2 = monstres, 3 = interface.
#pragma once
#include <string>
#include <SDL2/SDL_mixer.h>
#include "IAudio.h"

class SdlAudio : public IAudio {
  public:
    explicit SdlAudio( std::string audioDir );
    ~SdlAudio() override;
    bool ok() const { return opened; }
    void setMuted( bool m );
    bool muted() const { return isMuted; }

    void playMusic( Music m ) override;
    void playSfx( Sfx s ) override;
    bool playerSfxBusy() override { return opened && Mix_Playing( kChPlayer ) != 0; }

  private:
    static constexpr int kChPlayer = 1, kChMonster = 2, kChUi = 3;
    std::string dir;
    bool opened = false, isMuted = false;
    Mix_Music* music[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    Mix_Chunk* sfx[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    Music current = Music::None;
};
