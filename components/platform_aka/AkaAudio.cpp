#include "AkaAudio.h"
#include <stdio.h>

AkaAudio::AkaAudio( gb_audio_player& p, const char* baseDir ) : player( p ), dir( baseDir )
{
    queue = xQueueCreate( 12, sizeof( Cmd ) );
}

void AkaAudio::begin()
{
    player.add_track( &musicTrack );
    player.add_track( &playerTrack );
    player.add_track( &monsterTrack );
    player.add_track( &uiTrack );
}

void AkaAudio::post( Cmd c ) { if ( queue ) xQueueSend( queue, &c, 0 ); }     // file pleine : commande perdue
void AkaAudio::playMusic( Music m ) { post( Cmd{ 1, (uint8_t)m } ); }
void AkaAudio::playSfx( Sfx s )     { post( Cmd{ 0, (uint8_t)s } ); }

void AkaAudio::setVolumes( uint8_t music, uint8_t sfx )
{
    volMusic = music > 100 ? 100 : music;
    volSfx = sfx > 100 ? 100 : sfx;
    volDirty = true;
}

void AkaAudio::service()
{
    if ( volDirty.exchange( false ) ) {
        // Volume maitre = le plus fort des deux curseurs (echelle 0..255 comme dans Dark & Under) ;
        // chaque piste est ensuite ramenee a son propre curseur.
        uint8_t m = volMusic, s = volSfx, top = m > s ? m : s;
        player.set_master_volume( (uint8_t)( top * 255 / 100 ) );
        float fm = top ? (float)m / top : 0.0f, fs = top ? (float)s / top : 0.0f;
        musicTrack.set_track_volume( fm );
        playerTrack.set_track_volume( fs );
        monsterTrack.set_track_volume( fs );
        uiTrack.set_track_volume( fs );
    }
    Cmd c;
    while ( queue && xQueueReceive( queue, &c, 0 ) == pdTRUE ) {
        if ( c.isMusic ) applyMusic( (Music)c.arg ); else applySfx( (Sfx)c.arg );
    }
    busyPlayer = playerTrack.is_playing();
}

void AkaAudio::applyMusic( Music m )
{
    if ( m == currentMusic && ( m == Music::None || musicTrack.is_playing() ) ) return;
    currentMusic = m;
    if ( m == Music::None ) { musicTrack.stop_playing(); return; }
    static const char* const names[] = { "", "menu.mp3", "maison.mp3", "aventure.mp3", "grotte.mp3" };
    char path[96];
    snprintf( path, sizeof path, "%s/%s", dir, names[(int)m] );
    musicTrack.play_mp3( path, true );
}

// play_wav() sur une piste deja en lecture : on l'arrete d'abord (equivalent de channel.play())
void AkaAudio::playWav( gb_audio_track_wav& t, const char* path )
{
    if ( t.is_playing() ) t.stop_playing();
    t.play_wav( path );
}

void AkaAudio::applySfx( Sfx s )
{
    char path[96];
    auto file = [&]( const char* name ) { snprintf( path, sizeof path, "%s/%s", dir, name ); return path; };
    switch ( s ) {
        case Sfx::MenuSelect: playWav( uiTrack,     file( "selection_menu.wav" ) ); break;
        case Sfx::Walk:       playWav( playerTrack, file( "joueur_marche.wav" ) ); break;
        case Sfx::Attack:     playWav( playerTrack, file( "joueur_attaque.wav" ) ); break;
        case Sfx::Hurt:       playWav( playerTrack, file( "joueur_blessure.wav" ) ); break;
        case Sfx::MonsterHit: if ( !monsterTrack.is_playing() ) playWav( monsterTrack, file( "monstre_chauve_souris.wav" ) ); break;
    }
}
