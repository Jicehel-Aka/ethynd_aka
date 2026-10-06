// main.cpp (build PC/SDL) -- Ethynd, meme coeur de jeu (shared/game) que la build AKA.
//
// Usage : ethynd_pc [zoom 2..4] [--assets <dossier>] [--lang fr|en]
//   Le dossier d'assets est celui produit par tools/convert_assets.py (le meme que sur la SD :
//   /sdcard/Ethynd). Par defaut : "Ethynd" a cote de l'executable, sinon ../sdcard_files/Ethynd.
// Touches : fleches ou ZQSD (deplacement), X / Espace / Entree = A (attaquer, valider),
//   C ou H = aide, Retour arriere ou B = retour, M = couper le son, Echap = quitter.
#include "SdlRenderer.h"
#include "SdlInput.h"
#include "SdlAudio.h"
#include "FileAssetSource.h"
#include "Game.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace {
std::string exeDirectory()
{
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA( nullptr, buf, MAX_PATH );
    std::string p( buf, len );
    size_t pos = p.find_last_of( "\\/" );
#else
    char buf[PATH_MAX];
    ssize_t len = readlink( "/proc/self/exe", buf, sizeof( buf ) - 1 );
    if ( len <= 0 ) return ".";
    buf[len] = '\0';
    std::string p( buf );
    size_t pos = p.find_last_of( '/' );
#endif
    return pos == std::string::npos ? "." : p.substr( 0, pos );
}
bool dirExists( const std::string& d ) { struct stat st; return stat( ( d + "/tiles.bin" ).c_str(), &st ) == 0; }
}

int main( int argc, char** argv )
{
    int zoom = 3;
    std::string assets, lang = "fr";
    for ( int i = 1; i < argc; i++ ) {
        if ( !strcmp( argv[i], "--assets" ) && i + 1 < argc ) assets = argv[++i];
        else if ( !strcmp( argv[i], "--lang" ) && i + 1 < argc ) lang = argv[++i];
        else if ( std::atoi( argv[i] ) >= 2 && std::atoi( argv[i] ) <= 4 ) zoom = std::atoi( argv[i] );
        else std::fprintf( stderr, "argument ignore : %s\n", argv[i] );
    }
    if ( assets.empty() ) {
        const std::string exe = exeDirectory();
        for ( const std::string& c : { exe + "/Ethynd", exe + "/../sdcard_files/Ethynd", std::string( "Ethynd" ), std::string( "sdcard_files/Ethynd" ) } )
            if ( dirExists( c ) ) { assets = c; break; }
    }
    if ( assets.empty() || !dirExists( assets ) ) {
        std::fprintf( stderr, "Assets introuvables. Lancer tools/build_assets.sh, ou passer --assets <dossier>.\n" );
        return 1;
    }

    SdlRenderer renderer( zoom );
    if ( !renderer.ok() ) return 1;
    SdlInput input;
    SdlAudio audio( assets + "/audio" );
    FileAssetSource source( assets );

    Game game( renderer, audio, source );
    game.setLanguage( lang.c_str() );
    game.setQuitHintKey( "ui.quit.pc" );
    if ( !game.init() ) {
        std::fprintf( stderr, "Chargement des assets impossible dans %s (convertis avec --tile 24 ?)\n", assets.c_str() );
        return 1;
    }

    const uint32_t kTickMs = 1000 / kTick;          // 33 ms : un tick logique, comme tick(30) de pygame
    uint32_t last = SDL_GetTicks(), acc = 0;
    while ( true ) {
        input.poll();
        if ( input.quitRequested() ) break;
        if ( input.toggleMuteRequested() ) audio.setMuted( !audio.muted() );

        uint32_t now = SDL_GetTicks();
        acc += now - last;
        last = now;
        if ( acc > 4 * kTickMs ) acc = 4 * kTickMs;  // pas de rattrapage infini apres une pause
        while ( acc >= kTickMs ) {
            game.update( input );
            acc -= kTickMs;
        }
        game.render();
        renderer.present();
        SDL_Delay( 1 );
    }
    return 0;
}
