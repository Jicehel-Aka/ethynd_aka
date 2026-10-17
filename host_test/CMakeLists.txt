# Tests du jeu et de l'editeur avec CMake / CTest (Windows, Linux, macOS), sans ecran :
#   cmake -S host_test -B build_test -DETHYND_ASSETS=<sortie de convert_assets.py --tile 24> \
#         -DETHYND_EDITOR_ASSETS=<sortie --editor-out>
#   cmake --build build_test --config Release && ctest --test-dir build_test -C Release --output-on-failure
cmake_minimum_required(VERSION 3.16)
project(ethynd_tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ( MSVC )
    add_compile_options(/utf-8)
    add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

set(ETHYND_ASSETS "" CACHE PATH "assets convertis du jeu (--tile 24)")
set(ETHYND_EDITOR_ASSETS "" CACHE PATH "assets de l'editeur (--editor-out)")
set(SHARED ${CMAKE_CURRENT_SOURCE_DIR}/../shared)
set(INCS ${SHARED}/platform ${SHARED}/game ${CMAKE_CURRENT_SOURCE_DIR}/../components/aka_font/include)

add_executable(ethynd_test host_main.cpp
    ${SHARED}/game/Game.cpp ${SHARED}/game/GameMap.cpp ${SHARED}/game/Characters.cpp
    ${SHARED}/game/AssetStore.cpp ${SHARED}/game/World.cpp ${SHARED}/game/Lang.cpp)
target_include_directories(ethynd_test PRIVATE ${INCS} .)

add_executable(ethynd_editor_test ../editor/test_editor.cpp ../editor/EditorCore.cpp ../editor/WorldText.cpp)
target_include_directories(ethynd_editor_test PRIVATE ${INCS} ../editor)

add_executable(ethynd_panel_test panel_test.cpp ../platform_sdl/ControlPanel.cpp ../editor/WorldText.cpp)
target_include_directories(ethynd_panel_test PRIVATE ${INCS} ../platform_sdl ../editor)

enable_testing()
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shots)
if ( ETHYND_ASSETS )
    add_test(NAME game COMMAND ethynd_test ${ETHYND_ASSETS} ${CMAKE_BINARY_DIR}/shots)
endif()
add_test(NAME panel COMMAND ethynd_panel_test ${CMAKE_CURRENT_SOURCE_DIR}/../world ${CMAKE_BINARY_DIR}/shots)
if ( ETHYND_EDITOR_ASSETS )
    add_test(NAME editor COMMAND ethynd_editor_test ${CMAKE_CURRENT_SOURCE_DIR}/../world ${ETHYND_EDITOR_ASSETS}
                                 ${CMAKE_BINARY_DIR}/editor_work ${CMAKE_BINARY_DIR}/shots)
endif()
