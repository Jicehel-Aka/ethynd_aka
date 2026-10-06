// main.cpp (AKA) -- Ethynd sur Gamebuino AKA.
// Memes briques que Dark & Under : gb_core / gb_graphics, akaRuntime (menu systeme, volume,
// SD), tache FreeRTOS dediee au mixage audio. Le jeu tourne a pas fixe de 30 ticks/s (comme
// tick(30) de pygame) : la logique est cadencee par le temps, le rendu une fois par tour.
#include "AkaRenderer.h"
#include "AkaInput.h"
#include "AkaAudio.h"
#include "AkaAssetSource.h"
#include "Game.h"

#include "gb_core.h"
#include "gb_graphics.h"
#include "gb_audio_player.h"
#include "core/input.h"
#include "aka_runtime/aka_runtime.h"
#include "PlatformTime.h"
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

gb_core           g_core;
gb_graphics       gfx;
gb_audio_player   g_audio_player;
static AkaAudio*  g_audio = nullptr;

static void audio_mix_task( void* )
{
    while ( true ) {
        g_audio->service();          // applique les commandes du jeu (musique, effets, volumes)
        g_audio_player.pool();
        vTaskDelay( pdMS_TO_TICKS( 5 ) );
    }
}

static void onVolumeChanged( uint8_t musicVol, uint8_t sfxVol )
{
    if ( g_audio ) g_audio->setVolumes( musicVol, sfxVol );
}

static void fatalScreen( const char* line1, const char* line2 )
{
    gfx.clear( gfx.makeColor( 0, 0, 0 ) );
    gfx.setColor( gfx.makeColor( 255, 80, 80 ) );
    gfx.move_cursor( 8, 100 );  gfx.print_str( line1 );
    gfx.move_cursor( 8, 120 );  gfx.print_str( line2 );
    gfx.update();
}

extern "C" void app_main( void )
{
    g_core.init();
    input_init();
    akaRuntime.begin( "Ethynd" );
    akaRuntime.setVolumeChangedCallback( onVolumeChanged );

    // Libelles du menu Commandes : les cles existent dans sdcard_files/Ethynd/lang/*.json
    static const char* const kControls[] = { "CTRL_MOVE", "CTRL_ATTACK", "CTRL_HELP", "CTRL_BACK", nullptr };
    akaRuntime.setControlsKeys( kControls );
    akaRuntime.setCredits( "Ethynd (AKA)",
                           "Projet ISN 2019 -- Sofiane, Dorian, Anthony (lycee) ; musiques : dcinoot",
                           "Unlicense (domaine public) -- code d'origine : github.com/ProjetIsn2019/Ethynd",
                           "Portage Gamebuino AKA" );

    static AkaRenderer    renderer( gfx );
    static AkaInput       input;
    static AkaAssetSource source( "/sdcard/Ethynd" );
    static AkaAudio       audio( g_audio_player );
    g_audio = &audio;
    audio.begin();
    audio.setVolumes( akaRuntime.getMusicVolume(), akaRuntime.getSfxVolume() );
    xTaskCreatePinnedToCore( audio_mix_task, "AudioMixTask", 6144, nullptr, 5, nullptr, 1 );

    static Game game( renderer, audio, source );
    game.setQuitHintKey( "ui.quit.aka" );
    game.setLanguage( akaRuntime.getLanguage() );
    if ( !game.init() ) {
        fatalScreen( "Ethynd : assets introuvables", "Copier sdcard_files/Ethynd sur la SD" );
        while ( true ) vTaskDelay( pdMS_TO_TICKS( 1000 ) );
    }

    const uint32_t kTickMs = 1000 / kTick;           // 33 ms : un tick logique
    uint32_t last = platformMillis(), acc = 0;
    char lastLang[8] = "";
    while ( true ) {
        Keys keys;
        input_poll( keys );                           // AVANT akaRuntime.update() (voir Dark & Under)
        if ( !akaRuntime.update( keys ) ) {           // menu systeme ouvert : le jeu est en pause
            last = platformMillis();
            acc = 0;
            continue;
        }
        // Langue du menu systeme : textes des ecrans de menu du jeu (FR par defaut)
        const char* lang = akaRuntime.getLanguage();
        if ( strncmp( lang, lastLang, sizeof lastLang - 1 ) != 0 ) {
            strncpy( lastLang, lang, sizeof lastLang - 1 );
            game.setLanguage( lang );             // code du menu systeme : fr, en, ... (repli sur fr si pas de lang/<code>.bin)
        }

        uint32_t now = platformMillis();
        acc += now - last;
        last = now;
        if ( acc > 4 * kTickMs ) acc = 4 * kTickMs;   // pas de rattrapage infini apres une pause
        input.poll();
        while ( acc >= kTickMs ) {
            game.update( input );
            acc -= kTickMs;
        }
        game.render();
        renderer.present();
    }
}
