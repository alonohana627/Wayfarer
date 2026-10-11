#pragma once

// What Wayfarer keeps outside the save slots, in wayfarer.ini in the user
// data folder (WAYFARER_DATA_DIR moves it): GameLib's standard settings, the
// best results of finished journeys, the key bindings ([keys], GameLib's
// action-binding text), the slot played last and whether the minimap is big.

#include "actions.hpp"

#include <gamelib/app/arcade_profile.hpp>
#include <gamelib/platform/actions.hpp>
#include <gamelib/storage/ini.hpp>

#include <string_view>

namespace wayfarer::app {

struct Prefs {
    gamelib::platform::ActionMap actions = makeActions();
    int lastSlot = 0; // 0..2
    bool bigMinimap = false;

    void read(const gamelib::storage::IniFile& ini);
    void write(gamelib::storage::IniFile& ini) const;
    friend bool operator==(const Prefs&, const Prefs&) = default;
};

using ProfileFile = gamelib::app::ArcadeProfileFileOf<Prefs>;

[[nodiscard]] ProfileFile openProfile();

inline constexpr std::string_view kAppName = "Wayfarer";
inline constexpr std::string_view kDataDirVariable = "WAYFARER_DATA_DIR";
// Bests: the fastest finished journey (ticks, lower is better).
inline constexpr std::string_view kBestTime = "journey_ticks";

} // namespace wayfarer::app
