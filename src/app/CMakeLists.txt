# wayfarer_app: everything around the rules - the GameLib main-loop glue,
# the profile and the save slots, the world view and its shaders, the HUD,
# the windows and conversations, the screens, sounds, music and effects.
# Menus, settings, the controls list, sound banks, transitions and the
# attract mode come from GameLib (app, ui, audio, fx, ai, storage).
add_library(wayfarer_app STATIC
    actions.cpp
    dialogue_view.cpp
    effects.cpp
    hud.cpp
    look.cpp
    profile.cpp
    saves.cpp
    screens.cpp
    shapes.cpp
    sounds.cpp
    ui_kit.cpp
    view.cpp
    wayfarer_game.cpp
    windows.cpp)
target_include_directories(wayfarer_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(wayfarer_app PUBLIC wayfarer_game GameLib::app GameLib::ui GameLib::fx GameLib::ai GameLib::storage)
gamelib_target_warnings(wayfarer_app)
# Wayfarer's own procedural shapes, embedded as SPIR-V (see shapes.hpp).
gamelib_add_shaders(wayfarer_app SHADERS
    ${PROJECT_SOURCE_DIR}/shaders/avatar.frag
    ${PROJECT_SOURCE_DIR}/shaders/backdrop.frag
    ${PROJECT_SOURCE_DIR}/shaders/boss.frag
    ${PROJECT_SOURCE_DIR}/shaders/building.frag
    ${PROJECT_SOURCE_DIR}/shaders/deco.frag
    ${PROJECT_SOURCE_DIR}/shaders/fx.frag
    ${PROJECT_SOURCE_DIR}/shaders/monster.frag
    ${PROJECT_SOURCE_DIR}/shaders/terrain.frag
    ${PROJECT_SOURCE_DIR}/shaders/ui.frag)
