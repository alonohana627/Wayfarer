#include "effects.hpp"

#include "look.hpp"

#include <gamelib/core/easing.hpp>
#include <gamelib/draw/vector_text.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>

namespace wayfarer::app {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;
using game::EventKind;

constexpr float kPi = std::numbers::pi_v<float>;

// Damage number kinds, carried in the number's color.
enum class NumberKind { Hit, Crit, Hurt, Heal, Mana };

Color numberTag(const NumberKind k)
{
    return {static_cast<float>(k) / 10.0f, 0.0f, 0.0f, 1.0f};
}

NumberKind numberKind(const Color c)
{
    return static_cast<NumberKind>(std::lround(c.r * 10.0f));
}

void numberColors(const NumberKind k, Color& top, Color& bottom)
{
    switch (k) {
    case NumberKind::Hit:
        top = srgb(0xFFF1B8);
        bottom = srgb(0xFF9A3C);
        break;
    case NumberKind::Crit:
        top = srgb(0xFFF6C8);
        bottom = srgb(0xFF4E7A);
        break;
    case NumberKind::Hurt:
        top = srgb(0xE6CCFF);
        bottom = srgb(0x8A5ADC);
        break;
    case NumberKind::Heal:
        top = srgb(0xD8FFD0);
        bottom = srgb(0x3CB85A);
        break;
    case NumberKind::Mana:
        top = srgb(0xD8ECFF);
        bottom = srgb(0x3C7CDC);
        break;
    }
}

std::string itemName(const int item)
{
    const auto* def = game::itemRegistry().find(item);
    return def != nullptr ? def->name : "";
}

} // namespace

Effects::Effects()
    : m_numbers{{.pixelSize = 5.0f, .criticalScale = 1.45f, .pop = 0.6f, .popSeconds = 0.14f, .life = 1.05f, .rise = 70.0f,
                 .fadeFrom = 0.7f, .stackSpacing = 62.0f, .stackSeconds = 0.6f}}
    , m_sparks{400, {0.0f, 900.0f}, 0.4f, 7}
    , m_petals{200, {0.0f, 60.0f}, 0.6f, 11}
{
}

void Effects::clear()
{
    m_numbers.clear();
    m_arcs.clear();
    m_spots.clear();
    m_popups.clear();
    m_ghosts.clear();
    m_sparks.clear();
    m_petals.clear();
    m_motes.clear();
    m_shake.clear();
    m_flash.clear();
}

void Effects::setRegion(const game::Region region, const bool dungeon)
{
    using game::Region;
    gamelib::fx::AmbientStyle style;
    gamelib::fx::AmbientLook look;
    switch (region) {
    case Region::Town:
        style = {.rate = 2.5f, .speedX = {8.0f, 30.0f}, .speedY = {12.0f, 30.0f}, .life = {5.0f, 9.0f}, .size = {3.0f, 5.0f}};
        look = {.color = srgb(0xFFC8DC, 0.85f), .glow = 0.8f, .glowAlpha = 0.25f};
        break;
    case Region::Forest:
        style = {.rate = dungeon ? 3.0f : 3.5f, .speedX = {-20.0f, 25.0f}, .speedY = {18.0f, 40.0f}, .life = {5.0f, 9.0f},
                 .size = {3.5f, 6.0f}};
        look = {.color = dungeon ? srgb(0xB8FF9A, 0.9f) : srgb(0x9ED86A, 0.9f), .glow = dungeon ? 2.5f : 0.6f, .glowAlpha = 0.4f};
        break;
    case Region::Mines:
        style = {.rate = 6.0f, .speedX = {-6.0f, 6.0f}, .speedY = {-14.0f, -4.0f}, .life = {3.0f, 6.0f}, .size = {1.5f, 3.0f}};
        look = {.color = srgb(0x9AE8FF, 0.9f), .glow = 3.0f, .glowAlpha = 0.5f};
        break;
    case Region::Marsh:
        style = {.rate = 5.0f, .speedX = {-10.0f, 10.0f}, .speedY = {-12.0f, 6.0f}, .life = {3.0f, 7.0f}, .size = {2.0f, 3.5f},
                 .band = {0.3f, 1.0f}};
        look = {.color = srgb(0xE8FF8A, 0.95f), .glow = 3.5f, .glowAlpha = 0.55f};
        break;
    case Region::Ruins:
        style = {.rate = 5.0f, .speedX = {6.0f, 22.0f}, .speedY = {-6.0f, 6.0f}, .life = {4.0f, 8.0f}, .size = {1.5f, 3.0f}};
        look = {.color = srgb(0xFFE2A0, 0.8f), .glow = 2.0f, .glowAlpha = 0.35f};
        break;
    case Region::Peak:
        style = {.rate = 22.0f, .speedX = {-40.0f, -10.0f}, .speedY = {40.0f, 90.0f}, .life = {5.0f, 9.0f}, .size = {2.5f, 5.0f}};
        look = {.color = srgb(0xFFFFFF, 0.9f), .glow = 0.8f, .glowAlpha = 0.3f};
        break;
    case Region::Count: break;
    }
    m_motes.setStyle(style);
    m_motes.clear();
    m_moteLook = look;
}

void Effects::react(const game::Event& e, const game::Session& s)
{
    const Vec2 feet = s.hero().body.box.bottomCenter();
    const int facing = s.hero().body.facing;
    switch (e.kind) {
    case EventKind::Swing: {
        const int step = e.value;
        const auto weapon = s.character().gear.at(static_cast<int>(game::Slot::Weapon));
        const bool staff = game::itemInfo(weapon.item).style == 2;
        const Color core = staff ? srgb(0xE8E0FF) : srgb(0xFFFFFF);
        const Color edge = staff ? srgb(0xA88CFF) : srgb(0x9FE6FF);
        addArc({.offset = {static_cast<float>(facing) * 34.0f, -54.0f}, .facing = facing, .variant = step,
                .core = core.scaled(1.3f), .edge = edge, .size = step == 2 ? 290.0f : 240.0f},
               step == 2 ? 0.32f : 0.24f);
        break;
    }
    case EventKind::Hit: {
        const NumberKind kind = e.crit ? NumberKind::Crit : NumberKind::Hit;
        m_numbers.add(static_cast<long long>(e.value), e.at + Vec2{0.0f, -20.0f}, numberTag(kind), e.crit,
                      static_cast<std::uint64_t>(std::lround(e.at.x / 50.0f)) + 1000);
        spot({.at = e.at + Vec2{0.0f, 30.0f}, .kind = Fx::Spark, .size = e.crit ? 200.0f : 150.0f,
              .core = srgb(0xFFFFFF).scaled(1.4f), .edge = e.crit ? srgb(0xFFB040) : srgb(0xFFE6A0), .seed = e.at.x * 0.13f},
             0.28f);
        m_sparks.burst(e.at + Vec2{0.0f, 30.0f}, e.crit ? 14 : 8,
                       {.speed = {220.0f, 520.0f}, .size = {4.0f, 8.0f}, .life = {0.2f, 0.45f}, .spin = {-8.0f, 8.0f},
                        .color = e.crit ? srgb(0xFFD060) : srgb(0xFFF2C0)});
        m_shake.trigger(std::min(0.25f + (e.crit ? 0.25f : 0.0f), 1.0f) * m_shakeScale * 0.5f);
        break;
    }
    case EventKind::Guarded:
        spot({.at = e.at, .kind = Fx::Spark, .size = 120.0f, .core = srgb(0xD8E8FF), .edge = srgb(0x88A8FF)}, 0.25f);
        m_numbers.add("guard", e.at + Vec2{0.0f, -40.0f}, numberTag(NumberKind::Mana));
        break;
    case EventKind::MonsterDown: {
        const auto colors = monsterColors(static_cast<game::SpeciesId>(e.value), e.extra != 0);
        m_sparks.burst(e.at, 16, {.speed = {120.0f, 420.0f}, .size = {6.0f, 12.0f}, .life = {0.3f, 0.7f}, .spin = {-6.0f, 6.0f},
                                  .color = colors.accent});
        spot({.at = e.at, .kind = Fx::Puff, .size = 180.0f, .core = srgb(0xFFFFFF, 0.8f), .edge = srgb(0xFFFFFF)}, 0.5f);
        break;
    }
    case EventKind::Hurt:
        m_numbers.add(static_cast<long long>(e.value), e.at + Vec2{0.0f, -30.0f}, numberTag(NumberKind::Hurt), false, 1);
        m_flash.trigger(0.35f);
        m_flashColor = srgb(0xFF5A6A, 0.35f);
        m_shake.trigger(0.7f * m_shakeScale);
        spot({.at = e.at + Vec2{0.0f, 40.0f}, .kind = Fx::Spark, .size = 160.0f, .core = srgb(0xFFFFFF), .edge = srgb(0xFF8A9A)},
             0.25f);
        break;
    case EventKind::Heal:
        m_numbers.add("+" + std::to_string(e.value), e.at + Vec2{0.0f, -60.0f}, numberTag(NumberKind::Heal), false, 2);
        for (int i = 0; i < 14; ++i) {
            m_petals.burst(feet + Vec2{static_cast<float>(i - 7) * 10.0f, -10.0f}, 1,
                           {.direction = {-kPi * 0.85f, -kPi * 0.15f}, .speed = {120.0f, 320.0f}, .size = {18.0f, 28.0f},
                            .life = {0.8f, 1.4f}, .spin = {-3.0f, 3.0f}, .color = i % 2 == 0 ? srgb(0xFF9EC8) : srgb(0xFFE08A)});
        }
        spot({.at = Vec2{0.0f, -50.0f}, .kind = Fx::Plus, .size = 120.0f, .core = srgb(0xC8FFD0).scaled(1.3f),
              .edge = srgb(0x5CDC7A), .attached = true},
             0.7f);
        break;
    case EventKind::Potion:
        if (e.onHero) {
            m_numbers.add("+" + std::to_string(e.value), e.at + Vec2{0.0f, -60.0f},
                          numberTag(e.extra == 0 ? NumberKind::Heal : NumberKind::Mana), false, 2);
            spot({.at = Vec2{0.0f, -50.0f}, .kind = Fx::Plus, .size = 100.0f,
                  .core = e.extra == 0 ? srgb(0xFFD0D8) : srgb(0xD0E4FF),
                  .edge = e.extra == 0 ? srgb(0xFF6A7A) : srgb(0x6A9AFF), .attached = true},
                 0.5f);
        }
        break;
    case EventKind::HyperJump:
        spot({.at = e.at, .kind = Fx::Shockwave, .size = 220.0f, .core = srgb(0xFFFFFF), .edge = srgb(0x9FE6FF).scaled(1.3f)},
             0.4f);
        m_sparks.burst(e.at + Vec2{0.0f, 30.0f}, 12,
                       {.direction = e.value != 0 ? gamelib::fx::Range{kPi * 0.3f, kPi * 0.7f}
                                                  : gamelib::fx::Range{facing > 0 ? kPi * 0.8f : -kPi * 0.2f,
                                                                       facing > 0 ? kPi * 1.2f : kPi * 0.2f},
                        .speed = {200.0f, 500.0f}, .size = {4.0f, 8.0f}, .life = {0.25f, 0.5f}, .color = srgb(0xBFF2FF)});
        m_ghostClock = 0.0f;
        break;
    case EventKind::Jump:
        spot({.at = e.at + Vec2{0.0f, -8.0f}, .kind = Fx::Puff, .size = 90.0f, .core = srgb(0xFFFFFF, 0.6f), .edge = srgb(0xFFFFFF)},
             0.35f);
        break;
    case EventKind::Land:
        if (e.value > 20) {
            for (const float dir : {-1.0f, 1.0f}) {
                spot({.at = e.at + Vec2{dir * 26.0f, -10.0f}, .kind = Fx::Puff, .size = 70.0f, .core = srgb(0xFFFFFF, 0.55f),
                      .edge = srgb(0xFFFFFF)},
                     0.35f);
            }
        }
        break;
    case EventKind::Roll:
        spot({.at = e.at + Vec2{0.0f, -10.0f}, .kind = Fx::Puff, .size = 110.0f, .core = srgb(0xFFFFFF, 0.7f), .edge = srgb(0xFFFFFF)},
             0.4f);
        break;
    case EventKind::SkillCast: {
        const auto skill = static_cast<game::SkillId>(e.value);
        if (skill == game::SkillId::Sweep) {
            addArc({.offset = {0.0f, -56.0f}, .facing = facing, .core = srgb(0xFFFFFF).scaled(1.5f), .edge = srgb(0x7FD8FF).scaled(1.2f),
                    .size = 420.0f, .kind = Fx::Sweep},
                   0.62f);
            m_shake.trigger(0.4f * m_shakeScale);
        } else if (skill == game::SkillId::Tailwind) {
            spot({.at = Vec2{0.0f, -60.0f}, .kind = Fx::Shockwave, .size = 300.0f, .core = srgb(0xFFFFFF),
                  .edge = srgb(0x9AF0C8).scaled(1.3f), .attached = true},
                 0.6f);
            m_petals.burst(feet + Vec2{0.0f, -40.0f}, 18,
                           {.speed = {200.0f, 420.0f}, .size = {10.0f, 18.0f}, .life = {0.5f, 0.9f}, .spin = {-6.0f, 6.0f},
                            .color = srgb(0xB8F8D8)});
        } else if (skill == game::SkillId::Starbolt) {
            spot({.at = e.at + Vec2{static_cast<float>(facing) * 40.0f, -14.0f}, .kind = Fx::Spark, .size = 160.0f,
                  .core = srgb(0xFFFFFF).scaled(1.4f), .edge = srgb(0xFFD86A)},
                 0.25f);
        } else if (skill == game::SkillId::Bloom) {
            spot({.at = Vec2{0.0f, -10.0f}, .kind = Fx::Shockwave, .size = 340.0f, .core = srgb(0xFFFFFF),
                  .edge = srgb(0xFF9EC8).scaled(1.3f), .attached = true},
                 0.8f);
        }
        break;
    }
    case EventKind::LevelUp:
        spot({.at = Vec2{0.0f, -110.0f}, .kind = Fx::LevelUp, .size = 300.0f, .core = srgb(0xFFF2B0).scaled(1.4f),
              .edge = srgb(0xFFD040), .attached = true},
             2.2f);
        m_flash.trigger(0.5f);
        m_flashColor = srgb(0xFFF4C8, 0.4f);
        m_popups.add({.at = feet + Vec2{0.0f, -190.0f}, .text = "level up!", .color = srgb(0xFFD040)}, 2.4f);
        break;
    case EventKind::Pickup: {
        const int count = 1;
        (void)count;
        m_popups.add({.at = feet + Vec2{0.0f, -150.0f - static_cast<float>(m_popupRow) * 42.0f}, .text = itemName(e.value),
                      .color = rarityColor(static_cast<game::Rarity>(std::clamp(e.extra, 0, 3))), .icon = e.value,
                      .rarity = e.extra},
                     1.6f);
        m_popupRow = (m_popupRow + 1) % 4;
        m_popupClock = 0.0f;
        break;
    }
    case EventKind::Coins:
        if (e.value > 0) {
            m_popups.add({.at = feet + Vec2{0.0f, -150.0f - static_cast<float>(m_popupRow) * 42.0f},
                          .text = std::format("+{} coins", e.value), .color = srgb(0xFFD040)},
                         1.3f);
            m_popupRow = (m_popupRow + 1) % 4;
            m_popupClock = 0.0f;
        }
        break;
    case EventKind::Spawned:
        if (e.extra >= 100) {
            // Loot hitting the floor sparkles in its rarity color.
            if (e.extra - 100 >= 1) {
                spot({.at = e.at, .kind = Fx::Spark, .size = 160.0f, .core = srgb(0xFFFFFF),
                      .edge = rarityColor(static_cast<game::Rarity>(std::clamp(e.extra - 100, 0, 3)))},
                     0.45f);
            }
        } else {
            spot({.at = e.at + Vec2{0.0f, -30.0f}, .kind = Fx::Puff, .size = 140.0f, .core = srgb(0xFFFFFF, 0.8f),
                  .edge = srgb(0xFFFFFF)},
                 0.5f);
        }
        break;
    case EventKind::BossLand:
        m_shake.trigger(1.0f * m_shakeScale);
        spot({.at = e.at, .kind = Fx::Shockwave, .size = 360.0f, .core = srgb(0xFFFFFF), .edge = srgb(0xFFE0B0)}, 0.5f);
        break;
    case EventKind::BossPhase:
        m_flash.trigger(0.6f);
        m_flashColor = srgb(0xFFFFFF, 0.5f);
        m_shake.trigger(0.8f * m_shakeScale);
        break;
    case EventKind::BossDown:
        m_flash.trigger(0.9f);
        m_flashColor = srgb(0xFFFFFF, 0.7f);
        m_shake.trigger(1.0f * m_shakeScale);
        m_sparks.burst(e.at, 40, {.speed = {200.0f, 700.0f}, .size = {8.0f, 16.0f}, .life = {0.5f, 1.2f}, .color = srgb(0xFFE6A0)});
        break;
    case EventKind::Death:
        m_flash.trigger(0.8f);
        m_flashColor = srgb(0xFFFFFF, 0.6f);
        break;
    case EventKind::Upgrade:
    case EventKind::Equip: break;
    default: break;
    }
}

void Effects::update(const float seconds, const game::Session& session, const Rect visible, const Vec2 heroFeet)
{
    m_numbers.update(seconds);
    m_arcs.update(seconds);
    m_spots.update(seconds);
    m_popups.update(seconds);
    m_ghosts.update(seconds);
    m_sparks.update(seconds);
    m_petals.update(seconds);
    m_motes.update(seconds, visible);
    m_shake.update(seconds);
    m_flash.update(seconds);
    m_popupClock += seconds;
    if (m_popupClock > 1.2f) {
        m_popupRow = 0;
    }
    // Afterimages while the hyper jump carries the hero.
    const game::Hero& h = session.hero();
    if (h.hyperTicks > 0) {
        m_ghostClock -= seconds;
        if (m_ghostClock <= 0.0f) {
            m_ghosts.add({.feet = heroFeet, .facing = h.body.facing}, 0.32f);
            m_ghostClock = 0.035f;
        }
    }
}

void Effects::drawBehind(gamelib::draw::DrawList& list, const Shapes& shapes, const Vec2 heroFeet, const float time) const
{
    (void)time;
    for (const auto& item : m_spots) {
        const Spot& s = item.value;
        if (s.kind != Fx::Shockwave && s.kind != Fx::Puff && s.kind != Fx::LevelUp) {
            continue;
        }
        const Vec2 at = s.attached ? heroFeet + s.at : s.at;
        addFx(list, shapes, s.kind, at, s.size, s.core, s.edge, item.progress(), s.variant,
              s.seed);
    }
}

void Effects::drawFront(gamelib::draw::DrawList& list, const Shapes& shapes, const Vec2 heroFeet, const int heroFacing,
                        const float time) const
{
    (void)heroFacing;
    (void)time;
    // Petals and sparks.
    for (const auto& p : m_petals.particles()) {
        const float t = std::clamp(p.age / p.life, 0.0f, 1.0f);
        addFx(list, shapes, Fx::Petal, p.position, p.size * (1.0f - 0.3f * t), p.color.withAlpha(1.0f - t), p.color.scaled(1.2f), t,
              0.0f, p.angle);
    }
    for (const auto& p : m_sparks.particles()) {
        const float t = std::clamp(p.age / p.life, 0.0f, 1.0f);
        const float len = p.size * (2.4f - 1.4f * t);
        const float ang = std::atan2(p.velocity.y, p.velocity.x);
        list.add({.rect = {p.position.x - len, p.position.y - p.size * 0.35f, len * 2.0f, p.size * 0.7f},
                  .color = p.color.withAlpha(1.0f - t * t),
                  .color2 = p.color.scaled(1.4f),
                  .shape = shapes.fx,
                  .rotation = ang,
                  .params = {static_cast<float>(Fx::Streak), t * 0.6f, 0.0f, 0.0f}});
    }
    // Arcs follow the hero.
    for (const auto& item : m_arcs) {
        const Arc& a = item.value;
        const Vec2 at = a.attached ? heroFeet + Vec2{a.offset.x, a.offset.y} : a.offset;
        addFx(list, shapes, a.kind, at, a.size, a.core, a.edge, item.progress(), static_cast<float>(a.variant),
              static_cast<float>(a.facing));
    }
    for (const auto& item : m_spots) {
        const Spot& s = item.value;
        if (s.kind == Fx::Shockwave || s.kind == Fx::Puff || s.kind == Fx::LevelUp) {
            continue;
        }
        const Vec2 at = s.attached ? heroFeet + s.at : s.at;
        addFx(list, shapes, s.kind, at, s.size, s.core, s.edge, item.progress(), s.variant, s.seed);
    }
    m_motes.draw(list, m_moteLook);

    // Damage numbers: chunky outlined digits, stacked per target, never darkened by the night.
    list.setLayer(gamelib::draw::layers::Hud);
    for (const auto& n : m_numbers.numbers()) {
        const gamelib::fx::DamageNumbers::Shown shown = m_numbers.shown(n);
        if (shown.alpha <= 0.0f) {
            continue;
        }
        Color top;
        Color bottom;
        numberColors(numberKind(n.color), top, bottom);
        const float h = shown.pixelSize * 9.0f;
        const float advance = h * 0.6f;
        const bool word = !n.text.empty() && (n.text[0] < '0' || n.text[0] > '9') && n.text[0] != '+';
        if (word) {
            gamelib::draw::drawVectorText(list, n.text, shown.center.x, shown.center.y - h * 0.4f,
                                          {.size = h * 0.7f, .color = top.withAlpha(shown.alpha), .weight = 1.8f,
                                           .align = gamelib::draw::Align::Center,
                                           .shadow = {.color = bottom.scaled(0.4f).withAlpha(shown.alpha), .offset = {0.0f, 0.0f}, .weight = 4.0f}});
            continue;
        }
        const float width = advance * static_cast<float>(n.text.size());
        float x = shown.center.x - width * 0.5f + advance * 0.5f;
        for (std::size_t i = 0; i < n.text.size(); ++i) {
            const char ch = n.text[i];
            const float digit = ch == '+' ? 10.0f : static_cast<float>(ch - '0');
            const float bob = (i % 2 == 0 ? -1.0f : 1.0f) * h * 0.05f;
            list.add({.rect = {x - h * 0.5f, shown.center.y - h * 0.5f + bob, h, h},
                      .color = top.withAlpha(shown.alpha),
                      .color2 = bottom,
                      .shape = shapes.ui,
                      .params = {static_cast<float>(Ui::Digit), digit, 0.0f, 0.0f}});
            x += advance;
        }
    }
    // Pickup and level-up popups.
    for (const auto& item : m_popups) {
        const Popup& p = item.value;
        const float t = item.progress();
        const float rise = gamelib::core::easeOutCubic(std::min(t * 2.5f, 1.0f)) * 30.0f;
        const float alpha = 1.0f - gamelib::core::smoothstep(0.7f, 1.0f, t);
        const Vec2 at = p.at + Vec2{0.0f, -rise};
        const bool big = p.text == "level up!";
        const float size = big ? 64.0f : 30.0f;
        float textX = at.x;
        if (p.icon != 0) {
            const float w = gamelib::draw::vectorTextWidth(p.text, size);
            const IconLook icon = itemIcon(p.icon);
            addIcon(list, shapes, Ui::ItemIcon, {at.x - w * 0.5f - 6.0f, at.y + size * 0.5f}, 40.0f, icon.id, icon.style,
                    icon.a.withAlpha(alpha), icon.b);
            textX += 18.0f;
        }
        gamelib::draw::drawVectorText(list, p.text, textX, at.y,
                                      {.size = size, .color = p.color.withAlpha(alpha), .weight = big ? 2.2f : 1.5f,
                                       .align = gamelib::draw::Align::Center,
                                       .shadow = {.color = srgb(0x3A2A40, alpha * 0.9f), .offset = {0.0f, 0.0f}, .weight = big ? 5.0f : 3.6f}});
    }
}

void Effects::drawScreen(gamelib::draw::DrawList& list, const gamelib::core::Size canvas) const
{
    if (m_flash.active()) {
        gamelib::fx::drawFlash(list, canvas, m_flash, m_flashColor);
    }
}

} // namespace wayfarer::app
