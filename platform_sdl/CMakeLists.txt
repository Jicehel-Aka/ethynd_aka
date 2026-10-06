cmake_minimum_required(VERSION 3.16)
project(ethynd_pc CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)       # musiques MP3 et effets WAV (SDL2_image inutile : les assets sont des .bin)

add_executable(ethynd_pc
    main.cpp
    SdlRenderer.cpp
    SdlInput.cpp
    SdlAudio.cpp
    ../shared/game/Game.cpp
    ../shared/game/GameMap.cpp
    ../shared/game/Characters.cpp
    ../shared/game/AssetStore.cpp
    ../shared/game/World.cpp
    ../shared/game/Lang.cpp
)

target_include_directories(ethynd_pc PRIVATE
    ${SDL2_INCLUDE_DIRS}
    .
    ../shared/platform
    ../shared/game
    ../components/aka_font/include      # texte UTF-8 + accents (optionnel : sans lui, police 5x8 partout)
)

# Cibles modernes (vcpkg sous Windows) si elles existent, sinon variables classiques (Linux).
if ( TARGET SDL2_mixer::SDL2_mixer )
    set( SDL2_MIXER_LINK_TARGET SDL2_mixer::SDL2_mixer )
else()
    set( SDL2_MIXER_LINK_TARGET SDL2_mixer )
endif()

if ( TARGET SDL2::SDL2 )
    target_link_libraries(ethynd_pc PRIVATE SDL2::SDL2 ${SDL2_MIXER_LINK_TARGET})
    if ( TARGET SDL2::SDL2main )
        target_link_libraries(ethynd_pc PRIVATE SDL2::SDL2main)
    endif()
else()
    target_link_libraries(ethynd_pc PRIVATE ${SDL2_LIBRARIES} ${SDL2_MIXER_LINK_TARGET})
endif()
