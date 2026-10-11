#include "windows.hpp"

#include "actions.hpp"
#include "look.hpp"
#include "ui_kit.hpp"
#include "view.hpp"

#include <gamelib/core/easing.hpp>
#include <gamelib/ui/focus.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>

namespace wayfarer::app {

namespace {

// helpers

using gamelib::core::ItemStack;
using gamelib::core::Rect;
using gamelib::core::Vec2;
using gamelib::ui::MenuInput;
using ui::Align;

constexpr int kLayer = gamelib::draw::layers::Hud + 60;
constexpr int kGridColumns = 6;
constexpr float kSlotSize = 100.0f;

// The window's pop-in: scale from 0.92 and fade over a fifth of a second.
float popIn(const float age)
{
    return gamelib::core::easeOutBack(std::min(age / 0.2f, 1.0f));
}

Rect grid(const Vec2 origin, const int index)
{
    const int col = index % kGridColumns;
    const int row = index / kGridColumns;
    return {origin.x + static_cast<float>(col) * (kSlotSize + 10.0f), origin.y + static_cast<float>(row) * (kSlotSize + 10.0f), kSlotSize,
            kSlotSize};
}

// The inventory window: 5 gear slots (0..4) then the 24 bag slots (5..28).
constexpr Rect kInvWindow{300.0f, 110.0f, 1320.0f, 860.0f};
constexpr Vec2 kInvGrid{720.0f, 200.0f};

std::array<Rect, game::kSlotCount + game::kBagSlots> inventoryRects()
{
    std::array<Rect, game::kSlotCount + game::kBagSlots> r{};
    const float cx = 490.0f;
    r[static_cast<std::size_t>(game::Slot::Helmet)] = {cx - 50.0f, 190.0f, kSlotSize, kSlotSize};
    r[static_cast<std::size_t>(game::Slot::Weapon)] = {cx - 190.0f, 330.0f, kSlotSize, kSlotSize};
    r[static_cast<std::size_t>(game::Slot::Armor)] = {cx + 90.0f, 330.0f, kSlotSize, kSlotSize};
    r[static_cast<std::size_t>(game::Slot::Ring)] = {cx - 190.0f, 470.0f, kSlotSize, kSlotSize};
    r[static_cast<std::size_t>(game::Slot::Shoes)] = {cx + 90.0f, 470.0f, kSlotSize, kSlotSize};
    for (int i = 0; i < game::kBagSlots; ++i) {
        r[static_cast<std::size_t>(game::kSlotCount + i)] = grid(kInvGrid, i);
    }
    return r;
}

constexpr Rect kShopWindow{360.0f, 110.0f, 1200.0f, 860.0f};
constexpr int kShopRows = 8;

int dirX(const MenuInput i)
{
    return i == MenuInput::Left ? -1 : i == MenuInput::Right ? 1 : 0;
}

int dirY(const MenuInput i)
{
    return i == MenuInput::Up ? -1 : i == MenuInput::Down ? 1 : 0;
}

Color skillTint(const game::SkillId s, const bool second)
{
    switch (s) {
    case game::SkillId::Combo: return second ? srgb(0xFFF2B0) : srgb(0xF4C24E);
    case game::SkillId::HyperJump: return second ? srgb(0xBFF2FF) : srgb(0x5CA8FF);
    case game::SkillId::Roll: return second ? srgb(0xFFFFFF) : srgb(0x9AD86A);
    case game::SkillId::Sweep: return second ? srgb(0xFFF2B0) : srgb(0x9FE6FF);
    case game::SkillId::Starbolt: return second ? srgb(0xFFF2B0) : srgb(0xFFD86A);
    case game::SkillId::Tailwind: return second ? srgb(0xC8FFE8) : srgb(0x7FE0B0);
    case game::SkillId::Bloom: return second ? srgb(0xFFF2B0) : srgb(0xFF9EC8);
    case game::SkillId::Count: break;
    }
    return srgb(0xFFFFFF);
}

std::string_view coreText(const game::Core c)
{
    switch (c) {
    case game::Core::Str: return "Strength makes every swing hit harder.";
    case game::Core::Agi: return "Agility raises critical chance, Speed and Jump, and adds a little attack.";
    case game::Core::Vit: return "Vitality raises max HP and defense.";
    case game::Core::Mag: return "Magic powers Starbolt and Bloom and raises max MP.";
    case game::Core::Count: break;
    }
    return "";
}

} // namespace

void Windows::open(const WindowId id)
{
    m_open = id;
    m_cursor = 0;
    m_tab = 0;
    m_scroll = 0;
    m_age = 0.0f;
    m_flash.clear();
}

std::vector<int> Windows::smithTargets(const game::Session& s) const
{
    std::vector<int> t;
    const game::Character& c = s.character();
    for (int i = 0; i < c.gear.size(); ++i) {
        if (!c.gear.at(i).empty()) {
            t.push_back(i);
        }
    }
    for (int i = 0; i < c.bag.size(); ++i) {
        if (!c.bag.at(i).empty() && game::itemInfo(c.bag.at(i).item).kind == game::ItemKind::Equip) {
            t.push_back(-1 - i);
        }
    }
    return t;
}

WindowFeedback Windows::handleKey(const WindowKey key, game::Session& s)
{
    if (key == WindowKey::Auto && m_open == WindowId::Stats) {
        if (s.autoAssign() > 0) {
            m_flash.add({.row = 4}, 0.4f);
            return WindowFeedback::Big;
        }
        return WindowFeedback::Failed;
    }
    if ((key == WindowKey::QuickHp || key == WindowKey::QuickMp) && m_open == WindowId::Inventory && m_cursor >= game::kSlotCount) {
        return s.assignQuick(m_cursor - game::kSlotCount) ? WindowFeedback::Done : WindowFeedback::Failed;
    }
    if (key == WindowKey::Tab && m_open == WindowId::Shop) {
        m_tab = 1 - m_tab;
        m_cursor = 0;
        m_scroll = 0;
        return WindowFeedback::Tab;
    }
    return WindowFeedback::None;
}

WindowFeedback Windows::handle(const MenuInput input, game::Session& s)
{
    if (m_open == WindowId::None) {
        return WindowFeedback::None;
    }
    if (input == MenuInput::Back) {
        close();
        return WindowFeedback::Closed;
    }
    const game::Character& c = s.character();
    switch (m_open) {
    case WindowId::Stats: {
        if (dirY(input) != 0) {
            m_cursor = (m_cursor + dirY(input) + 5) % 5;
            return WindowFeedback::Moved;
        }
        if (input == MenuInput::Confirm) {
            if (m_cursor == 4) {
                return handleKey(WindowKey::Auto, s);
            }
            if (s.spendPoint(static_cast<game::Core>(m_cursor))) {
                m_flash.add({.row = m_cursor}, 0.35f);
                return WindowFeedback::Done;
            }
            return WindowFeedback::Failed;
        }
        break;
    }
    case WindowId::Skills: {
        if (dirY(input) != 0) {
            m_cursor = (m_cursor + dirY(input) + game::kSkillCount) % game::kSkillCount;
            return WindowFeedback::Moved;
        }
        if (input == MenuInput::Confirm) {
            const auto skill = static_cast<game::SkillId>(m_cursor);
            if (s.learnSkill(skill) == gamelib::core::LearnResult::Learned) {
                m_flash.add({.row = m_cursor}, 0.45f);
                return WindowFeedback::Big;
            }
            return WindowFeedback::Failed;
        }
        break;
    }
    case WindowId::Inventory: {
        if (dirX(input) != 0 || dirY(input) != 0) {
            const auto rects = inventoryRects();
            const int next = gamelib::ui::stepFocus(rects, m_cursor, dirX(input), dirY(input), false);
            if (next != m_cursor) {
                m_cursor = next;
                return WindowFeedback::Moved;
            }
            return WindowFeedback::None;
        }
        if (input == MenuInput::Confirm) {
            bool ok = false;
            if (m_cursor < game::kSlotCount) {
                ok = s.unequip(static_cast<game::Slot>(m_cursor));
            } else {
                ok = s.useFromBag(m_cursor - game::kSlotCount);
            }
            if (ok) {
                m_flash.add({.row = m_cursor}, 0.3f);
            }
            return ok ? WindowFeedback::Done : WindowFeedback::Failed;
        }
        break;
    }
    case WindowId::Quests: {
        const int n = static_cast<int>(s.quests().log().size());
        if (dirY(input) != 0 && n > 0) {
            m_cursor = (m_cursor + dirY(input) + n) % n;
            return WindowFeedback::Moved;
        }
        break;
    }
    case WindowId::Shop: {
        if (input == MenuInput::Left || input == MenuInput::Right) {
            if (m_tab == 0) {
                return handleKey(WindowKey::Tab, s);
            }
            const int next = std::clamp(m_cursor + dirX(input), 0, game::kBagSlots - 1);
            if (m_cursor % kGridColumns == (input == MenuInput::Left ? 0 : kGridColumns - 1)) {
                return handleKey(WindowKey::Tab, s);
            }
            m_cursor = next;
            return WindowFeedback::Moved;
        }
        if (m_tab == 0) {
            const int n = static_cast<int>(s.shopEntries().size());
            if (dirY(input) != 0 && n > 0) {
                m_cursor = (m_cursor + dirY(input) + n) % n;
                if (m_cursor < m_scroll) {
                    m_scroll = m_cursor;
                } else if (m_cursor >= m_scroll + kShopRows) {
                    m_scroll = m_cursor - kShopRows + 1;
                }
                return WindowFeedback::Moved;
            }
            if (input == MenuInput::Confirm) {
                const bool ok = s.buy(m_cursor, 1) == gamelib::core::TradeResult::Done;
                if (ok) {
                    m_flash.add({.row = m_cursor}, 0.3f);
                }
                return ok ? WindowFeedback::Done : WindowFeedback::Failed;
            }
        } else {
            if (dirY(input) != 0) {
                const int next = m_cursor + dirY(input) * kGridColumns;
                if (next >= 0 && next < game::kBagSlots) {
                    m_cursor = next;
                    return WindowFeedback::Moved;
                }
                return WindowFeedback::None;
            }
            if (input == MenuInput::Confirm) {
                const bool ok = s.sellFromBag(m_cursor);
                if (ok) {
                    m_flash.add({.row = m_cursor}, 0.3f);
                }
                return ok ? WindowFeedback::Done : WindowFeedback::Failed;
            }
        }
        break;
    }
    case WindowId::Smith: {
        const std::vector<int> targets = smithTargets(s);
        const int n = static_cast<int>(targets.size());
        if (dirY(input) != 0 && n > 0) {
            m_cursor = (m_cursor + dirY(input) + n) % n;
            if (m_cursor < m_scroll) {
                m_scroll = m_cursor;
            } else if (m_cursor >= m_scroll + 7) {
                m_scroll = m_cursor - 6;
            }
            return WindowFeedback::Moved;
        }
        if (input == MenuInput::Confirm && m_cursor < n) {
            if (s.upgrade(targets[static_cast<std::size_t>(m_cursor)])) {
                m_flash.add({.row = m_cursor}, 0.6f);
                return WindowFeedback::Big;
            }
            return WindowFeedback::Failed;
        }
        break;
    }
    case WindowId::None: break;
    }
    (void)c;
    return WindowFeedback::None;
}

void Windows::draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s,
                   const gamelib::platform::ActionMap& actions, const gamelib::core::Size canvas, const float time) const
{
    if (m_open == WindowId::None) {
        return;
    }
    list.setLayer(kLayer - 1);
    // A soft veil over the paused world.
    list.box({0.0f, 0.0f, static_cast<float>(canvas.width), static_cast<float>(canvas.height)},
             srgb(0x1A1428, 0.35f * std::min(m_age / 0.15f, 1.0f)));
    list.setLayer(kLayer);
    switch (m_open) {
    case WindowId::Stats: drawStats(list, shapes, s, time); break;
    case WindowId::Skills: drawSkills(list, shapes, s, actions, time); break;
    case WindowId::Inventory: drawInventory(list, shapes, s, time); break;
    case WindowId::Quests: drawQuests(list, shapes, s, time); break;
    case WindowId::Shop: drawShop(list, shapes, s, time); break;
    case WindowId::Smith: drawSmith(list, shapes, s, time); break;
    case WindowId::None: break;
    }
}

void Windows::drawStats(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s, const float time) const
{
    const game::Character& c = s.character();
    const float pop = popIn(m_age);
    const Rect win{430.0f, 120.0f + (1.0f - pop) * 30.0f, 1060.0f, 820.0f};
    const Rect in = ui::window(list, shapes, win, "character");
    // The portrait.
    const Rect portrait{in.x + 10.0f, in.y + 10.0f, 330.0f, 420.0f};
    ui::well(list, shapes, portrait);
    anime::Avatar a = heroAvatar(c);
    a.feet = {portrait.center().x, portrait.bottom() - 28.0f};
    a.height = 430.0f;
    a.facing = 1.0f;
    a.pose = anime::AvatarPose::Idle;
    a.expression = anime::Expression::Happy;
    a.nextExpression = anime::Expression::Happy;
    list.setLayer(kLayer + 1);
    list.add(anime::avatarSprite(a, shapes.avatar));
    list.setLayer(kLayer + 2);
    ui::text(list, "wayfarer", portrait.center().x, portrait.bottom() + 16.0f,
             {.size = 34.0f, .color = ui_color::kInk, .weight = 1.8f, .align = Align::Center});
    ui::text(list, std::format("level {}", c.level), portrait.center().x, portrait.bottom() + 58.0f,
             {.size = 28.0f, .color = ui_color::kSoftInk, .weight = 1.4f, .align = Align::Center});
    const Rect xp{portrait.x + 10.0f, portrait.bottom() + 100.0f, portrait.width - 20.0f, 22.0f};
    ui::gauge(list, shapes, xp, c.xpProgress(), 0.0f, ui_color::kXp, srgb(0xFFF2C0));
    ui::text(list, c.level >= game::kMaxLevel ? "max level" : std::format("{} exp to go", ui::groupDigits(c.xpToNext())),
             portrait.center().x, xp.bottom() + 8.0f, {.size = 22.0f, .color = ui_color::kSoftInk, .align = Align::Center});

    // The core stats.
    const float x0 = portrait.right() + 40.0f;
    const float w = in.right() - x0;
    float y = in.y + 4.0f;
    ui::text(list, "ability points", x0, y, {.size = 28.0f, .color = ui_color::kInk, .weight = 1.6f});
    const float pulse = c.abilityPoints > 0 ? 1.0f + 0.08f * std::sin(time * 6.0f) : 1.0f;
    ui::text(list, std::to_string(c.abilityPoints), in.right() - 10.0f, y - (pulse - 1.0f) * 20.0f,
             {.size = 34.0f * pulse, .color = c.abilityPoints > 0 ? srgb(0xE0901E) : ui_color::kSoftInk, .weight = 2.0f, .align = Align::Right});
    y += 56.0f;
    const game::Derived now = s.derived();
    game::CoreStats preview = c.base;
    if (m_cursor < 4) {
        ++preview[static_cast<game::Core>(m_cursor)];
    }
    const game::Derived next = c.derivedWith(preview);
    for (int i = 0; i < game::kCoreCount; ++i) {
        const auto core = static_cast<game::Core>(i);
        const Rect row{x0, y, w, 70.0f};
        const bool sel = m_cursor == i;
        float bounce = 0.0f;
        for (const auto& f : m_flash) {
            if (f.value.row == i) {
                bounce = std::sin(f.progress() * 3.14159f) * 8.0f;
            }
        }
        if (sel) {
            addUi(list, shapes, Ui::Button, row.expanded(4.0f), srgb(0xFFF0CC), ui_color::kGold, 1.0f);
        }
        addIcon(list, shapes, Ui::StatIcon, {row.x + 38.0f, row.center().y - bounce}, 56.0f, static_cast<float>(i), 0.0f, ui_color::kWhite,
                ui_color::kWhite);
        ui::text(list, game::coreName(core), row.x + 80.0f, row.y + 16.0f, {.size = 30.0f, .color = ui_color::kInk, .weight = 1.5f});
        const int gearBonus = now.core[core] - c.base[core];
        const std::string value = gearBonus > 0 ? std::format("{} (+{})", c.base[core], gearBonus) : std::to_string(c.base[core]);
        ui::text(list, value, row.right() - 90.0f, row.y + 14.0f - bounce, {.size = 32.0f, .color = ui_color::kInk, .weight = 1.8f, .align = Align::Right});
        // The "+" button.
        const bool can = c.abilityPoints > 0;
        addUi(list, shapes, Ui::Button, {row.right() - 66.0f, row.y + 10.0f, 56.0f, 50.0f}, can ? srgb(0xFFD86A) : srgb(0xE8E0D0),
              ui_color::kGold, can ? (sel ? 1.0f : 0.0f) : 3.0f);
        ui::text(list, "+", row.right() - 38.0f, row.y + 8.0f, {.size = 40.0f, .color = can ? ui_color::kInk : ui_color::kSoftInk, .weight = 2.4f, .align = Align::Center});
        y += 78.0f;
    }
    const Rect autoBtn{x0, y + 6.0f, w, 62.0f};
    ui::button(list, shapes, autoBtn, "auto-assign", m_cursor == 4, c.abilityPoints > 0);
    y += 92.0f;

    // Derived values with the preview of a point on the selected stat.
    const auto line = [&](const std::string_view name, const int a, const int b, const bool percent, const float lx, const float ly) {
        ui::text(list, name, lx, ly, {.size = 26.0f, .color = ui_color::kSoftInk, .weight = 1.3f});
        const std::string v = percent ? std::format("{}%", a) : ui::groupDigits(a);
        ui::text(list, v, lx + 300.0f, ly, {.size = 28.0f, .color = ui_color::kInk, .weight = 1.6f, .align = Align::Right});
        if (b != a && c.abilityPoints > 0 && m_cursor < 4) {
            const std::string d = percent ? std::format("> {}%", b) : std::format("> {}", ui::groupDigits(b));
            ui::text(list, d, lx + 312.0f, ly, {.size = 24.0f, .color = ui_color::kGood, .weight = 1.6f});
        }
    };
    const float colB = x0 + w * 0.5f + 10.0f;
    line("max hp", now.maxHp, next.maxHp, false, x0, y);
    line("max mp", now.maxMp, next.maxMp, false, colB, y);
    y += 40.0f;
    line("attack", now.attack, next.attack, false, x0, y);
    line("magic", now.magic, next.magic, false, colB, y);
    y += 40.0f;
    line("defense", now.defense, next.defense, false, x0, y);
    line("critical", now.crit, next.crit, true, colB, y);
    y += 40.0f;
    line("speed", now.speed, next.speed, true, x0, y);
    line("jump", now.jump, next.jump, true, colB, y);
    y += 52.0f;
    // What the selected stat does.
    const std::string_view about = m_cursor < 4 ? coreText(static_cast<game::Core>(m_cursor)) : "Spend every point the balanced way: STR 2 : AGI 1 : VIT 1 : MAG 1.";
    ui::paragraph(list, about, x0, y, w, {.size = 24.0f, .color = ui_color::kSoftInk});
}

void Windows::drawSkills(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s,
                         const gamelib::platform::ActionMap& actions, const float time) const
{
    const game::Character& c = s.character();
    const float pop = popIn(m_age);
    const Rect win{470.0f, 110.0f + (1.0f - pop) * 30.0f, 980.0f, 860.0f};
    const Rect in = ui::window(list, shapes, win, "skills");
    ui::text(list, "skill points", in.x + 10.0f, in.y, {.size = 28.0f, .color = ui_color::kInk, .weight = 1.6f});
    ui::text(list, std::to_string(c.skills.points()), in.right() - 10.0f, in.y,
             {.size = 34.0f, .color = c.skills.points() > 0 ? srgb(0xE0901E) : ui_color::kSoftInk, .weight = 2.0f, .align = Align::Right});
    float y = in.y + 54.0f;
    for (int i = 0; i < game::kSkillCount; ++i) {
        const auto skill = static_cast<game::SkillId>(i);
        const int level = c.skills.levelOf(i);
        const int max = c.skills.maxLevelOf(i);
        const Rect row{in.x, y, in.width, 74.0f};
        const bool sel = m_cursor == i;
        float bounce = 0.0f;
        for (const auto& f : m_flash) {
            if (f.value.row == i) {
                bounce = std::sin(f.progress() * 3.14159f) * 10.0f;
            }
        }
        if (sel) {
            addUi(list, shapes, Ui::Button, row.expanded(3.0f), srgb(0xFFF0CC), ui_color::kGold, 1.0f);
        }
        addUi(list, shapes, Ui::Slot, {row.x + 6.0f, row.y + 3.0f, 68.0f, 68.0f}, srgb(0x5A4E70), srgb(0xFFFFFF), 0.0f, 0.0f);
        addIcon(list, shapes, Ui::SkillIcon, {row.x + 40.0f, row.center().y - bounce}, 58.0f, static_cast<float>(i), 0.0f,
                skillTint(skill, false), skillTint(skill, true), level > 0 ? 0.0f : 1.0f);
        ui::text(list, game::skillName(skill), row.x + 92.0f, row.y + 8.0f,
                 {.size = 30.0f, .color = level > 0 ? ui_color::kInk : ui_color::kSoftInk, .weight = 1.6f});
        const std::string req = c.level < game::skillUnlockLevel(skill) ? std::format("from level {}", game::skillUnlockLevel(skill))
                                : (skill == game::SkillId::HyperJump && level == 0) ? "taught by Captain Dorran"
                                                                                    : std::string{};
        ui::text(list, req.empty() ? std::format("lv {} / {}", level, max) : req, row.x + 92.0f, row.y + 44.0f,
                 {.size = 22.0f, .color = req.empty() ? ui_color::kSoftInk : srgb(0xC06060)});
        // The hotkey or the way it is used.
        std::string key;
        if (skill >= game::SkillId::Sweep) {
            key = actions.keyLabel(actionId(static_cast<Action>(static_cast<int>(Action::Skill1) + (i - static_cast<int>(game::SkillId::Sweep)))));
        } else if (skill == game::SkillId::Roll) {
            key = actions.keyLabel(actionId(Action::Roll));
        } else if (skill == game::SkillId::HyperJump) {
            key = actions.keyLabel(actionId(Action::Jump)) + " x2";
        } else {
            key = actions.keyLabel(actionId(Action::Attack));
        }
        ui::keyCap(list, shapes, {row.right() - 150.0f, row.center().y}, key, 38.0f);
        const bool can = s.canLearn(skill);
        addUi(list, shapes, Ui::Button, {row.right() - 66.0f, row.y + 12.0f, 56.0f, 50.0f}, can ? srgb(0xFFD86A) : srgb(0xE8E0D0),
              ui_color::kGold, can ? (sel ? 1.0f : 0.0f) : 3.0f);
        ui::text(list, "+", row.right() - 38.0f, row.y + 10.0f, {.size = 40.0f, .color = can ? ui_color::kInk : ui_color::kSoftInk, .weight = 2.4f, .align = Align::Center});
        y += 80.0f;
    }
    // The selected skill.
    const auto skill = static_cast<game::SkillId>(m_cursor);
    const int level = c.skills.levelOf(m_cursor);
    const Rect detail{in.x, y + 10.0f, in.width, in.bottom() - y - 10.0f};
    ui::well(list, shapes, detail);
    float dy = detail.y + 8.0f;
    dy += ui::paragraph(list, game::skillDescription(skill), detail.x + 14.0f, dy, detail.width - 28.0f,
                        {.size = 25.0f, .color = ui_color::kInk, .weight = 1.3f});
    dy += 8.0f;
    if (level > 0) {
        ui::text(list, "now:  " + game::skillEffectText(skill, level), detail.x + 14.0f, dy, {.size = 23.0f, .color = ui_color::kSoftInk});
        dy += 32.0f;
    }
    if (level < c.skills.maxLevelOf(m_cursor)) {
        ui::text(list, "next:  " + game::skillEffectText(skill, level + 1), detail.x + 14.0f, dy, {.size = 23.0f, .color = ui_color::kGood});
    }
    (void)time;
}

void Windows::drawInventory(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s, const float time) const
{
    const game::Character& c = s.character();
    const float pop = popIn(m_age);
    const Rect win{kInvWindow.x, kInvWindow.y + (1.0f - pop) * 30.0f, kInvWindow.width, kInvWindow.height};
    (void)ui::window(list, shapes, win, "bag & equipment");
    const float lift = (1.0f - pop) * 30.0f;
    const auto rects = inventoryRects();
    // The equipment side: a portrait with the five slots round it.
    const Rect equip{kInvWindow.x + 30.0f, kInvWindow.y + 70.0f + lift, 380.0f, 540.0f};
    ui::well(list, shapes, equip);
    anime::Avatar a = heroAvatar(c);
    a.feet = {equip.center().x, equip.y + 470.0f};
    a.height = 300.0f;
    a.facing = 1.0f;
    a.pose = anime::AvatarPose::Idle;
    list.setLayer(kLayer + 1);
    list.add(anime::avatarSprite(a, shapes.avatar));
    list.setLayer(kLayer + 2);
    for (int i = 0; i < game::kSlotCount; ++i) {
        Rect r = rects[static_cast<std::size_t>(i)];
        r.y += lift;
        ui::slot(list, shapes, r, c.gear.at(i), m_cursor == i, time);
        if (c.gear.at(i).empty()) {
            ui::text(list, game::slotName(static_cast<game::Slot>(i)), r.center().x, r.center().y - 12.0f,
                     {.size = 20.0f, .color = ui_color::kSoftInk, .align = Align::Center});
        }
    }
    // The bag.
    for (int i = 0; i < game::kBagSlots; ++i) {
        Rect r = rects[static_cast<std::size_t>(game::kSlotCount + i)];
        r.y += lift;
        float bounce = 0.0f;
        for (const auto& f : m_flash) {
            if (f.value.row == game::kSlotCount + i) {
                bounce = std::sin(f.progress() * 3.14159f) * 6.0f;
            }
        }
        r.y -= bounce;
        ui::slot(list, shapes, r, c.bag.at(i), m_cursor == game::kSlotCount + i, time);
        const ItemStack& st = c.bag.at(i);
        if (!st.empty() && (st.item == c.quick[0] || st.item == c.quick[1])) {
            ui::text(list, st.item == c.quick[0] ? "Q" : "W", r.x + 8.0f, r.bottom() - 28.0f,
                     {.size = 20.0f, .color = srgb(0xFFE08A), .weight = 1.8f, .outline = true});
        }
    }
    // Coins.
    const float gridBottom = rects.back().bottom() + lift;
    addIcon(list, shapes, Ui::Coin, {kInvGrid.x + 26.0f, gridBottom + 40.0f}, 44.0f, 0.0f, 0.0f, srgb(0xF6C845), srgb(0xFFFFFF));
    ui::text(list, ui::groupDigits(c.coins), kInvGrid.x + 60.0f, gridBottom + 22.0f, {.size = 30.0f, .color = ui_color::kInk, .weight = 1.6f});
    // The selected item's card.
    const ItemStack& sel = m_cursor < game::kSlotCount ? c.gear.at(m_cursor) : c.bag.at(m_cursor - game::kSlotCount);
    const Rect card{equip.x, equip.bottom() + 24.0f, kInvWindow.width - 60.0f, win.bottom() - equip.bottom() - 54.0f};
    ui::well(list, shapes, card);
    if (!sel.empty()) {
        const IconLook icon = itemIcon(sel.item);
        addIcon(list, shapes, Ui::ItemIcon, {card.x + 60.0f, card.y + 60.0f}, 96.0f, icon.id, icon.style, icon.a, icon.b);
        ui::itemCard(list, shapes, {card.x + 130.0f, card.y + 10.0f, 560.0f, card.height - 20.0f}, sel, c, m_cursor >= game::kSlotCount);
        const game::ItemInfo& info = game::itemInfo(sel.item);
        std::string hint;
        if (m_cursor < game::kSlotCount) {
            hint = m_cursor == static_cast<int>(game::Slot::Weapon) ? "your trusty weapon" : "confirm: take it off";
        } else if (info.kind == game::ItemKind::Equip) {
            hint = "confirm: equip";
        } else if (info.kind == game::ItemKind::Use) {
            hint = (info.heal > 0 || info.mana > 0) ? "confirm: use    q / w: put on the quick slot" : "confirm: use";
        }
        ui::text(list, hint, card.right() - 20.0f, card.bottom() - 40.0f, {.size = 22.0f, .color = ui_color::kSoftInk, .align = Align::Right});
    } else {
        ui::text(list, "an empty slot", card.x + 30.0f, card.y + 30.0f, {.size = 26.0f, .color = ui_color::kSoftInk});
    }
}

void Windows::drawQuests(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s, const float time) const
{
    (void)time;
    const float pop = popIn(m_age);
    const Rect win{470.0f, 140.0f + (1.0f - pop) * 30.0f, 980.0f, 780.0f};
    const Rect in = ui::window(list, shapes, win, "quest log");
    const std::vector<int> log = s.quests().log();
    if (log.empty()) {
        ui::paragraph(list, "No quests yet. Talk to the people of Hearthvale - look for a ! over their heads.", in.x + 10.0f, in.y + 20.0f,
                      in.width - 20.0f, {.size = 28.0f, .color = ui_color::kSoftInk});
        return;
    }
    float y = in.y;
    for (int i = 0; i < static_cast<int>(log.size()) && i < 7; ++i) {
        const int id = log[static_cast<std::size_t>(i)];
        const auto* def = s.quests().find(id);
        const bool complete = s.quests().stateOf(id) == gamelib::core::QuestState::Complete;
        const Rect row{in.x, y, in.width, 66.0f};
        if (m_cursor == i) {
            addUi(list, shapes, Ui::Button, row.expanded(3.0f), srgb(0xFFF0CC), ui_color::kGold, 1.0f);
        }
        const bool main = id <= game::kQuestChapter5;
        addIcon(list, shapes, Ui::Marker, {row.x + 34.0f, row.center().y}, 50.0f, complete ? 2.0f : 1.0f, 0.0f,
                complete ? srgb(0x4CC86A) : main ? srgb(0xFFC23A) : srgb(0x8CCAD6), ui_color::kWhite);
        ui::text(list, game::questText(id).title, row.x + 72.0f, row.y + 6.0f, {.size = 28.0f, .color = ui_color::kInk, .weight = 1.6f});
        std::string progress = complete ? "complete - hand it in" : "";
        if (!complete && def != nullptr && !def->objectives.empty() && def->objectives.front().required > 1) {
            progress = std::format("{} / {}", s.quests().progressOf(id, 0), def->objectives.front().required);
        }
        ui::text(list, progress, row.right() - 14.0f, row.y + 10.0f,
                 {.size = 24.0f, .color = complete ? ui_color::kGood : ui_color::kSoftInk, .weight = 1.4f, .align = Align::Right});
        ui::text(list, main ? "story" : "side quest", row.x + 72.0f, row.y + 40.0f, {.size = 18.0f, .color = ui_color::kSoftInk});
        y += 74.0f;
    }
    const int sel = std::clamp(m_cursor, 0, static_cast<int>(log.size()) - 1);
    const int id = log[static_cast<std::size_t>(sel)];
    const auto* def = s.quests().find(id);
    const Rect detail{in.x, in.bottom() - 180.0f, in.width, 180.0f};
    ui::well(list, shapes, detail);
    float dy = detail.y + 10.0f;
    dy += ui::paragraph(list, game::questText(id).summary, detail.x + 16.0f, dy, detail.width - 32.0f,
                        {.size = 26.0f, .color = ui_color::kInk, .weight = 1.4f});
    if (def != nullptr) {
        const int npc = def->turnInTo != 0 ? def->turnInTo : def->giver;
        ui::text(list, std::format("from {}  -  reward: {} exp, {} coins", game::npcName(static_cast<game::NpcId>(def->giver)),
                                   ui::groupDigits(def->reward.experience), ui::groupDigits(def->reward.coins)),
                 detail.x + 16.0f, dy + 10.0f, {.size = 22.0f, .color = ui_color::kSoftInk});
        ui::text(list, std::format("hand in to {}", game::npcName(static_cast<game::NpcId>(npc))), detail.x + 16.0f, dy + 44.0f,
                 {.size = 22.0f, .color = ui_color::kSoftInk});
    }
}

void Windows::drawShop(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s, const float time) const
{
    const game::Character& c = s.character();
    const float pop = popIn(m_age);
    const float lift = (1.0f - pop) * 30.0f;
    const Rect win{kShopWindow.x, kShopWindow.y + lift, kShopWindow.width, kShopWindow.height};
    const Rect in = ui::window(list, shapes, win, "pippa's general store", srgb(0xF2A0BE));
    // Tabs.
    ui::button(list, shapes, {in.x, in.y, 200.0f, 56.0f}, "buy", m_tab == 0, true, m_tab == 0 ? srgb(0xFFD86A) : ui_color::kButton);
    ui::button(list, shapes, {in.x + 214.0f, in.y, 200.0f, 56.0f}, "sell", m_tab == 1, true, m_tab == 1 ? srgb(0xFFD86A) : ui_color::kButton);
    addIcon(list, shapes, Ui::Coin, {in.right() - 230.0f, in.y + 28.0f}, 44.0f, 0.0f, 0.0f, srgb(0xF6C845), srgb(0xFFFFFF));
    ui::text(list, ui::groupDigits(c.coins), in.right() - 10.0f, in.y + 10.0f, {.size = 32.0f, .color = ui_color::kInk, .weight = 1.7f, .align = Align::Right});
    const float top = in.y + 80.0f;
    const Rect card{in.x + 700.0f, top, in.width - 700.0f, in.bottom() - top};
    ui::well(list, shapes, card);
    ItemStack selected;
    if (m_tab == 0) {
        const std::vector<gamelib::core::ShopEntry> entries = s.shopEntries();
        for (int i = m_scroll; i < static_cast<int>(entries.size()) && i < m_scroll + kShopRows; ++i) {
            const auto& e = entries[static_cast<std::size_t>(i)];
            const auto& def = game::itemRegistry().at(e.item);
            const Rect row{in.x, top + static_cast<float>(i - m_scroll) * 84.0f, 670.0f, 76.0f};
            const bool sel = i == m_cursor;
            if (sel) {
                addUi(list, shapes, Ui::Button, row, srgb(0xFFF0CC), ui_color::kGold, 1.0f);
            }
            addUi(list, shapes, Ui::Slot, {row.x + 6.0f, row.y + 4.0f, 68.0f, 68.0f}, ui_color::kSlot, ui_color::kWhite, 0.0f, 0.0f);
            const IconLook icon = itemIcon(e.item);
            addIcon(list, shapes, Ui::ItemIcon, {row.x + 40.0f, row.center().y}, 56.0f, icon.id, icon.style, icon.a, icon.b);
            ui::text(list, def.name, row.x + 92.0f, row.y + 10.0f, {.size = 28.0f, .color = ui_color::kInk, .weight = 1.5f});
            const int price = e.price >= 0 ? e.price : def.price;
            ui::text(list, ui::groupDigits(price), row.right() - 16.0f, row.y + 20.0f,
                     {.size = 28.0f, .color = price <= c.coins ? srgb(0xB07A1A) : ui_color::kBad, .weight = 1.6f, .align = Align::Right});
            if (def.levelRequirement > 1 && game::itemInfo(e.item).kind == game::ItemKind::Equip) {
                ui::text(list, std::format("lv {}", def.levelRequirement), row.x + 92.0f, row.y + 44.0f,
                         {.size = 20.0f, .color = def.levelRequirement > c.level ? ui_color::kBad : ui_color::kSoftInk});
            }
            if (sel) {
                selected = {.item = e.item, .count = 1};
            }
        }
        if (static_cast<int>(entries.size()) > kShopRows) {
            ui::text(list, std::format("{} / {}", m_cursor + 1, entries.size()), in.x + 335.0f, in.bottom() - 30.0f,
                     {.size = 20.0f, .color = ui_color::kSoftInk, .align = Align::Center});
        }
    } else {
        for (int i = 0; i < game::kBagSlots; ++i) {
            Rect r = grid({in.x + 10.0f, top}, i);
            ui::slot(list, shapes, r, c.bag.at(i), m_cursor == i, time);
        }
        selected = c.bag.at(m_cursor);
    }
    if (!selected.empty()) {
        const float h = ui::itemCard(list, shapes, {card.x + 18.0f, card.y + 16.0f, card.width - 36.0f, card.height - 100.0f}, selected, c, true);
        (void)h;
        const game::ItemInfo& info = game::itemInfo(selected.item);
        if (m_tab == 1) {
            const int each = info.kind == game::ItemKind::Equip ? game::sellValue(selected) : game::itemRegistry().at(selected.item).sellPrice;
            ui::text(list, info.kind == game::ItemKind::Quest ? "not for sale" : std::format("sells for {}", ui::groupDigits(each * selected.count)),
                     card.x + 18.0f, card.bottom() - 70.0f, {.size = 26.0f, .color = srgb(0xB07A1A), .weight = 1.5f});
        }
    }
    ui::text(list, m_tab == 0 ? "confirm: buy one    left / right: sell" : "confirm: sell the stack", card.center().x, card.bottom() - 32.0f,
             {.size = 21.0f, .color = ui_color::kSoftInk, .align = Align::Center});
}

void Windows::drawSmith(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s, const float time) const
{
    const game::Character& c = s.character();
    const float pop = popIn(m_age);
    const Rect win{400.0f, 120.0f + (1.0f - pop) * 30.0f, 1120.0f, 840.0f};
    const Rect in = ui::window(list, shapes, win, "brann's forge", srgb(0xE09A6A));
    const int ore = c.bag.countOf(game::materialItem(game::Material::GlimmerOre));
    const IconLook oreIcon = itemIcon(game::materialItem(game::Material::GlimmerOre));
    addIcon(list, shapes, Ui::ItemIcon, {in.x + 26.0f, in.y + 26.0f}, 48.0f, oreIcon.id, oreIcon.style, oreIcon.a, oreIcon.b);
    ui::text(list, std::format("glimmer ore  {}", ore), in.x + 60.0f, in.y + 8.0f, {.size = 28.0f, .color = ui_color::kInk, .weight = 1.5f});
    addIcon(list, shapes, Ui::Coin, {in.right() - 230.0f, in.y + 26.0f}, 44.0f, 0.0f, 0.0f, srgb(0xF6C845), srgb(0xFFFFFF));
    ui::text(list, ui::groupDigits(c.coins), in.right() - 10.0f, in.y + 8.0f, {.size = 32.0f, .color = ui_color::kInk, .weight = 1.7f, .align = Align::Right});
    const std::vector<int> targets = smithTargets(s);
    const float top = in.y + 70.0f;
    for (int i = m_scroll; i < static_cast<int>(targets.size()) && i < m_scroll + 7; ++i) {
        const int t = targets[static_cast<std::size_t>(i)];
        const ItemStack& st = s.upgradeTarget(t);
        const Rect row{in.x, top + static_cast<float>(i - m_scroll) * 88.0f, 560.0f, 80.0f};
        const bool sel = i == m_cursor;
        float bounce = 0.0f;
        for (const auto& f : m_flash) {
            if (f.value.row == i) {
                bounce = std::sin(f.progress() * 3.14159f) * 12.0f;
            }
        }
        if (sel) {
            addUi(list, shapes, Ui::Button, row, srgb(0xFFF0CC), ui_color::kGold, 1.0f);
        }
        ui::slot(list, shapes, {row.x + 6.0f, row.y + 4.0f - bounce, 72.0f, 72.0f}, st, false, time);
        const auto& def = game::itemRegistry().at(st.item);
        const int up = game::upgradeOf(st);
        ui::text(list, up > 0 ? std::format("{} +{}", def.name, up) : def.name, row.x + 96.0f, row.y + 10.0f,
                 {.size = 28.0f, .color = game::rarityOf(st) == game::Rarity::Common ? ui_color::kInk : rarityColor(game::rarityOf(st)).scaled(0.85f), .weight = 1.6f});
        ui::text(list, t >= 0 ? "equipped" : "in your bag", row.x + 96.0f, row.y + 46.0f, {.size = 20.0f, .color = ui_color::kSoftInk});
    }
    const Rect detail{in.x + 590.0f, top, in.width - 590.0f, in.bottom() - top};
    ui::well(list, shapes, detail);
    if (targets.empty()) {
        ui::paragraph(list, "Hm. Bring me some gear to work on.", detail.x + 20.0f, detail.y + 20.0f, detail.width - 40.0f,
                      {.size = 28.0f, .color = ui_color::kSoftInk});
        return;
    }
    const int t = targets[static_cast<std::size_t>(std::clamp(m_cursor, 0, static_cast<int>(targets.size()) - 1))];
    const ItemStack& st = s.upgradeTarget(t);
    float dy = detail.y + 16.0f;
    dy += ui::itemCard(list, shapes, {detail.x + 18.0f, dy, detail.width - 36.0f, 400.0f}, st, c, false);
    dy += 20.0f;
    const auto cost = s.upgradeCostOf(t);
    if (!cost) {
        ui::text(list, "this piece is as good as it gets", detail.x + 18.0f, dy, {.size = 26.0f, .color = ui_color::kGood, .weight = 1.4f});
        return;
    }
    ItemStack next = st;
    game::applyUpgrade(next);
    const auto gain = [&](const game::Bonus b) { return game::stackBonus(next, b) - game::stackBonus(st, b); };
    std::string what;
    for (const game::Bonus b : {game::Bonus::Attack, game::Bonus::Magic, game::Bonus::Defense, game::Bonus::Hp}) {
        if (gain(b) > 0) {
            what += std::format("{}{} +{}", what.empty() ? "" : ",  ", game::bonusName(b), gain(b));
        }
    }
    ui::text(list, std::format("upgrade to +{}:  {}", game::upgradeOf(st) + 1, what), detail.x + 18.0f, dy,
             {.size = 25.0f, .color = ui_color::kGood, .weight = 1.5f});
    dy += 44.0f;
    ui::text(list, std::format("costs {} coins and {} glimmer ore", ui::groupDigits(cost->coins), cost->ore), detail.x + 18.0f, dy,
             {.size = 25.0f, .color = (c.coins >= cost->coins && ore >= cost->ore) ? ui_color::kInk : ui_color::kBad, .weight = 1.4f});
    ui::text(list, "confirm: upgrade", detail.center().x, detail.bottom() - 40.0f, {.size = 22.0f, .color = ui_color::kSoftInk, .align = Align::Center});
}

} // namespace wayfarer::app
