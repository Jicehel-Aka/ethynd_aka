// AkaAudio -- IAudio pour l'AKA.
//
// Les 4 pistes du gb_audio_player (AUDIO_PLAYER_TRACK_COUNT = 4) :
//   musique (MP3, en boucle) | canal joueur (WAV : pas, attaque, blessure)
//   canal monstres (WAV)     | canal interface (WAV : selection de menu)
//
// THREAD-SAFETY : gb_audio_player::pool() tourne dans une tache dediee (comme dans Dark & Under).
// Le jeu (thread principal) ne touche jamais aux pistes : il poste des commandes dans une file,
// que la tache audio applique via service() juste avant pool().
#pragma once
#include <atomic>
#include "IAudio.h"
#include "gb_audio_player.h"
#include "gb_audio_track_wav.h"
#include "gb_audio_track_mp3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class AkaAudio : public IAudio {
  public:
    explicit AkaAudio( gb_audio_player& player, const char* baseDir = "/sdcard/Ethynd/audio" );
    void begin();                       // enregistre les 4 pistes dans le lecteur
    void service();                     // a appeler par la tache audio, AVANT player.pool()
    // volumes du menu systeme (0..100) ; appliques dans service()
    void setVolumes( uint8_t music, uint8_t sfx );

    void playMusic( Music m ) override;
    void playSfx( Sfx s ) override;
    bool playerSfxBusy() override { return busyPlayer.load(); }

  private:
    struct Cmd { uint8_t isMusic; uint8_t arg; };
    gb_audio_player&   player;
    const char*        dir;
    QueueHandle_t      queue;
    gb_audio_track_mp3 musicTrack;
    gb_audio_track_wav playerTrack, monsterTrack, uiTrack;
    Music              currentMusic = Music::None;
    std::atomic<bool>  busyPlayer { false };
    std::atomic<uint8_t> volMusic { 80 }, volSfx { 80 };
    std::atomic<bool>  volDirty { true };

    void post( Cmd c );
    void applyMusic( Music m );
    void applySfx( Sfx s );
    static void playWav( gb_audio_track_wav& t, const char* path );
};
