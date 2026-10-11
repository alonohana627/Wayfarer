#pragma once

// The game's own shader shapes, registered once, and small builders that
// pack each shader's fields into a draw::Sprite (see the shaders' headers).

#include "anime_look.hpp"

#include <gamelib/core/color.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/gfx/renderer.hpp>

namespace wayfarer::app {

struct Shapes {
    gamelib::draw::ShapeId avatar;
    gamelib::draw::ShapeId monster;
    gamelib::draw::ShapeId boss;
    gamelib::draw::ShapeId building;
    gamelib::draw::ShapeId deco;
    gamelib::draw::ShapeId terrain;
    gamelib::draw::ShapeId backdrop;
    gamelib::draw::ShapeId fx;
    gamelib::draw::ShapeId ui;

    [[nodiscard]] static Shapes registerWith(gamelib::gfx::Renderer& renderer);
};

// fx.frag kinds.
enum class Fx : std::uint8_t {
    Slash, Sweep, Star, Spark, Petal, Wind, Portal, Shockwave, RainWarn, Shot, LevelUp, Coin, LootGlow, Puff, Streak, Plus
};

// ui.frag kinds.
enum class Ui : std::uint8_t {
    Window, Button, Slot, Gauge, ItemIcon, SkillIcon, StatIcon, Marker, Digit, Coin, KeyCap, Frame, Arrow, Tooltip, Shadow
};

// A square fx sprite centered on `at`, `size` across.
void addFx(gamelib::draw::DrawList& list, const Shapes& shapes, Fx kind, gamelib::core::Vec2 at, float size,
           gamelib::core::Color core, gamelib::core::Color edge, float progress, float variant = 0.0f, float w = 0.0f,
           float rotation = 0.0f);

// A UI sprite filling `rect`.
void addUi(gamelib::draw::DrawList& list, const Shapes& shapes, Ui kind, gamelib::core::Rect rect, gamelib::core::Color a,
           gamelib::core::Color b, float y = 0.0f, float z = 0.0f, float w = 0.0f);

// A square icon (item, skill, stat, marker) centered on `at`.
void addIcon(gamelib::draw::DrawList& list, const Shapes& shapes, Ui kind, gamelib::core::Vec2 at, float size, float id,
             float style, gamelib::core::Color a, gamelib::core::Color b, float w = 0.0f);

// A decoration standing on `ground` (deco.frag), `height` tall, `width` wide.
void addDeco(gamelib::draw::DrawList& list, const Shapes& shapes, int kind, gamelib::core::Vec2 ground, float width, float height,
             gamelib::core::Color a, gamelib::core::Color b, float lit, float variant, float seed);

} // namespace wayfarer::app
