#include "SdlAudio.h"
#include <cstdio>

static const char* const kMusicFiles[5] = { "", "menu.mp3", "maison.mp3", "aventure.mp3", "grotte.mp3" };
static const char* const kSfxFiles[5] = { "selection_menu.wav", "joueur_marche.wav", "joueur_attaque.wav",
                                          "joueur_blessure.wav", "monstre_chauve_souris.wav" };

SdlAudio::SdlAudio( std::string d ) : dir( d )
{
    if ( SDL_InitSubSystem( SDL_INIT_AUDIO ) != 0 || Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) != 0 ) {
        std::fprintf( stderr, "audio indisponible : %s (le jeu continue sans son)\n", Mix_GetError() );
        return;
    }
    opened = true;
    Mix_AllocateChannels( 8 );
    for ( int i = 1; i < 5; i++ ) {
        music[i] = Mix_LoadMUS( ( dir + "/" + kMusicFiles[i] ).c_str() );
        if ( !music[i] ) std::fprintf( stderr, "musique %s : %s\n", kMusicFiles[i], Mix_GetError() );
    }
    for ( int i = 0; i < 5; i++ ) {
        sfx[i] = Mix_LoadWAV( ( dir + "/" + kSfxFiles[i] ).c_str() );
        if ( !sfx[i] ) std::fprintf( stderr, "effet %s : %s\n", kSfxFiles[i], Mix_GetError() );
    }
}

SdlAudio::~SdlAudio()
{
    if ( !opened ) return;
    Mix_HaltMusic();
    Mix_HaltChannel( -1 );
    for ( Mix_Music* m : music ) if ( m ) Mix_FreeMusic( m );
    for ( Mix_Chunk* c : sfx ) if ( c ) Mix_FreeChunk( c );
    Mix_CloseAudio();
}

void SdlAudio::setMuted( bool m )
{
    isMuted = m;
    if ( !opened ) return;
    Mix_VolumeMusic( m ? 0 : MIX_MAX_VOLUME );
    Mix_Volume( -1, m ? 0 : MIX_MAX_VOLUME );
}

void SdlAudio::playMusic( Music m )
{
    if ( !opened || ( m == current && Mix_PlayingMusic() ) ) return;
    current = m;
    if ( m == Music::None ) { Mix_HaltMusic(); return; }
    if ( music[(int)m] ) Mix_PlayMusic( music[(int)m], -1 );      // -1 = en boucle
}

void SdlAudio::playSfx( Sfx s )
{
    if ( !opened || !sfx[(int)s] ) return;
    switch ( s ) {
        case Sfx::MenuSelect: Mix_PlayChannel( kChUi, sfx[(int)s], 0 ); break;
        case Sfx::Walk: case Sfx::Attack: case Sfx::Hurt: Mix_PlayChannel( kChPlayer, sfx[(int)s], 0 ); break;  // remplace le son en cours
        case Sfx::MonsterHit: if ( !Mix_Playing( kChMonster ) ) Mix_PlayChannel( kChMonster, sfx[(int)s], 0 ); break;
    }
}
