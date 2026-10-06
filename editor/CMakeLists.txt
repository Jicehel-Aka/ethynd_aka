cmake_minimum_required(VERSION 3.16)
project(ethynd_editor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

add_executable(ethynd_editor
    main_sdl.cpp
    EditorCore.cpp
    WorldText.cpp
)

target_include_directories(ethynd_editor PRIVATE
    ${SDL2_INCLUDE_DIRS}
    .
    ../shared/platform
    ../shared/game                      # Defs.h, EthyndData.h : types de monstres et de PNJ valides
    ../components/aka_font/include      # police 8x8 UTF-8 (facultative : sans elle, police 5x8)
)

if ( TARGET SDL2::SDL2 )
    target_link_libraries(ethynd_editor PRIVATE SDL2::SDL2)
    if ( TARGET SDL2::SDL2main )
        target_link_libraries(ethynd_editor PRIVATE SDL2::SDL2main)
    endif()
else()
    target_link_libraries(ethynd_editor PRIVATE ${SDL2_LIBRARIES})
endif()
