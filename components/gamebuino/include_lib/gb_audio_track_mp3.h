/*
Piste audio MP3 pour la bibliotheque Gamebuino-AKA.
Meme interface que gb_audio_track_wav : a ajouter au gb_audio_player
avec add_track(). Decodeur : minimp3 (lieff/minimp3, CC0), a placer dans
include_lib/minimp3.h.

Fonctionnement :
 - une tache FreeRTOS lit le fichier sur la SD, decode (minimp3), passe en mono,
   reechantillonne vers GB_AUDIO_SAMPLE_RATE et remplit un tampon circulaire ;
 - play_callback() (appele par gb_audio_player::pool()) ne fait que vider ce
   tampon : aucun acces SD ni decodage dans le thread principal.
 - les delais d'encodeur sont rognes grace au tag Xing/Info + LAME/Lavc, ce qui
   donne des boucles sans trou quand le fichier en contient un (ffmpeg et lame
   l'ecrivent par defaut).
*/
#pragma once

#include "gb_audio_player.h"
#include <atomic>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

//! tampon circulaire entre decodeur et mixeur, en echantillons (puissance de 2)
#ifndef GB_MP3_RING_SAMPLES
#define GB_MP3_RING_SAMPLES 8192
#endif
//! pile de la tache de decodage (minimp3 en utilise ~16 Ko sur la pile)
#ifndef GB_MP3_TASK_STACK
#define GB_MP3_TASK_STACK   (24 * 1024)
#endif
//! priorite de la tache de decodage
#ifndef GB_MP3_TASK_PRIO
#define GB_MP3_TASK_PRIO    5
#endif
//! taille du tampon de lecture SD
#ifndef GB_MP3_IN_BUF
#define GB_MP3_IN_BUF       8192
#endif

class gb_audio_track_mp3 : public gb_audio_track_base {
    public:
    gb_audio_track_mp3() {}
    ~gb_audio_track_mp3() { stop_playing(); }

        //! joue un MP3 depuis la SD. Le chemin doit etre sous MOUNT_POINT, sans "..".
        //! Tout MP3 est accepte (MPEG1/2/2.5 couche III, mono ou stereo, toute
        //! frequence de 8 a 48 kHz) : il est converti en mono GB_AUDIO_SAMPLE_RATE.
        //! @return GB_OK, GB_ERR_PARAM, GB_ERR_NOT_FOUND (ou SD non montee),
        //!         GB_ERR_IO, GB_ERR_FORMAT, GB_ERR_NO_SPACE (memoire/tache)
    int play_mp3( const char* pc_file_name, bool loop = false );
        //! active/desactive le bouclage en cours de lecture
    void set_loop( bool loop ) { _loop = loop; }
        //! @return 0 si un buffer a ete produit, -1 si la piste est inactive
    int play_callback( int16_t* pi16_buffer, uint16_t u16_sample_count ) override;
    void stop_playing() override;
    bool is_playing() override { return _playing.load(); }
        //! position de lecture en ms depuis le debut (cumule sur les boucles)
    uint32_t position() override;
        //! duree en ms si le fichier a un tag Xing/Info, sinon 0
    uint32_t length() override { return _length_ms.load(); }

    private:
    static void decode_task_entry( void* arg );
    void decode_task();
    void decode_loop( void* dec, uint8_t* in, int16_t* pcm, int16_t* mono );
    bool push_samples( const int16_t* s, uint32_t n );     // bloquant, false si stop
    bool emit_resampled( const int16_t* mono, uint32_t n );

    FILE*                   _f {0};
    uint32_t                _data_start {0};    // apres le tag ID3v2
    uint32_t                _audio_start {0};   // premiere trame audio (apres Xing)
    int16_t*                _ring {0};
    std::atomic<uint32_t>   _wr {0};            // compteurs libres (modulo implicite)
    std::atomic<uint32_t>   _rd {0};
    std::atomic<bool>       _playing {false};
    std::atomic<bool>       _done {false};      // le decodeur a fini
    std::atomic<bool>       _stop_req {false};
    std::atomic<bool>       _loop {false};
    std::atomic<uint32_t>   _played {0};        // echantillons envoyes au mixeur
    std::atomic<uint32_t>   _length_ms {0};
    SemaphoreHandle_t       _task_done {0};
        // reechantillonneur lineaire (utilise uniquement par la tache)
    uint32_t                _rs_step {65536};   // echantillons source par sortie, 16.16
    uint32_t                _rs_phase {0};
    int32_t                 _rs_prev {0};
};
