// The screens around the world: the title with the demo behind it, the
// menus and the controls list drawn in the storybook style, the save slots,
// the fainting overlay, the ending with the journey's numbers and the
// credits, and the developer's construction sheets of every character.

#include "wayfarer_game.hpp"

#include "look.hpp"
#include "ui_kit.hpp"

#include <gamelib/core/easing.hpp>
#include <gamelib/core/timers.hpp>
#include <gamelib/fx/juice.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <format>

namespace wayfarer::app {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;
using gamelib::app::ArcadeScreen;
using ui::Align;

constexpr int kScreenLayer = gamelib::draw::layers::Hud + 200;

std::string lower(std::string s)
{
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

} // namespace

void WayfarerGame::drawMenu(gamelib::draw::DrawList& list, const float time) const
{
    const gamelib::ui::Menu* menu = m_menus.menu();
    if (menu == nullptr || menu->items().empty()) {
        return;
    }
    using gamelib::draw::Font;
    const bool title = m_menus.is(ArcadeScreen::Title);
    // GameLib lays the menu out; the window, its title bar and the rows are
    // Wayfarer's shader frames. The menu's own title is laid out but not
    // drawn (transparent): the window's bar shows it (none on the title screen).
    constexpr float kTitleSize = 32.0f;
    const std::string barTitle = title ? std::string{} : lower(menu->title());
    gamelib::ui::MenuStyle style{
        .veil = title ? Color{0.0f, 0.0f, 0.0f, 0.0f} : srgb(0x1A1428, 0.45f),
        .title = srgb(0xFFFFFF, 0.0f),
        .text = ui_color::kSoftInk,
        .highlight = ui_color::kInk,
        .dim = ui_color::kSoftInk,
        .titleFont = Font::vector(title ? 0.01f : kTitleSize, 1.7f).lowered(),
        .itemFont = Font::vector(30.0f, 1.35f).lowered(),
        .selectedWeight = 1.7f / 1.35f,
        .itemHeight = 54.0f,
        .padding = title ? 14.0f : 12.0f,
        .minWidth = 520.0f,
        .titleGap = title ? 0.0f : 56.0f - 0.75f * kTitleSize,
        .top = title ? 520.0f : -1.0f,
        .rowInset = 18.0f,
        .labelIndent = 68.0f,
        .valueInset = 36.0f,
        .valueFont = Font::vector(28.0f, 1.5f).lowered(),
        .valueHighlight = srgb(0xB07A1A),
        .panelPainter =
            [this, title, &barTitle](gamelib::draw::DrawList& out, const Rect& rect) {
                if (title) {
                    addUi(out, m_shapes, Ui::Window, rect.expanded(10.0f), srgb(0xFFF7E8), ui_color::kTitle, 0.0f, 0.0f);
                } else {
                    (void)ui::window(out, m_shapes, rect, barTitle);
                }
            },
        .rowPainter =
            [this, time](gamelib::draw::DrawList& out, const Rect& row, const std::size_t, const bool selected) {
                if (!selected) {
                    return;
                }
                const Rect pill{row.x, row.y + 4.0f, row.width, row.height - 8.0f};
                addUi(out, m_shapes, Ui::Button, pill, ui_color::kButton, ui_color::kGold, 1.0f);
                addUi(out, m_shapes, Ui::Arrow, {pill.x + 14.0f + std::sin(time * 7.0f) * 3.0f, pill.center().y - 12.0f, 24.0f, 24.0f},
                      ui_color::kGold, ui_color::kGold);
            }};
    list.setLayer(kScreenLayer);
    gamelib::ui::drawMenu(list, *menu, m_canvas, style);
}

void WayfarerGame::drawControls(gamelib::draw::DrawList& list, const float time) const
{
    const gamelib::ui::BindingList* controls = m_menus.controls();
    if (controls == nullptr) {
        return;
    }
    list.setLayer(kScreenLayer + 5);
    list.box({0.0f, 0.0f, static_cast<float>(m_canvas.width), static_cast<float>(m_canvas.height)}, srgb(0x1A1428, 0.55f));
    const std::size_t rows = controls->rowCount();
    const float rowH = 40.0f;
    const float w = 980.0f;
    const float h = rowH * static_cast<float>(rows) + 110.0f;
    const Rect win{static_cast<float>(m_canvas.width) * 0.5f - w * 0.5f, std::max(30.0f, static_cast<float>(m_canvas.height) * 0.5f - h * 0.5f),
                   w, std::min(h, static_cast<float>(m_canvas.height) - 60.0f)};
    const Rect in = ui::window(list, m_shapes, win, "controls");
    for (std::size_t i = 0; i < rows; ++i) {
        const bool sel = i == controls->selected();
        const Rect row{in.x, in.y + static_cast<float>(i) * rowH, in.width, rowH - 4.0f};
        if (sel) {
            addUi(list, m_shapes, Ui::Button, row, controls->waiting() ? srgb(0xFFD86A) : ui_color::kButton, ui_color::kGold, 1.0f);
        }
        ui::text(list, lower(controls->label(i)), row.x + 24.0f, row.y + 4.0f, {.size = 24.0f, .color = ui_color::kInk, .weight = sel ? 1.6f : 1.3f});
        const std::string value = sel && controls->waiting() ? (std::fmod(time, 0.8f) < 0.5f ? "press a key..." : "") : lower(controls->value(i));
        ui::text(list, value, row.right() - 20.0f, row.y + 4.0f, {.size = 24.0f, .color = srgb(0x6A5A9A), .weight = 1.4f, .align = Align::Right});
    }
}

void WayfarerGame::drawTitle(gamelib::draw::DrawList& list, const float time) const
{
    const float W = static_cast<float>(m_canvas.width);
    list.setLayer(kScreenLayer - 10);
    // A soft vignette to lift the logo off the demo.
    addUi(list, m_shapes, Ui::Shadow, {W * 0.5f - 900.0f, -260.0f, 1800.0f, 900.0f}, srgb(0xFFF6E0, 0.55f), srgb(0xFFFFFF));
    const float bob = std::sin(time * 1.4f) * 6.0f;
    const float grow = gamelib::core::easeOutBack(std::min(time / 0.8f, 1.0f));
    list.setLayer(kScreenLayer);
    ui::text(list, "Wayfarer", W * 0.5f, 170.0f + bob, {.size = 190.0f * grow, .color = srgb(0xFFF4D0), .weight = 3.0f, .align = Align::Center, .outline = true});
    ui::text(list, "Wayfarer", W * 0.5f, 170.0f + bob, {.size = 190.0f * grow, .color = srgb(0xFFD86A, 0.55f), .weight = 1.4f, .align = Align::Center});
    ui::text(list, "a tale of hearthvale", W * 0.5f, 410.0f + bob * 0.5f,
             {.size = 40.0f, .color = srgb(0xFFFFFF), .weight = 1.5f, .align = Align::Center, .outline = true});
    // Sparkles round the logo.
    for (int i = 0; i < 6; ++i) {
        const float t = std::fmod(time * 0.5f + static_cast<float>(i) / 6.0f, 1.0f);
        const Vec2 at{W * 0.5f + std::cos(static_cast<float>(i) * 2.1f) * 520.0f, 230.0f + std::sin(static_cast<float>(i) * 1.7f) * 110.0f};
        addFx(list, m_shapes, Fx::Spark, at, 90.0f * (1.0f - t), srgb(0xFFFFFF).scaled(1.3f), srgb(0xFFE08A), t * 0.6f, 0.0f,
              static_cast<float>(i));
    }
    drawMenu(list, time);
    drawControls(list, time);
    ui::text(list, "enter: choose     esc: back     f11: fullscreen", W * 0.5f, static_cast<float>(m_canvas.height) - 52.0f,
             {.size = 22.0f, .color = srgb(0xFFFFFF, 0.85f), .weight = 1.3f, .align = Align::Center, .outline = true});
}

void WayfarerGame::drawSlots(gamelib::draw::DrawList& list, const float time) const
{
    (void)time;
    using gamelib::draw::Font;
    // GameLib's slot list with Wayfarer's window, rows and number frames.
    constexpr float kTitleSize = (68.0f - 1.5f * 20.0f) / 0.75f; // rows start under the window's title bar
    const int slots = m_slotList.slotCount();
    gamelib::ui::SaveSlotStyle style{
        .menu = {.veil = srgb(0x1A1428, 0.3f),
                 .title = srgb(0xFFFFFF, 0.0f),
                 .text = ui_color::kSoftInk,
                 .highlight = ui_color::kInk,
                 .dim = ui_color::kSoftInk,
                 .titleFont = Font::vector(kTitleSize, 1.7f),
                 .padding = 20.0f,
                 .top = 200.0f,
                 .rowInset = 18.0f,
                 .panelPainter = [this](gamelib::draw::DrawList& out,
                                        const Rect& rect) { (void)ui::window(out, m_shapes, rect, "choose your journey"); },
                 .rowPainter =
                     [this, slots](gamelib::draw::DrawList& out, const Rect& row, const std::size_t index, const bool selected) {
                         if (static_cast<int>(index) == slots) {
                             addUi(out, m_shapes, Ui::Button, {row.center().x - 140.0f, row.center().y - 31.0f, 280.0f, 62.0f},
                                   ui_color::kButton, ui_color::kGold, selected ? 1.0f : 0.0f);
                             return;
                         }
                         addUi(out, m_shapes, Ui::Button, row, selected ? srgb(0xFFF0CC) : srgb(0xF6EAD2), ui_color::kGold,
                               selected ? 1.0f : 0.0f);
                         addUi(out, m_shapes, Ui::Frame, {row.x + 20.0f, row.center().y - 44.0f, 88.0f, 88.0f},
                               selected ? srgb(0xFFD86A) : srgb(0xE8DCC4), ui_color::kWhite);
                         ui::text(out, std::to_string(index + 1), row.x + 64.0f, row.center().y - 28.0f,
                                  {.size = 44.0f, .color = ui_color::kInk, .weight = 2.0f, .align = Align::Center});
                         const auto& r = m_slotList.rows()[index];
                         if (selected && r.exists && r.readable && m_slotList.confirmingErase() != static_cast<int>(index)) {
                             ui::text(out, "left: erase", row.right() - 26.0f, row.center().y - 11.0f,
                                      {.size = 21.0f, .color = ui_color::kSoftInk, .align = Align::Right});
                         }
                     }},
        .danger = ui_color::kBad,
        .width = 1040.0f,
        .rowHeight = 128.0f,
        .nameFont = Font::vector(26.0f, 1.5f).lowered(),
        .lineFont = Font::vector(32.0f, 1.5f).lowered(),
        .textIndent = 158.0f};
    const gamelib::ui::SaveSlotLabels labels{.slot = "",
                                             .empty = "a new journey",
                                             .damaged = "this page is torn (damaged)",
                                             .erase = "erase this journey? press left again",
                                             .back = "back"};
    list.setLayer(kScreenLayer);
    gamelib::ui::drawSaveSlots(list, m_slotList, m_canvas, style, labels, "");
}

void WayfarerGame::drawFainted(gamelib::draw::DrawList& list, const float time) const
{
    if (!m_session || m_session->phase() != game::Phase::Dead) {
        return;
    }
    const float t = std::min(m_deadTime / 0.8f, 1.0f);
    list.setLayer(kScreenLayer - 20);
    list.box({0.0f, 0.0f, static_cast<float>(m_canvas.width), static_cast<float>(m_canvas.height)}, srgb(0x2A1E3A, 0.45f * t));
    ui::text(list, "you fainted...", static_cast<float>(m_canvas.width) * 0.5f, 420.0f - (1.0f - t) * 30.0f,
             {.size = 70.0f, .color = srgb(0xFFF2D8, t), .weight = 2.2f, .align = Align::Center, .outline = true});
    ui::text(list, "someone carries you home to the inn", static_cast<float>(m_canvas.width) * 0.5f, 520.0f,
             {.size = 32.0f, .color = srgb(0xFFFFFF, t * 0.9f), .weight = 1.4f, .align = Align::Center, .outline = true});
    (void)time;
}

void WayfarerGame::drawEnding(gamelib::draw::DrawList& list, const float time) const
{
    const float W = static_cast<float>(m_canvas.width);
    const float H = static_cast<float>(m_canvas.height);
    // A sunset over Hearthvale: sky, hills and everyone cheering.
    const RegionLook look = regionLook(game::Region::Town, false, 0.35f);
    list.setLayer(gamelib::draw::layers::Background);
    list.add({.rect = {0.0f, 0.0f, W, H}, .color = look.skyHorizon, .color2 = look.skyTop, .shape = m_shapes.backdrop,
              .params = {0.0f, 0.0f, 0.0f, 0.3f}, .bits = 0});
    list.add({.rect = {0.0f, 0.0f, W, H}, .color = look.cloud, .color2 = look.cloudShade, .shape = m_shapes.backdrop,
              .params = {time * 20.0f, 0.0f, 1.0f, 0.3f}, .bits = 0});
    list.setLayer(gamelib::draw::layers::Parallax);
    list.add({.rect = {0.0f, 0.0f, W, H}, .color = look.far, .color2 = look.farRim, .shape = m_shapes.backdrop,
              .params = {0.0f, 0.0f, 2.0f, 0.3f}, .bits = 0});
    list.add({.rect = {0.0f, 80.0f, W, H}, .color = look.near, .color2 = look.nearRim, .shape = m_shapes.backdrop,
              .params = {200.0f, 0.0f, 3.0f, 0.3f}, .bits = 0});
    // The ground.
    list.setLayer(gamelib::draw::layers::TilesBehind);
    list.add({.rect = {-10.0f, H - 150.0f, W + 20.0f, 220.0f}, .color = srgb(0x8FD46A), .color2 = srgb(0xB57E52), .shape = m_shapes.terrain,
              .params = {1.0f, 0.0f, 3.0f, 0.0f}});
    // Everyone together.
    list.setLayer(gamelib::draw::layers::Entities);
    const std::array npcs{game::NpcId::Nell, game::NpcId::Pippa, game::NpcId::Elder, game::NpcId::Count, game::NpcId::Dorran,
                          game::NpcId::Brann, game::NpcId::Lumi};
    float x = W * 0.5f - 3.0f * 190.0f;
    for (const game::NpcId n : npcs) {
        anime::Avatar a;
        if (n == game::NpcId::Count) {
            game::Character hero;
            a = heroAvatar(hero);
            if (m_session) {
                a = heroAvatar(m_session->character());
            }
        } else {
            a = npcAvatar(n);
        }
        const float hop = std::max(0.0f, std::sin(time * 4.0f + x * 0.01f)) * 24.0f;
        a.feet = {x, H - 140.0f - hop};
        a.height = 230.0f;
        a.facing = x < W * 0.5f ? 1.0f : -1.0f;
        a.pose = anime::AvatarPose::Cheer;
        a.expression = anime::Expression::Happy;
        a.nextExpression = anime::Expression::Happy;
        list.add(anime::avatarSprite(a, m_shapes.avatar));
        x += 190.0f;
    }
    // Fireworks.
    list.setLayer(gamelib::draw::layers::Effects);
    for (int i = 0; i < 5; ++i) {
        const float period = 2.2f + static_cast<float>(i) * 0.37f;
        const float t = std::fmod(time + static_cast<float>(i) * 0.71f, period) / period;
        const Vec2 at{W * (0.15f + 0.17f * static_cast<float>(i)), 160.0f + std::sin(static_cast<float>(i) * 2.3f) * 60.0f};
        const std::array<std::uint32_t, 5> colors{0xFF9EC8, 0xFFD86A, 0x9FE6FF, 0xB79CFF, 0x9AF0B0};
        addFx(list, m_shapes, Fx::Spark, at, 300.0f, srgb(0xFFFFFF).scaled(1.3f), srgb(colors[static_cast<std::size_t>(i)]).scaled(1.2f), t);
    }
    // The words and the numbers.
    list.setLayer(kScreenLayer);
    const float in = std::min(m_endingTime / 1.5f, 1.0f);
    ui::text(list, "the hush is lifted", W * 0.5f, 60.0f, {.size = 80.0f, .color = srgb(0xFFF4D0, in), .weight = 2.6f, .align = Align::Center, .outline = true});
    ui::text(list, "songs and colors return to every corner of the world", W * 0.5f, 170.0f,
             {.size = 32.0f, .color = srgb(0xFFFFFF, in), .weight = 1.4f, .align = Align::Center, .outline = true});
    const float statsIn = std::clamp((m_endingTime - 1.5f) / 1.0f, 0.0f, 1.0f);
    if (statsIn > 0.0f) {
        const Rect panel{W * 0.5f - 420.0f, 250.0f, 840.0f, 360.0f};
        addUi(list, m_shapes, Ui::Window, panel.expanded(10.0f), srgb(0xFFF7E8, 0.93f * statsIn), ui_color::kTitle, 0.0f, 0.0f);
        const std::array<std::pair<std::string, std::string>, 8> lines{{
            {"journey time", gamelib::core::formatClock(static_cast<int>(std::min<std::int64_t>(m_endStats.ticks, 359'999LL * 60)), game::kTickRate, 0)},
            {"level", std::to_string(m_endLevel)},
            {"monsters calmed", ui::groupDigits(m_endStats.defeated)},
            {"elites among them", ui::groupDigits(m_endStats.elites)},
            {"coins earned", ui::groupDigits(m_endStats.coinsEarned)},
            {"best hit", ui::groupDigits(m_endStats.bestHit)},
            {"potions sipped", ui::groupDigits(m_endStats.potions)},
            {"naps at the inn (faints)", ui::groupDigits(m_endStats.deaths)},
        }};
        float y = panel.y + 18.0f;
        for (const auto& [name, value] : lines) {
            ui::text(list, name, panel.x + 40.0f, y, {.size = 30.0f, .color = ui_color::kSoftInk.withAlpha(statsIn), .weight = 1.4f});
            ui::text(list, value, panel.right() - 40.0f, y, {.size = 32.0f, .color = ui_color::kInk.withAlpha(statsIn), .weight = 1.8f, .align = Align::Right});
            y += 42.0f;
        }
        if (m_newBest) {
            ui::text(list, "your fastest journey!", panel.center().x, panel.bottom() + 16.0f,
                     {.size = 30.0f, .color = srgb(0xFFD86A, statsIn), .weight = 1.8f, .align = Align::Center, .outline = true});
        }
    }
    // Credits scrolling up on the right.
    const std::array<std::string_view, 12> credits{
        "Wayfarer", "", "a small journey in five regions", "", "every picture drawn by shaders", "every note synthesized",
        "", "built on GameLib", "", "thank you for playing", "", "see you on the road!"};
    const float scroll = std::max(0.0f, m_endingTime - 3.0f) * 40.0f;
    float cy = H + 40.0f - scroll;
    for (const std::string_view c : credits) {
        if (cy > -60.0f && cy < H) {
            const float edge = std::min({1.0f, (cy + 60.0f) / 160.0f, (H - cy) / 160.0f});
            ui::text(list, c, W - 230.0f, cy, {.size = c == "Wayfarer" ? 48.0f : 26.0f, .color = srgb(0xFFFFFF, std::max(edge, 0.0f)), .weight = 1.5f,
                                               .align = Align::Center, .outline = true});
        }
        cy += 52.0f;
    }
    if (m_endingTime > kEndingInputDelay) {
        ui::text(list, "press a key to return to the title", W * 0.5f, H - 60.0f,
                 {.size = 24.0f, .color = srgb(0xFFFFFF, 0.6f + 0.4f * std::sin(time * 3.0f)), .weight = 1.3f, .align = Align::Center, .outline = true});
    }
}

void WayfarerGame::drawSheet(gamelib::draw::DrawList& list, const float time) const
{
    // Construction sheets for reviewing the characters: big with the
    // skeleton overlay, then at game size.
    const float W = static_cast<float>(m_canvas.width);
    const float H = static_cast<float>(m_canvas.height);
    list.setLayer(gamelib::draw::layers::Background);
    list.box({0.0f, 0.0f, W, H}, srgb(0xCDEBF7));
    list.setLayer(gamelib::draw::layers::Entities);
    const auto label = [&](const std::string_view text, const float x, const float y) {
        ui::text(list, text, x, y, {.size = 24.0f, .color = ui_color::kInk, .weight = 1.3f, .align = Align::Center});
    };
    if (m_sheet == 0 || m_sheet == 1) {
        // Sheet 0: the hero in each tier's gear; sheet 1: the townsfolk.
        std::vector<std::pair<std::string, anime::Avatar>> cast;
        if (m_sheet == 0) {
            for (int tier = 0; tier <= 5; ++tier) {
                game::Character c;
                if (tier > 0) {
                    c.gear.setAt(static_cast<int>(game::Slot::Weapon), {.item = game::equipItem(tier, game::Slot::Weapon, tier % 2), .count = 1});
                    c.gear.setAt(static_cast<int>(game::Slot::Helmet), {.item = game::equipItem(tier, game::Slot::Helmet, 0), .count = 1});
                    c.gear.setAt(static_cast<int>(game::Slot::Armor), {.item = game::equipItem(tier, game::Slot::Armor, 0), .count = 1});
                    c.gear.setAt(static_cast<int>(game::Slot::Shoes), {.item = game::equipItem(tier, game::Slot::Shoes, 0), .count = 1});
                } else {
                    c.gear.setAt(static_cast<int>(game::Slot::Weapon), {.item = game::kStarterSword, .count = 1});
                    c.gear.setAt(static_cast<int>(game::Slot::Armor), {.item = game::kStarterTunic, .count = 1});
                    c.gear.setAt(static_cast<int>(game::Slot::Shoes), {.item = game::kStarterShoes, .count = 1});
                }
                cast.emplace_back(tier == 0 ? "starter" : std::format("tier {}", tier), heroAvatar(c));
            }
        } else {
            for (int n = 0; n < game::kNpcCount; ++n) {
                cast.emplace_back(std::string{game::npcName(static_cast<game::NpcId>(n))}, npcAvatar(static_cast<game::NpcId>(n)));
            }
        }
        const float step = W / static_cast<float>(cast.size());
        for (std::size_t i = 0; i < cast.size(); ++i) {
            anime::Avatar a = cast[i].second;
            const float x = step * (static_cast<float>(i) + 0.5f);
            a.feet = {x, 470.0f};
            a.height = 400.0f;
            a.skeleton = true;
            list.add(anime::avatarSprite(a, m_shapes.avatar));
            label(cast[i].first, x, 490.0f);
            // Game size, in a few poses.
            const std::array poses{anime::AvatarPose::Idle, anime::AvatarPose::Walk, anime::AvatarPose::Attack, anime::AvatarPose::Jump};
            for (std::size_t p = 0; p < poses.size(); ++p) {
                anime::Avatar small = cast[i].second;
                small.feet = {x - 120.0f + static_cast<float>(p) * 80.0f, 760.0f + static_cast<float>(p % 2) * 200.0f};
                small.height = 158.0f;
                small.pose = poses[p];
                small.phase = std::fmod(time * 1.2f, 1.0f);
                list.add(anime::avatarSprite(small, m_shapes.avatar));
            }
        }
    } else if (m_sheet == 2) {
        // Every species, normal and elite, at game size.
        for (int i = 0; i < game::kSpeciesCount; ++i) {
            const auto id = static_cast<game::SpeciesId>(i);
            const game::Species& sp = game::species(id);
            const float x = 110.0f + static_cast<float>(i % 10) * 175.0f;
            const float y = 330.0f + static_cast<float>(i / 10) * 420.0f;
            for (int e = 0; e < 2; ++e) {
                anime::Monster m;
                m.feet = {x, y + static_cast<float>(e) * 190.0f};
                m.height = monsterScale(sp.look, sp.size.y * (e == 1 ? 1.3f : 1.0f), e == 1).quad;
                m.kind = static_cast<anime::MonsterKind>(monsterShaderKind(sp.look));
                const MonsterColors colors = monsterColors(id, e == 1);
                m.body = colors.body;
                m.accent = colors.accent;
                m.elite = e == 1;
                m.trait = static_cast<std::uint8_t>(eliteTrait(id));
                m.state = anime::MonsterState::Idle;
                m.facing = -1.0f;
                list.add(anime::monsterSprite(m, m_shapes.monster));
            }
            label(sp.name, x, y + 220.0f);
        }
    } else {
        // The guardians.
        for (int b = 0; b < game::kBossCount; ++b) {
            anime::Boss boss;
            boss.feet = {200.0f + static_cast<float>(b) * 380.0f, 800.0f};
            boss.height = 380.0f;
            boss.phase = b % 3;
            boss.kind = b;
            const MonsterColors bc = bossColors(static_cast<game::BossId>(b));
            boss.body = bc.body;
            boss.cap = bc.accent;
            boss.state = anime::MonsterState::Idle;
            list.add(anime::bossSprite(boss, m_shapes.boss));
            label(game::bossInfo(static_cast<game::BossId>(b)).name, boss.feet.x, 830.0f);
        }
    }
    ui::text(list, std::format("construction sheet {} / 4   (left / right)", m_sheet + 1), 30.0f, 20.0f,
             {.size = 26.0f, .color = ui_color::kInk, .weight = 1.4f});
}

void WayfarerGame::drawHints(gamelib::draw::DrawList& list, const float time) const
{
    (void)list;
    (void)time;
}

void WayfarerGame::draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info)
{
    (void)app;
    const float time = info.time;
    if (m_mode == Mode::Sheet) {
        drawSheet(list, time);
        return;
    }
    if (m_mode == Mode::Ending) {
        drawEnding(list, time);
    } else if (m_mode == Mode::Play && m_session && !m_demoMode) {
        const Vec2 shake = m_profile.value().settings.screenShake ? m_effects.shake(time) : Vec2{};
        m_view.draw(list, m_shapes, *m_session, m_effects, m_alpha, time, shake);
        m_effects.drawScreen(list, m_canvas);
        if (!m_dialogue.isOpen()) {
            m_hud.draw(list, m_shapes, *m_session, prefs().actions, m_view.view(), m_canvas, prefs().bigMinimap, time);
        }
        m_windows.draw(list, m_shapes, *m_session, prefs().actions, m_canvas, time);
        m_dialogue.draw(list, m_shapes, m_canvas, time);
        drawFainted(list, time);
        if (m_menus.is(ArcadeScreen::Paused)) {
            drawMenu(list, time);
            drawControls(list, time);
        }
    } else {
        const game::Session& demo = m_attract.sim().session;
        m_demoView.draw(list, m_shapes, demo, m_demoEffects, info.alpha, time, {});
        if (!m_demoMode) {
            if (m_mode == Mode::Slots) {
                drawSlots(list, time);
            } else {
                drawTitle(list, time);
            }
        } else {
            list.setLayer(kScreenLayer);
            ui::text(list, "demo - press any key", static_cast<float>(m_canvas.width) * 0.5f, 40.0f,
                     {.size = 30.0f, .color = srgb(0xFFFFFF, 0.6f + 0.4f * std::sin(time * 3.0f)), .weight = 1.4f, .align = Align::Center, .outline = true});
        }
    }
    list.setLayer(kScreenLayer + 100);
    list.setView({});
    m_transition.draw(list, m_canvas);
}

} // namespace wayfarer::app
