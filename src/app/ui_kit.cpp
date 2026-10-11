#include "ui_kit.hpp"

#include <gamelib/draw/vector_text.hpp>
#include <gamelib/ui/item_grid.hpp>

#include <algorithm>
#include <format>

namespace wayfarer::app::ui {

void text(gamelib::draw::DrawList& list, const std::string_view str, const float x, const float y, const TextStyle& style)
{
    gamelib::draw::VectorTextStyle vs{.size = style.size, .color = style.color, .weight = style.weight, .align = style.align};
    if (style.outline) {
        vs.shadow = {.color = srgb(0x2A1E30, style.color.a * 0.9f), .offset = {0.0f, 0.0f}, .weight = style.weight + 2.6f};
    }
    gamelib::draw::drawVectorText(list, str, x, y, vs);
}

float textWidth(const std::string_view str, const float size)
{
    return gamelib::draw::vectorTextWidth(str, size);
}

std::vector<std::string> wrap(const std::string_view str, const float size, const float width)
{
    return gamelib::draw::wrapVectorText(str, size, width);
}

float paragraph(gamelib::draw::DrawList& list, const std::string_view str, const float x, const float y, const float width,
                const TextStyle& style, const float lineGap)
{
    float cy = y;
    for (const std::string& line : wrap(str, style.size, width)) {
        text(list, line, x, cy, style);
        cy += style.size * lineGap;
    }
    return cy - y;
}

Rect window(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const std::string_view title, const Color accent)
{
    const float bar = title.empty() ? 0.0f : 54.0f;
    addUi(list, shapes, Ui::Window, rect.expanded(10.0f), ui_color::kWindow, accent, bar, 0.0f);
    if (!title.empty()) {
        text(list, title, rect.center().x, rect.y + 12.0f,
             {.size = 32.0f, .color = srgb(0xFFFFFF), .weight = 1.7f, .align = Align::Center, .outline = true});
    }
    return {rect.x + 18.0f, rect.y + bar + 14.0f, rect.width - 36.0f, rect.height - bar - 28.0f};
}

void well(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect)
{
    addUi(list, shapes, Ui::Window, rect.expanded(10.0f), srgb(0xF6EAD2), ui_color::kTitle, 0.0f, 2.0f);
}

void button(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const std::string_view label, const bool selected,
            const bool enabled, const Color fill)
{
    addUi(list, shapes, Ui::Button, rect, fill, ui_color::kGold, !enabled ? 3.0f : selected ? 1.0f : 0.0f);
    text(list, label, rect.center().x, rect.center().y - 15.0f,
         {.size = 26.0f, .color = enabled ? ui_color::kInk : ui_color::kSoftInk, .weight = 1.5f, .align = Align::Center});
}

void slot(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const gamelib::core::ItemStack& stack,
          const bool selected, const float time)
{
    const game::Rarity rarity = stack.empty() ? game::Rarity::Common : game::rarityOf(stack);
    const float glow = stack.empty() || rarity == game::Rarity::Common ? 0.0f : 0.6f + 0.15f * std::sin(time * 3.0f);
    addUi(list, shapes, Ui::Slot, rect, ui_color::kSlot, rarityColor(rarity), glow, selected ? 1.0f : 0.0f);
    if (stack.empty()) {
        return;
    }
    const IconLook icon = itemIcon(stack.item);
    const float bob = selected ? std::sin(time * 6.0f) * 2.0f : 0.0f;
    addIcon(list, shapes, Ui::ItemIcon, rect.center() + Vec2{0.0f, bob}, rect.height * 0.78f, icon.id, icon.style, icon.a, icon.b);
    if (stack.count > 1) {
        text(list, std::to_string(stack.count), rect.right() - 8.0f, rect.bottom() - 30.0f,
             {.size = 22.0f, .color = srgb(0xFFFFFF), .weight = 1.5f, .align = Align::Right, .outline = true});
    }
    const int up = game::upgradeOf(stack);
    if (up > 0) {
        text(list, "+" + std::to_string(up), rect.x + 8.0f, rect.y + 4.0f,
             {.size = 20.0f, .color = srgb(0xFFD86A), .weight = 1.6f, .outline = true});
    }
}

void keyCap(gamelib::draw::DrawList& list, const Shapes& shapes, const Vec2 center, const std::string_view label, const float height)
{
    const float size = height * 0.62f;
    const float w = std::max(height, textWidth(label, size) + height * 0.6f);
    addUi(list, shapes, Ui::KeyCap, {center.x - w * 0.5f, center.y - height * 0.5f, w, height}, srgb(0xF4ECDC), srgb(0xFFFFFF));
    text(list, label, center.x, center.y - size * 0.62f, {.size = size, .color = ui_color::kInk, .weight = 1.6f, .align = Align::Center});
}

void gauge(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const float fill, const float trail, const Color color,
           const Color trailColor)
{
    addUi(list, shapes, Ui::Gauge, rect, color, trailColor, std::clamp(fill, 0.0f, 1.0f), std::clamp(trail, 0.0f, 1.0f));
}

std::string groupDigits(const long long value)
{
    std::string s = std::to_string(value < 0 ? -value : value);
    for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) {
        s.insert(static_cast<std::size_t>(i), ",");
    }
    return value < 0 ? "-" + s : s;
}

float itemCard(gamelib::draw::DrawList& list, const Shapes& shapes, const Rect rect, const gamelib::core::ItemStack& stack,
               const game::Character& character, const bool compare, const float iconSize)
{
    if (stack.empty()) {
        return 0.0f;
    }
    const auto* def = game::itemRegistry().find(stack.item);
    if (def == nullptr) {
        return 0.0f;
    }
    const game::ItemInfo& info = game::itemInfo(stack.item);
    const game::Rarity rarity = game::rarityOf(stack);
    gamelib::ui::ItemCard card;
    card.name = def->name;
    if (game::upgradeOf(stack) > 0) {
        card.name += std::format(" +{}", game::upgradeOf(stack));
    }
    card.nameColor = rarity == game::Rarity::Common ? ui_color::kInk : rarityColor(rarity).scaled(0.85f);
    switch (info.kind) {
    case game::ItemKind::Equip:
        card.subtitle = std::format("{} {}  -  level {}", game::rarityName(rarity),
                                    game::slotName(static_cast<game::Slot>(def->equipSlot)), def->levelRequirement);
        break;
    case game::ItemKind::Use: card.subtitle = "consumable"; break;
    case game::ItemKind::Material: card.subtitle = "material"; break;
    case game::ItemKind::Quest: card.subtitle = "quest item"; break;
    }
    card.subtitleWarning = info.kind == game::ItemKind::Equip && def->levelRequirement > character.level;
    if (info.kind == game::ItemKind::Equip) {
        // Wayfarer's bonuses (base, rolled and upgrades folded in by stackBonus),
        // against the worn piece when comparing.
        const auto& worn = character.gear.at(def->equipSlot);
        for (int b = 0; b < game::kBonusCount; ++b) {
            const auto bonus = static_cast<game::Bonus>(b);
            if (bonus == game::Bonus::Upgrade) {
                continue;
            }
            const int value = game::stackBonus(stack, bonus);
            const int wornValue = compare && !worn.empty() && &worn != &stack ? game::stackBonus(worn, bonus) : value;
            if (value == 0 && wornValue == 0) {
                continue;
            }
            card.lines.push_back({.label = std::string{game::bonusName(bonus)}, .value = value,
                                  .difference = compare ? std::optional<int>{value - wornValue} : std::nullopt});
        }
    }
    card.text = info.text;
    const gamelib::ui::ItemCardStyle style{.nameFont = gamelib::draw::Font::vector(32.0f, 1.7f),
                                           .font = gamelib::draw::Font::vector(25.0f, 1.25f),
                                           .smallFont = gamelib::draw::Font::vector(22.0f, 1.2f),
                                           .text = ui_color::kInk,
                                           .dim = ui_color::kSoftInk,
                                           .warning = ui_color::kBad,
                                           .better = ui_color::kGood,
                                           .worse = ui_color::kBad,
                                           .gap = 14.0f,
                                           .iconSize = iconSize};
    if (iconSize > 0.0f) {
        const Rect cell = gamelib::ui::itemCardIconRect(rect, style);
        const IconLook icon = itemIcon(stack.item);
        addIcon(list, shapes, Ui::ItemIcon, cell.center(), iconSize, icon.id, icon.style, icon.a, icon.b);
    }
    return gamelib::ui::drawItemCard(list, card, rect, style);
}

} // namespace wayfarer::app::ui
