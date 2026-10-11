#pragma once

// Wayfarer's interface pieces, in canvas units: storybook windows with a
// title bar, smooth lowercase text (GameLib's vector font) with word wrap,
// pill buttons, item slots with rarity glows and counts, gauges, key caps
// and the item card (name in its rarity color, stats, what it changes).

#include "look.hpp"
#include "shapes.hpp"

#include "character.hpp"

#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/draw/text.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace wayfarer::app::ui {

using gamelib::core::Rect;
using gamelib::core::Vec2;
using gamelib::draw::Align;

struct TextStyle {
    float size = 28.0f;
    Color color = ui_color::kInk;
    float weight = 1.25f;
    Align align = Align::Left;
    bool outline = false; // a dark outline (text over the world)
};

void text(gamelib::draw::DrawList& list, std::string_view text, float x, float y, const TextStyle& style = {});
[[nodiscard]] float textWidth(std::string_view text, float size);
// Words wrapped to `width` (hard breaks on '\n').
[[nodiscard]] std::vector<std::string> wrap(std::string_view text, float size, float width);
// Wrapped text from (x, y); returns the height used.
float paragraph(gamelib::draw::DrawList& list, std::string_view text, float x, float y, float width, const TextStyle& style = {},
                float lineGap = 1.3f);

// A window with a title bar; returns the content rect (below the bar, inset).
Rect window(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, std::string_view title, Color accent = ui_color::kTitle);
void well(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect);
void button(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, std::string_view label, bool selected,
            bool enabled = true, Color fill = ui_color::kButton);
void slot(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, const gamelib::core::ItemStack& stack, bool selected,
          float time);
void keyCap(gamelib::draw::DrawList& list, const Shapes& shapes, Vec2 center, std::string_view label, float height = 34.0f);
void gauge(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, float fill, float trail, Color color, Color trailColor);

// The item card: name, kind, level, stats and bonus lines, the price and
// (for equipment) the difference to what is worn.
float itemCard(gamelib::draw::DrawList& list, const Shapes& shapes, Rect rect, const gamelib::core::ItemStack& stack,
               const game::Character& character, bool compare);

// "1 234" style numbers.
[[nodiscard]] std::string groupDigits(long long value);

} // namespace wayfarer::app::ui
