#include "hud.hpp"

#include "actions.hpp"
#include "look.hpp"
#include "ui_kit.hpp"

#include <gamelib/core/easing.hpp>

#include <algorithm>
#include <cmath>
#include <format>

namespace wayfarer::app {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;
using ui::Align;

constexpr int kHudLayer = gamelib::draw::layers::Hud + 10;

std::string clockText(const gamelib::core::GameClock& clock)
{
    const int h = clock.hour();
    const int m = clock.minute() / 10 * 10;
    const int h12 = h % 12 == 0 ? 12 : h % 12;
    return std::format("{}:{:02} {}", h12, m, h < 12 ? "am" : "pm");
}

} // namespace

void Hud::reset(const game::Session& s)
{
    m_hp.reset(s.character().hp.fraction());
    m_mp.reset(s.character().mp.fraction());
    m_xpShown = s.character().xpProgress();
    m_coinsShown = static_cast<float>(s.character().coins);
    m_toasts.clear();
    m_title.clear();
    if (s.boss()) {
        m_boss.reset(1.0f);
    }
}

void Hud::toast(std::string text, const Color color)
{
    m_toasts.add({.text = std::move(text), .color = color}, 3.2f);
}

void Hud::mapTitle(const game::Session& s)
{
    m_title.clear();
    const game::MapDef& def = s.map();
    m_title.add({.name = std::string{def.name}, .region = def.region == game::Region::Town ? "home" : std::string{game::regionName(def.region)}},
                3.6f);
}

void Hud::react(const game::Event& e, const game::Session& s)
{
    using game::EventKind;
    switch (e.kind) {
    case EventKind::QuestUpdate: {
        const game::QuestText& q = game::questText(e.value);
        const auto* def = s.quests().find(e.value);
        int required = 0;
        if (def != nullptr && !def->objectives.empty()) {
            required = def->objectives.front().required;
        }
        if (!q.title.empty() && required > 1) {
            toast(std::format("{}  {}/{}", q.title, std::min(e.extra, required), required), ui_color::kWhite);
        }
        break;
    }
    case EventKind::QuestComplete: toast(std::format("{} - complete!", game::questText(e.value).title), srgb(0xA8F0A0)); break;
    case EventKind::QuestAccepted: toast(std::format("new quest: {}", game::questText(e.value).title), srgb(0xFFE08A)); break;
    case EventKind::QuestDone: toast(std::format("quest done: {}", game::questText(e.value).title), srgb(0xFFE08A)); break;
    case EventKind::Full: toast("your bag is full", srgb(0xFFB0B0)); break;
    case EventKind::PortalClosed: toast("the way is sealed for now", srgb(0xFFD0D0)); break;
    case EventKind::NoMana:
        if (e.value >= 0) {
            toast("not enough mp", srgb(0xB8D0FF));
        } else {
            toast("no potions of that kind", srgb(0xFFD0D0));
        }
        break;
    case EventKind::SkillLearned:
        toast(std::format("{} learned", game::skillName(static_cast<game::SkillId>(e.value))), srgb(0xC8E8FF));
        break;
    case EventKind::Travel: mapTitle(s); break;
    case EventKind::BossWake: m_boss.reset(1.0f); break;
    default: break;
    }
}

void Hud::update(const float seconds, const game::Session& s)
{
    m_hp.set(s.character().hp.fraction());
    m_mp.set(s.character().mp.fraction());
    m_hp.update(seconds);
    m_mp.update(seconds);
    if (s.boss()) {
        m_boss.set(static_cast<float>(s.boss()->hp) / static_cast<float>(std::max(s.boss()->maxHp, 1)));
        m_boss.update(seconds);
    }
    m_toasts.update(seconds);
    m_title.update(seconds);
    // The XP bar fills smoothly (wrapping round on a level-up).
    const float xp = s.character().xpProgress();
    if (xp < m_xpShown - 0.001f) {
        m_xpShown = std::max(0.0f, m_xpShown - 1.0f);
    }
    m_xpShown += (xp - m_xpShown) * std::min(1.0f, seconds * 6.0f);
    const auto coins = static_cast<float>(s.character().coins);
    m_coinsShown += (coins - m_coinsShown) * std::min(1.0f, seconds * 8.0f);
    if (std::abs(coins - m_coinsShown) < 0.5f) {
        m_coinsShown = coins;
    }
}

void Hud::drawBars(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s, const gamelib::core::Size canvas) const
{
    const float W = static_cast<float>(canvas.width);
    const float H = static_cast<float>(canvas.height);
    const game::Character& c = s.character();
    // The experience bar along the bottom edge.
    const Rect xpBar{14.0f, H - 30.0f, W - 28.0f, 20.0f};
    ui::gauge(list, shapes, xpBar, m_xpShown, 0.0f, ui_color::kXp, srgb(0xFFF2C0));
    ui::text(list, std::format("exp {:.1f}%", c.xpProgress() * 100.0f), W * 0.5f, H - 33.0f,
             {.size = 19.0f, .color = srgb(0xFFFFFF), .weight = 1.5f, .align = Align::Center, .outline = true});
    // The status panel.
    const Rect panel{18.0f, H - 160.0f, 520.0f, 116.0f};
    addUi(list, shapes, Ui::Window, panel.expanded(8.0f), srgb(0x3E3354, 0.86f), srgb(0x8CCAD6), 0.0f, 1.0f);
    // The level badge.
    const Vec2 badge{panel.x + 62.0f, panel.center().y};
    addUi(list, shapes, Ui::Frame, {badge.x - 50.0f, badge.y - 50.0f, 100.0f, 100.0f}, srgb(0xF6C845), srgb(0xFFF2C0));
    ui::text(list, "lv", badge.x, badge.y - 40.0f, {.size = 20.0f, .color = srgb(0x6A4A1A), .weight = 1.6f, .align = Align::Center});
    ui::text(list, std::to_string(c.level), badge.x, badge.y - 18.0f,
             {.size = 44.0f, .color = srgb(0xFFFFFF), .weight = 2.2f, .align = Align::Center, .outline = true});
    // HP and MP.
    const float gx = panel.x + 128.0f;
    const float gw = panel.width - 146.0f;
    ui::text(list, "hp", gx, panel.y + 12.0f, {.size = 22.0f, .color = srgb(0xFFB8C0), .weight = 1.6f});
    ui::gauge(list, shapes, {gx + 40.0f, panel.y + 14.0f, gw - 40.0f, 30.0f}, c.hp.fraction(), m_hp.value(), ui_color::kHp, srgb(0xFFE6A8));
    ui::text(list, std::format("{} / {}", c.hp.current, c.hp.max), gx + 40.0f + (gw - 40.0f) * 0.5f, panel.y + 14.0f,
             {.size = 21.0f, .color = srgb(0xFFFFFF), .weight = 1.5f, .align = Align::Center, .outline = true});
    ui::text(list, "mp", gx, panel.y + 58.0f, {.size = 22.0f, .color = srgb(0xB8D0FF), .weight = 1.6f});
    ui::gauge(list, shapes, {gx + 40.0f, panel.y + 60.0f, gw - 40.0f, 30.0f}, c.mp.fraction(), m_mp.value(), ui_color::kMp, srgb(0xD8E8FF));
    ui::text(list, std::format("{} / {}", c.mp.current, c.mp.max), gx + 40.0f + (gw - 40.0f) * 0.5f, panel.y + 60.0f,
             {.size = 21.0f, .color = srgb(0xFFFFFF), .weight = 1.5f, .align = Align::Center, .outline = true});
    // Unspent points: a nudge towards the stat and skill windows.
    if (c.abilityPoints > 0 || c.skills.points() > 0) {
        std::string nudge;
        if (c.abilityPoints > 0) {
            nudge += std::format("{} ap", c.abilityPoints);
        }
        if (c.skills.points() > 0) {
            nudge += (nudge.empty() ? "" : "  ") + std::format("{} sp", c.skills.points());
        }
        ui::text(list, nudge, panel.x + 14.0f, panel.y - 40.0f, {.size = 24.0f, .color = srgb(0xFFE08A), .weight = 1.6f, .outline = true});
    }
}

void Hud::drawHotkeys(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s,
                      const gamelib::platform::ActionMap& actions, const gamelib::core::Size canvas, const float time) const
{
    (void)time;
    const float W = static_cast<float>(canvas.width);
    const float H = static_cast<float>(canvas.height);
    const game::Character& c = s.character();
    const float size = 84.0f;
    const float gap = 12.0f;
    const int count = game::kHotkeyCount + 2;
    float x = W - 24.0f - static_cast<float>(count) * (size + gap);
    const float y = H - 150.0f;
    addUi(list, shapes, Ui::Window, {x - 20.0f, y - 18.0f, static_cast<float>(count) * (size + gap) + 30.0f, size + 52.0f},
          srgb(0x3E3354, 0.86f), srgb(0x8CCAD6), 0.0f, 1.0f);
    for (int i = 0; i < count; ++i) {
        const Rect r{x, y, size, size};
        addUi(list, shapes, Ui::Slot, r, srgb(0x5A4E70), srgb(0xFFFFFF), 0.0f, 0.0f);
        if (i < game::kHotkeyCount) {
            const game::SkillId skill = game::hotkeySkill(i);
            const int id = static_cast<int>(skill);
            const bool learned = c.skills.levelOf(id) > 0;
            const Color tintA = skill == game::SkillId::Sweep      ? srgb(0x9FE6FF)
                                : skill == game::SkillId::Starbolt ? srgb(0xFFD86A)
                                : skill == game::SkillId::Tailwind ? srgb(0x7FE0B0)
                                                                   : srgb(0xFF9EC8);
            const Color tintB = skill == game::SkillId::Tailwind ? srgb(0xC8FFE8) : srgb(0xFFF2B0);
            addIcon(list, shapes, Ui::SkillIcon, r.center(), size * 0.8f, static_cast<float>(id), 0.0f, tintA, tintB, learned ? 0.0f : 1.0f);
            const float cd = 1.0f - c.skills.cooldownProgress(id);
            if (learned && cd > 0.0f && c.skills.cooldownRemaining(id) > 0) {
                gamelib::ui::drawCooldown(list, r.expanded(-4.0f), cd, {0.08f, 0.05f, 0.15f, 0.6f}, 10.0f);
                ui::text(list, std::format("{:.0f}", std::ceil(static_cast<float>(c.skills.cooldownRemaining(id)) / game::kTickRate)),
                         r.center().x, r.center().y - 16.0f,
                         {.size = 28.0f, .color = srgb(0xFFFFFF), .weight = 1.8f, .align = Align::Center, .outline = true});
            } else if (learned) {
                const int cost = game::skillDef(skill).levels[static_cast<std::size_t>(c.skills.levelOf(id) - 1)].cost;
                if (cost > c.mp.current) {
                    addUi(list, shapes, Ui::Tooltip, r.expanded(-4.0f), srgb(0x2A3A70, 0.55f), srgb(0x5E9CF2));
                }
            }
            ui::keyCap(list, shapes, {r.center().x, r.bottom() + 4.0f}, actions.keyLabel(actionId(static_cast<Action>(static_cast<int>(Action::Skill1) + i))), 30.0f);
        } else {
            const int which = i - game::kHotkeyCount;
            const int item = c.quick[static_cast<std::size_t>(which)];
            const IconLook icon = itemIcon(item);
            const int countHeld = c.bag.countOf(item);
            addIcon(list, shapes, Ui::ItemIcon, r.center(), size * 0.74f, icon.id, icon.style,
                    countHeld > 0 ? icon.a : icon.a.withAlpha(0.35f), icon.b);
            ui::text(list, std::to_string(countHeld), r.right() - 6.0f, r.bottom() - 30.0f,
                     {.size = 24.0f, .color = srgb(0xFFFFFF), .weight = 1.6f, .align = Align::Right, .outline = true});
            ui::keyCap(list, shapes, {r.center().x, r.bottom() + 4.0f},
                       actions.keyLabel(actionId(which == 0 ? Action::PotionHp : Action::PotionMp)), 30.0f);
        }
        x += size + gap;
    }
}

void Hud::drawMinimap(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s, const bool big, const float time) const
{
    const game::MapDef& def = s.map();
    if (m_cached != def.id) {
        m_cached = def.id;
        m_lines.clear();
        for (int y = 0; y < def.rows; ++y) {
            int x = 0;
            while (x < def.columns) {
                const char c = def.text[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
                const bool surface = (c == game::kGround || c == game::kRampUp || c == game::kRampDown)
                                     && (y == 0 || def.text[static_cast<std::size_t>(y - 1)][static_cast<std::size_t>(x)] == game::kEmpty);
                const bool foot = c == game::kFoothold;
                if (!surface && !foot) {
                    ++x;
                    continue;
                }
                const int start = x;
                while (x < def.columns) {
                    const char d = def.text[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
                    const bool s2 = (d == game::kGround || d == game::kRampUp || d == game::kRampDown)
                                    && (y == 0 || def.text[static_cast<std::size_t>(y - 1)][static_cast<std::size_t>(x)] == game::kEmpty);
                    if ((foot && d != game::kFoothold) || (!foot && !s2)) {
                        break;
                    }
                    ++x;
                }
                m_lines.push_back({.rect = {static_cast<float>(start), static_cast<float>(y), static_cast<float>(x - start), 1.0f},
                                   .kind = foot ? 1 : 0});
            }
        }
        for (int x = 0; x < def.columns; ++x) {
            int y = 0;
            while (y < def.rows) {
                const char c = def.text[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
                if (c != game::kRope && c != game::kLadder) {
                    ++y;
                    continue;
                }
                const int start = y;
                while (y < def.rows && def.text[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] == c) {
                    ++y;
                }
                m_lines.push_back({.rect = {static_cast<float>(x), static_cast<float>(start), 1.0f, static_cast<float>(y - start)}, .kind = 2});
            }
        }
    }
    const float maxW = big ? 560.0f : 330.0f;
    const float maxH = big ? 300.0f : 170.0f;
    const float scale = std::min(maxW / static_cast<float>(def.columns), maxH / static_cast<float>(def.rows));
    const float w = static_cast<float>(def.columns) * scale;
    const float h = static_cast<float>(def.rows) * scale;
    const Rect frame{22.0f, 64.0f, w + 24.0f, h + 24.0f};
    addUi(list, shapes, Ui::Window, frame.expanded(8.0f), srgb(0x2E2840, 0.75f), srgb(0x8CCAD6), 0.0f, 1.0f);
    ui::text(list, def.name, frame.x + 4.0f, frame.y - 46.0f, {.size = 28.0f, .color = srgb(0xFFFFFF), .weight = 1.6f, .outline = true});
    const Vec2 o{frame.x + 12.0f, frame.y + 12.0f};
    for (const Line& l : m_lines) {
        const Rect r{o.x + l.rect.x * scale, o.y + l.rect.y * scale, std::max(l.rect.width * scale, 2.0f), std::max(l.rect.height * scale, 2.0f)};
        const Color c = l.kind == 0 ? srgb(0xA8E890) : l.kind == 1 ? srgb(0xE8D0A0) : srgb(0xC8A878, 0.8f);
        list.box(l.kind == 2 ? Rect{r.x + r.width * 0.5f - 1.0f, r.y, 2.0f, r.height} : Rect{r.x, r.y, r.width, 3.0f}, c, {.cornerRadius = 1.0f});
    }
    const float t = game::kTile;
    for (int i = 0; i < static_cast<int>(def.portals.size()); ++i) {
        const game::Portal& p = def.portals[static_cast<std::size_t>(i)];
        if (p.afterBoss && !s.portalOpen(i)) {
            continue;
        }
        const Vec2 at = o + Vec2{p.at.x / t * scale, (p.at.y / t - 1.0f) * scale};
        list.circle(at, 5.0f, s.portalOpen(i) ? srgb(0x8FD8FF) : srgb(0x8A8098), {.glowWidth = 4.0f, .glowColor = srgb(0x8FD8FF, 0.5f)});
    }
    for (const game::NpcHere& n : s.npcs()) {
        const Vec2 at = o + Vec2{n.feet.x / t * scale, (n.feet.y / t - 1.0f) * scale};
        list.circle(at, 4.5f, srgb(0x6CE070));
    }
    if (s.boss()) {
        const Vec2 at = o + Vec2{s.boss()->box.center().x / t * scale, s.boss()->box.center().y / t * scale};
        list.circle(at, 7.0f, srgb(0xFF5A6A), {.glowWidth = 6.0f, .glowColor = srgb(0xFF5A6A, 0.5f)});
    }
    const Vec2 hero = s.hero().body.box.bottomCenter();
    const Vec2 at = o + Vec2{hero.x / t * scale, (hero.y / t - 1.0f) * scale};
    const float pulse = 1.0f + 0.25f * std::sin(time * 6.0f);
    list.circle(at, 6.0f * pulse, srgb(0xFFE24A), {.glowWidth = 6.0f, .glowColor = srgb(0xFFE24A, 0.5f)});
}

void Hud::drawClock(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s, const gamelib::core::Size canvas) const
{
    const float W = static_cast<float>(canvas.width);
    const Rect panel{W - 300.0f, 22.0f, 276.0f, 112.0f};
    addUi(list, shapes, Ui::Window, panel.expanded(8.0f), srgb(0x2E2840, 0.75f), srgb(0x8CCAD6), 0.0f, 1.0f);
    const float night = s.nightAmount();
    const Vec2 icon{panel.x + 46.0f, panel.y + 32.0f};
    if (night > 0.5f) {
        list.circle(icon, 18.0f, srgb(0xFFF4D0), {.glowWidth = 10.0f, .glowColor = srgb(0xB8C8FF, 0.5f)});
        list.circle(icon + Vec2{8.0f, -6.0f}, 15.0f, srgb(0x2E2840));
    } else {
        list.circle(icon, 18.0f, srgb(0xFFD24A), {.glowWidth = 12.0f, .glowColor = srgb(0xFFB040, 0.5f)});
    }
    const auto date = s.clock().date();
    ui::text(list, clockText(s.clock()), panel.x + 84.0f, panel.y + 12.0f, {.size = 30.0f, .color = srgb(0xFFFFFF), .weight = 1.6f});
    ui::text(list, std::format("day {}", s.clock().day() + 1), panel.x + 84.0f, panel.y + 48.0f,
             {.size = 20.0f, .color = srgb(0xD8D0E8), .weight = 1.3f});
    (void)date;
    addIcon(list, shapes, Ui::Coin, {panel.x + 46.0f, panel.y + 86.0f}, 40.0f, 0.0f, 0.0f, srgb(0xF6C845), srgb(0xFFFFFF));
    ui::text(list, ui::groupDigits(static_cast<long long>(std::lround(m_coinsShown))), panel.x + 84.0f, panel.y + 70.0f,
             {.size = 28.0f, .color = srgb(0xFFE08A), .weight = 1.6f});
    // Active buffs under the clock.
    float bx = panel.right() - 40.0f;
    for (const auto& b : s.character().buffs.all()) {
        if (b.id == game::kTailwindBuff) {
            addIcon(list, shapes, Ui::SkillIcon, {bx, panel.bottom() + 46.0f}, 56.0f, static_cast<float>(game::SkillId::Tailwind), 0.0f,
                    srgb(0x7FE0B0), srgb(0xC8FFE8));
            ui::text(list, std::format("{}s", b.remaining / game::kTickRate), bx, panel.bottom() + 74.0f,
                     {.size = 20.0f, .color = srgb(0xFFFFFF), .weight = 1.4f, .align = Align::Center, .outline = true});
            bx -= 66.0f;
        }
    }
}

void Hud::drawTracker(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s, const gamelib::core::Size canvas) const
{
    (void)shapes;
    const float W = static_cast<float>(canvas.width);
    float y = 260.0f;
    int shown = 0;
    for (const int id : s.quests().log()) {
        if (shown == 3) {
            break;
        }
        const auto* def = s.quests().find(id);
        const game::QuestText& q = game::questText(id);
        if (def == nullptr) {
            continue;
        }
        const bool complete = s.quests().stateOf(id) == gamelib::core::QuestState::Complete;
        ui::text(list, q.title, W - 30.0f, y,
                 {.size = 25.0f, .color = complete ? srgb(0xA8F0A0) : srgb(0xFFE08A), .weight = 1.6f, .align = Align::Right, .outline = true});
        y += 32.0f;
        std::string line;
        if (complete) {
            line = "return to " + std::string{game::npcName(static_cast<game::NpcId>(def->turnInTo != 0 || id == game::kQuestArrival ? def->turnInTo : def->giver))};
        } else if (!def->objectives.empty()) {
            const auto& o = def->objectives.front();
            line = std::format("{}  {}/{}", q.summary, s.quests().progressOf(id, 0), o.required);
            if (o.required == 1) {
                line = std::string{q.summary};
            }
        }
        ui::text(list, line, W - 30.0f, y, {.size = 20.0f, .color = srgb(0xFFFFFF), .weight = 1.3f, .align = Align::Right, .outline = true});
        y += 40.0f;
        ++shown;
    }
}

void Hud::drawBoss(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s, const gamelib::core::Size canvas,
                   const float time) const
{
    (void)time;
    if (!s.boss()) {
        return;
    }
    const game::Boss& b = *s.boss();
    const float W = static_cast<float>(canvas.width);
    const float appear = b.state == game::BossState::Intro ? b.progress() : 1.0f;
    const Rect bar{W * 0.5f - 420.0f, 46.0f - (1.0f - appear) * 120.0f, 840.0f, 34.0f};
    addUi(list, shapes, Ui::Window, Rect{bar.x - 30.0f, bar.y - 52.0f, bar.width + 60.0f, bar.height + 76.0f}.expanded(4.0f),
          srgb(0x3E2840, 0.86f), srgb(0xF07080), 0.0f, 1.0f);
    ui::text(list, b.info().name, bar.x, bar.y - 44.0f, {.size = 32.0f, .color = srgb(0xFFD8E0), .weight = 1.8f, .outline = true});
    ui::text(list, b.info().title, bar.right(), bar.y - 38.0f, {.size = 22.0f, .color = srgb(0xE8C8D8), .align = Align::Right});
    ui::gauge(list, shapes, bar, static_cast<float>(b.hp) / static_cast<float>(std::max(b.maxHp, 1)) * appear, m_boss.value() * appear,
              srgb(0xF0506A), srgb(0xFFE08A));
}

void Hud::drawPrompt(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s, const gamelib::platform::ActionMap& actions,
                     const gamelib::core::View2D& view, const float time) const
{
    if (!s.hero().body.grounded || s.phase() != game::Phase::Playing) {
        return;
    }
    std::string label;
    Vec2 at;
    if (const auto npc = s.npcInReach()) {
        label = "talk";
        for (const auto& n : s.npcs()) {
            if (n.npc == *npc) {
                at = n.feet + Vec2{0.0f, -200.0f};
            }
        }
    } else if (s.innInReach()) {
        label = "rest";
        at = s.hero().body.box.bottomCenter() + Vec2{0.0f, -190.0f};
    } else if (const int p = s.portalInReach(); p >= 0) {
        label = s.portalOpen(p) ? "enter" : "sealed";
        at = s.map().portals[static_cast<std::size_t>(p)].at + Vec2{0.0f, -290.0f};
    } else {
        return;
    }
    const Vec2 c = view.toCanvas(at) + Vec2{0.0f, std::sin(time * 4.0f) * 4.0f};
    const float w = ui::textWidth(label, 26.0f) + 80.0f;
    addUi(list, shapes, Ui::Tooltip, {c.x - w * 0.5f, c.y - 24.0f, w, 50.0f}, srgb(0x2E2840, 0.85f), srgb(0xFFE08A));
    ui::keyCap(list, shapes, {c.x - w * 0.5f + 28.0f, c.y + 1.0f}, actions.keyLabel(actionId(Action::Up)), 32.0f);
    ui::text(list, label, c.x + 18.0f, c.y - 15.0f, {.size = 26.0f, .color = srgb(0xFFF2D8), .weight = 1.5f, .align = Align::Center});
}

void Hud::draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& s, const gamelib::platform::ActionMap& actions,
               const gamelib::core::View2D& view, const gamelib::core::Size canvas, const bool bigMinimap, const float time) const
{
    list.setLayer(kHudLayer);
    drawPrompt(list, shapes, s, actions, view, time);
    drawBars(list, shapes, s, canvas);
    drawHotkeys(list, shapes, s, actions, canvas, time);
    drawMinimap(list, shapes, s, bigMinimap, time);
    drawClock(list, shapes, s, canvas);
    drawTracker(list, shapes, s, canvas);
    drawBoss(list, shapes, s, canvas, time);
    const float W = static_cast<float>(canvas.width);
    // The map title, eased in and out.
    for (const auto& item : m_title) {
        const float t = item.progress();
        const float in = gamelib::core::easeOutBack(std::min(t * 5.0f, 1.0f));
        const float alpha = 1.0f - gamelib::core::smoothstep(0.75f, 1.0f, t);
        const float y = 180.0f - (1.0f - in) * 30.0f;
        ui::text(list, item.value.region, W * 0.5f, y - 44.0f,
                 {.size = 30.0f, .color = srgb(0xFFF2D8, alpha), .weight = 1.5f, .align = Align::Center, .outline = true});
        ui::text(list, item.value.name, W * 0.5f, y,
                 {.size = 66.0f * (0.85f + 0.15f * in), .color = srgb(0xFFFFFF, alpha), .weight = 2.3f, .align = Align::Center, .outline = true});
    }
    // Toasts, newest at the bottom.
    float y = static_cast<float>(canvas.height) - 230.0f;
    int index = 0;
    for (auto it = m_toasts.items().rbegin(); it != m_toasts.items().rend() && index < 4; ++it, ++index) {
        const float t = it->progress();
        const float alpha = (1.0f - gamelib::core::smoothstep(0.8f, 1.0f, t)) * std::min(t * 12.0f, 1.0f);
        const float slide = (1.0f - gamelib::core::easeOutCubic(std::min(t * 8.0f, 1.0f))) * 40.0f;
        const float w = ui::textWidth(it->value.text, 26.0f) + 50.0f;
        addUi(list, shapes, Ui::Tooltip, {W - 40.0f - w + slide, y - 6.0f, w, 46.0f}, srgb(0x2E2840, 0.8f * alpha), it->value.color.withAlpha(alpha));
        ui::text(list, it->value.text, W - 40.0f - w * 0.5f + slide, y + 2.0f,
                 {.size = 26.0f, .color = it->value.color.withAlpha(alpha), .weight = 1.4f, .align = Align::Center});
        y -= 56.0f;
    }
}

} // namespace wayfarer::app
