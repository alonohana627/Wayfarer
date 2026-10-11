#pragma once

// Wayfarer's controls as GameLib actions (platform::ActionMap): rebindable on
// the CONTROLS screen, kept in the profile's [keys] section, the keyboard
// and any gamepad driving the same actions. Pause (Escape, P, Start) is
// locked: ArcadeMenus owns it.

#include "rules.hpp"

#include <gamelib/platform/actions.hpp>
#include <gamelib/platform/input.hpp>
#include <gamelib/ui/menu.hpp>

#include <cstdint>

namespace wayfarer::app {

enum class Action : std::uint8_t {
    Left, Right, Up, Down, Jump, Attack, Roll, Skill1, Skill2, Skill3, Skill4, PotionHp, PotionMp,
    Stats, Skills, Inventory, Quests, Minimap, Pause, Count
};

[[nodiscard]] gamelib::platform::ActionMap makeActions();

[[nodiscard]] constexpr gamelib::platform::ActionId actionId(const Action action)
{
    return static_cast<gamelib::platform::ActionId>(action);
}

// The rules' input for this tick from the held and pressed actions.
[[nodiscard]] game::Input readInput(const gamelib::platform::ActionMap& actions, const gamelib::platform::InputState& input);

// The player's own keys on screens that are not a ui::Menu: directions move,
// jump and attack confirm, pause backs out.
[[nodiscard]] const gamelib::ui::MenuBindings& menuBindings();

} // namespace wayfarer::app
