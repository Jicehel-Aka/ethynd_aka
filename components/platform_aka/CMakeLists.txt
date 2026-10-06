idf_component_register(
    SRCS "AkaRenderer.cpp" "AkaInput.cpp" "AkaAssetSource.cpp" "AkaAudio.cpp"
         "../../shared/game/Game.cpp"
         "../../shared/game/GameMap.cpp"
         "../../shared/game/Characters.cpp"
         "../../shared/game/AssetStore.cpp"
         "../../shared/game/World.cpp"
         "../../shared/game/Lang.cpp"
    INCLUDE_DIRS "." "../../shared/platform" "../../shared/game"
    REQUIRES gamebuino aka_runtime aka_font)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=maybe-uninitialized -Wno-error=unused-variable
    -Wno-error=unused-parameter -Wno-error=sign-compare
    -Wno-error=reorder -Wno-error=switch -Wno-error=parentheses
    -Wno-error=unused-function -Wno-error=missing-field-initializers)
