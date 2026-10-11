// Wayfarer - a small action-RPG platformer on GameLib.
//
// Command line: --demo (the demo bot plays full screen) plus GameLib's
// self-test switches (--frames N --hidden --require-validation --no-vsync
// --allow-software --resize-at-frame N --minimize-at-frame N
// --focus-loss-at-frame N --keep-running).
//
// Developer aids (environment): WAYFARER_SCENE=sheet shows the character
// construction sheets; otherwise a comma list starts slot play there, e.g.
// "chapter3,level15,map9,night,gear,stats" (see WayfarerGame::applyDevScene);
// WAYFARER_START_LEVEL=N raises the hero to level N.
#include "profile.hpp"
#include "wayfarer_game.hpp"

#include "rules.hpp"

#include <gamelib/app/app.hpp>

#include <memory>
#include <utility>

int main(int argc, char** argv)
{
    wayfarer::app::ProfileFile profile = wayfarer::app::openProfile();
    gamelib::app::AppConfig config{
        .title = "Wayfarer",
        .windowSize = {1280, 720},
        .canvas = {1920, 1080},
        .tick = std::chrono::nanoseconds{1'000'000'000 / wayfarer::game::kTickRate},
        .fullscreen = profile.value().settings.fullscreen,
        .f11TogglesFullscreen = false, // F11 goes through ArcadeMenus, so the setting is saved
        .pauseOnFocusLoss = true,
        .gameFlags = {"--demo"},
    };
    config.renderer.vsync = profile.value().settings.vsync;
    return gamelib::app::runMain(argc, argv, config, [&](gamelib::app::App& app) {
        return std::make_unique<wayfarer::app::WayfarerGame>(app, std::move(profile), app.hasFlag("--demo"));
    });
}
