#include "actions.hpp"

namespace wayfarer::app {

namespace {

// helpers

using gamelib::platform::GamepadButton;
using gamelib::platform::Key;

} // namespace

gamelib::platform::ActionMap makeActions()
{
    gamelib::platform::ActionMap map;
    map.add({.name = "left", .label = "LEFT", .category = "MOVE", .keys = {Key::Left},
             .buttons = {GamepadButton::DpadLeft, GamepadButton::LeftStickLeft}});
    map.add({.name = "right", .label = "RIGHT", .category = "MOVE", .keys = {Key::Right},
             .buttons = {GamepadButton::DpadRight, GamepadButton::LeftStickRight}});
    map.add({.name = "up", .label = "UP / TALK", .category = "MOVE", .keys = {Key::Up},
             .buttons = {GamepadButton::DpadUp, GamepadButton::LeftStickUp}});
    map.add({.name = "down", .label = "DOWN", .category = "MOVE", .keys = {Key::Down},
             .buttons = {GamepadButton::DpadDown, GamepadButton::LeftStickDown}});
    map.add({.name = "jump", .label = "JUMP", .category = "ACTION", .keys = {Key::Z, Key::Space},
             .buttons = {GamepadButton::A}});
    map.add({.name = "attack", .label = "ATTACK", .category = "ACTION", .keys = {Key::X, Key::LeftControl},
             .buttons = {GamepadButton::X}});
    map.add({.name = "roll", .label = "DODGE ROLL", .category = "ACTION", .keys = {Key::LeftShift, Key::V},
             .buttons = {GamepadButton::B}});
    map.add({.name = "skill1", .label = "CRESCENT SWEEP", .category = "SKILLS", .keys = {Key::A},
             .buttons = {GamepadButton::RightBumper}});
    map.add({.name = "skill2", .label = "STARBOLT", .category = "SKILLS", .keys = {Key::S}, .buttons = {GamepadButton::Y}});
    map.add({.name = "skill3", .label = "TAILWIND", .category = "SKILLS", .keys = {Key::D},
             .buttons = {GamepadButton::RightTrigger}});
    map.add({.name = "skill4", .label = "BLOOM", .category = "SKILLS", .keys = {Key::F},
             .buttons = {GamepadButton::LeftBumper}});
    map.add({.name = "potion_hp", .label = "HP POTION", .category = "SKILLS", .keys = {Key::Q},
             .buttons = {GamepadButton::LeftTrigger}});
    map.add({.name = "potion_mp", .label = "MP POTION", .category = "SKILLS", .keys = {Key::W},
             .buttons = {GamepadButton::RightThumb}});
    map.add({.name = "stats", .label = "STATS", .category = "WINDOWS", .keys = {Key::C}});
    map.add({.name = "skills", .label = "SKILLS", .category = "WINDOWS", .keys = {Key::K}});
    map.add({.name = "inventory", .label = "INVENTORY", .category = "WINDOWS", .keys = {Key::I},
             .buttons = {GamepadButton::Back}});
    map.add({.name = "quests", .label = "QUEST LOG", .category = "WINDOWS", .keys = {Key::L}});
    map.add({.name = "minimap", .label = "MINIMAP", .category = "WINDOWS", .keys = {Key::M}});
    map.add({.name = "pause", .label = "PAUSE", .category = "SYSTEM", .keys = {Key::Escape, Key::P},
             .buttons = {GamepadButton::Start}, .locked = true});
    return map;
}

game::Input readInput(const gamelib::platform::ActionMap& actions, const gamelib::platform::InputState& input)
{
    game::Input in;
    const auto down = [&](const Action a) { return actions.isDown(input, actionId(a)); };
    const auto pressed = [&](const Action a) { return actions.wasPressed(input, actionId(a)); };
    in.move = (down(Action::Right) ? 1 : 0) - (down(Action::Left) ? 1 : 0);
    in.climb = (down(Action::Down) ? 1 : 0) - (down(Action::Up) ? 1 : 0);
    in.jumpPressed = pressed(Action::Jump);
    in.jumpHeld = in.jumpPressed || down(Action::Jump);
    in.attackPressed = pressed(Action::Attack);
    in.attackHeld = in.attackPressed || down(Action::Attack);
    // A held attack keeps the combo going, MapleStory-style.
    in.attackPressed = in.attackPressed || in.attackHeld;
    in.rollPressed = pressed(Action::Roll);
    in.upPressed = pressed(Action::Up);
    in.skillPressed[0] = pressed(Action::Skill1);
    in.skillPressed[1] = pressed(Action::Skill2);
    in.skillPressed[2] = pressed(Action::Skill3);
    in.skillPressed[3] = pressed(Action::Skill4);
    in.hpPotion = pressed(Action::PotionHp);
    in.mpPotion = pressed(Action::PotionMp);
    return in;
}

const gamelib::ui::MenuBindings& menuBindings()
{
    static const gamelib::ui::MenuBindings bindings{
        .up = {actionId(Action::Up)},
        .down = {actionId(Action::Down)},
        .left = {actionId(Action::Left)},
        .right = {actionId(Action::Right)},
        .confirm = {actionId(Action::Jump), actionId(Action::Attack)},
        .back = {actionId(Action::Pause)},
    };
    return bindings;
}

} // namespace wayfarer::app
