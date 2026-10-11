#include "view.hpp"

#include "look.hpp"

#include <gamelib/core/collision.hpp>
#include <gamelib/core/easing.hpp>
#include <gamelib/draw/parallax.hpp>
#include <gamelib/draw/tile_map.hpp>
#include <gamelib/draw/vector_text.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <numbers>

namespace wayfarer::app {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;
namespace layers = gamelib::draw::layers;

constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kAvatarQuad = 158.0f; // the hero is about 125 units tall
constexpr int kLayerDecoBack = -250;
constexpr int kLayerHouses = -240;
constexpr int kLayerClimb = -160;
constexpr int kLayerTerrain = -150;
constexpr int kLayerFoothold = -140;
constexpr int kLayerPortal = -60;
constexpr int kLayerNpc = 10;
constexpr int kLayerLoot = 20;
constexpr int kLayerMonster = 30;
constexpr int kLayerBoss = 25;
constexpr int kLayerHero = 50;
constexpr int kLayerFront = 200;
constexpr int kLayerEffects = 300;
constexpr int kLayerWorldUi = layers::Hud; // not lit, not graded

float hashf(const float x)
{
    const float s = std::sin(x * 12.9898f) * 43758.5453f;
    return s - std::floor(s);
}

Rect lerpR(const Rect& a, const Rect& b, const float t)
{
    return gamelib::core::lerp(a, b, t);
}

struct DecoSize {
    float width;
    float height;
};

DecoSize decoSize(const game::DecoKind k)
{
    using K = game::DecoKind;
    switch (k) {
    case K::House: return {560.0f, 560.0f};
    case K::Tree: return {320.0f, 320.0f};
    case K::Pine: return {280.0f, 300.0f};
    case K::BigShroom: return {280.0f, 280.0f};
    case K::Bush: return {200.0f, 128.0f};
    case K::Flowers: return {120.0f, 96.0f};
    case K::Grass: return {96.0f, 72.0f};
    case K::Rock: return {160.0f, 120.0f};
    case K::Crystal: return {140.0f, 140.0f};
    case K::Lamp: return {130.0f, 240.0f};
    case K::Fence: return {260.0f, 100.0f};
    case K::Sign: return {140.0f, 170.0f};
    case K::Barrel: return {80.0f, 90.0f};
    case K::Crate: return {80.0f, 80.0f};
    case K::FlowerPot: return {80.0f, 80.0f};
    case K::Well: return {220.0f, 230.0f};
    case K::Bench: return {150.0f, 96.0f};
    case K::Cart: return {170.0f, 120.0f};
    case K::Reeds: return {130.0f, 160.0f};
    case K::Column: return {150.0f, 280.0f};
    case K::Arch: return {300.0f, 330.0f};
    case K::Torch: return {80.0f, 150.0f};
    case K::Waystone: return {140.0f, 210.0f};
    case K::Snowpile: return {170.0f, 90.0f};
    case K::Lantern: return {110.0f, 210.0f};
    case K::Banner: return {120.0f, 240.0f};
    case K::Anvil: return {130.0f, 120.0f};
    case K::Stall: return {260.0f, 240.0f};
    case K::Count: break;
    }
    return {100.0f, 100.0f};
}

// Colors of a decoration by region (foliage, mushroom caps, stone).
void decoColors(const game::DecoKind k, const game::Region r, const int variant, Color& a, Color& b)
{
    using K = game::DecoKind;
    using R = game::Region;
    a = srgb(0xFFFFFF);
    b = srgb(0xFFFFFF);
    switch (k) {
    case K::Tree:
        a = r == R::Marsh ? srgb(0x6FB88A) : r == R::Ruins ? srgb(0xB8C870) : (variant == 1 ? srgb(0x78C860) : srgb(0x66BE5A));
        b = variant == 1 ? srgb(0xEC5662) : srgb(0xF790B6);
        break;
    case K::Pine: a = srgb(0x3F9A6A); break;
    case K::BigShroom:
        a = r == R::Mines ? srgb(0x7FD8FF) : r == R::Marsh ? srgb(0xA070E0) : (variant == 1 ? srgb(0xF4C24E) : srgb(0xEC5662));
        b = srgb(0xFFF3D6);
        break;
    case K::Bush:
        a = r == R::Marsh ? srgb(0x5CA87A) : srgb(0x6CC062);
        b = variant == 1 ? srgb(0xFFD84E) : srgb(0xF790B6);
        break;
    case K::Flowers:
        a = variant == 1 ? srgb(0xFFD84E) : srgb(0xF790B6);
        b = variant == 1 ? srgb(0xFFFFFF) : srgb(0xA0B4FF);
        break;
    case K::Grass: a = r == R::Ruins ? srgb(0xC8C870) : r == R::Peak ? srgb(0xA8C8B0) : srgb(0x7CCB58); break;
    case K::Rock:
        a = r == R::Peak ? srgb(0xB8C4DC) : r == R::Ruins ? srgb(0xE0C08E) : srgb(0xA8A4BC);
        b = srgb(0x7CCB58);
        break;
    case K::Crystal: a = r == R::Peak ? srgb(0xA8E8FF) : (variant == 1 ? srgb(0xD08CFF) : srgb(0x7FE6FF)); break;
    case K::Fence: a = r == R::Town ? srgb(0xFFFFFF) : srgb(0xC8955A); break;
    case K::FlowerPot: a = variant == 1 ? srgb(0xFFD84E) : srgb(0xEC5662); break;
    case K::Reeds: a = srgb(0x6FAA5A); break;
    case K::Column: a = srgb(0xE6D2A6); break;
    case K::Arch: a = srgb(0xD8D2C4); break;
    case K::Banner: a = variant == 1 ? srgb(0x5CA8FF) : srgb(0xEC5662); break;
    case K::Stall: a = srgb(0xF790B6); b = srgb(0xFFF6EA); break;
    default: break;
    }
}

// House colors (wall, roof) by style.
void houseColors(const int style, Color& wall, Color& roof)
{
    switch (style) {
    case 0: wall = srgb(0xFFF1D6); roof = srgb(0xEC5662); break;  // inn
    case 1: wall = srgb(0xFDE6EE); roof = srgb(0x4E80E6); break;  // shop
    case 2: wall = srgb(0xEAE4F0); roof = srgb(0xF2C460); break;  // cottage
    case 3: wall = srgb(0xC8955A); roof = srgb(0x6DBE57); break;  // tree hut
    case 4: wall = srgb(0xF0E2CC); roof = srgb(0x5E6680); break;  // forge
    case 5: wall = srgb(0xFFF4E2); roof = srgb(0xF0924E); break;  // bakery
    case 6: wall = srgb(0xD8D4E8); roof = srgb(0x7C6CD8); break;  // observatory
    default: wall = srgb(0xFFFFFF); roof = srgb(0xEC5662); break;
    }
}

int expressionFor(const game::Session& s)
{
    const game::Hero& h = s.hero();
    if (!h.alive) {
        return static_cast<int>(anime::Expression::Knocked);
    }
    if (h.hurtTicks > 0) {
        return static_cast<int>(anime::Expression::Hurt);
    }
    if (h.combo.busy() || h.castTicks > 0) {
        return static_cast<int>(anime::Expression::Determined);
    }
    if (s.character().hp.current * 4 < s.character().hp.max) {
        return static_cast<int>(anime::Expression::Sad);
    }
    if (h.idleTicks > 240) {
        return static_cast<int>(anime::Expression::Happy);
    }
    return static_cast<int>(anime::Expression::Neutral);
}

} // namespace

WorldView::WorldView(const gamelib::core::Size canvas)
    : m_canvas{canvas}
    , m_camera{canvas}
    , m_squash{4.5f, 0.32f}
{
}

Rect WorldView::visible() const
{
    return m_camera.view().visibleWorld(m_canvas);
}

Vec2 WorldView::heroFeet(const game::Session& session, const float alpha) const
{
    const game::Hero& h = session.hero();
    return lerpR(h.previous, h.body.box, alpha).bottomCenter();
}

void WorldView::snap(const game::Session& session)
{
    m_map = session.mapId();
    m_camera.setBounds(session.map().bounds());
    const Vec2 feet = session.hero().body.box.bottomCenter();
    m_camera.centerOn(feet + Vec2{0.0f, -140.0f});
    m_squash.reset();
}

void WorldView::follow(const game::Session& session, const float alpha, const float seconds, const float time)
{
    (void)time;
    if (session.mapId() != m_map) {
        snap(session);
    }
    const game::Hero& h = session.hero();
    m_camera.follow({.position = heroFeet(session, alpha) + Vec2{0.0f, -150.0f},
                     .facing = static_cast<float>(h.body.facing),
                     .grounded = h.body.grounded || h.body.climbing},
                    seconds,
                    {.deadZone = {60.0f, 40.0f}, .lookahead = 120.0f, .lookaheadRate = 1.8f, .rate = 5.5f,
                     .verticalOnLanding = true, .maxRise = 300.0f});
    // Squash on landing and jumping.
    if (h.landTicks < m_lastLandTicks && h.body.grounded) {
        m_squash.kick(-0.16f);
    }
    m_lastLandTicks = h.landTicks;
    m_squash.update(seconds);
    // Faces blend from one expression to the next over a tenth of a second.
    const int wanted = expressionFor(session);
    if (wanted != m_expressionTo) {
        m_expressionFrom = m_expressionBlend > 0.5f ? m_expressionTo : m_expressionFrom;
        m_expressionTo = wanted;
        m_expressionBlend = 0.0f;
    }
    m_expressionBlend = std::min(1.0f, m_expressionBlend + seconds / 0.1f);
}

anime::Avatar WorldView::heroLook(const game::Session& s, const float alpha, const float time)
{
    (void)time;
    const game::Hero& h = s.hero();
    const auto& b = h.body;
    anime::Avatar a = heroAvatar(s.character());
    a.feet = lerpR(h.previous, b.box, alpha).bottomCenter();
    a.height = kAvatarQuad;
    a.facing = static_cast<float>(b.facing);
    using P = anime::AvatarPose;
    if (!h.alive) {
        a.pose = P::Hurt;
        a.phase = 0.0f;
    } else if (h.hurtTicks > 0) {
        a.pose = P::Hurt;
        a.phase = 1.0f - static_cast<float>(h.hurtTicks) / 18.0f;
    } else if (b.climbing) {
        a.pose = P::Climb;
        a.phase = std::fmod(b.box.top() / 70.0f, 1.0f);
        if (a.phase < 0.0f) {
            a.phase += 1.0f;
        }
    } else if (h.combo.busy()) {
        a.pose = P::Attack;
        const auto& clip = h.combo.clip();
        const float total = static_cast<float>(std::max(clip.totalTicks(), 1));
        const float swingAt = static_cast<float>(clip.startup) / total;
        const float p = static_cast<float>(h.combo.ticks()) / total;
        a.phase = p < swingAt ? p / std::max(swingAt, 1e-3f) * 0.4f : 0.4f + (p - swingAt) / std::max(1.0f - swingAt, 1e-3f) * 0.6f;
    } else if (h.castTicks > 0) {
        const float p = 1.0f - static_cast<float>(h.castTicks) / static_cast<float>(std::max(h.castLength, 1));
        if (h.casting == game::SkillId::Sweep) {
            a.pose = P::Attack;
            a.phase = 0.3f + p * 0.7f;
        } else if (h.casting == game::SkillId::Starbolt) {
            a.pose = P::Attack;
            a.phase = std::min(0.4f + p * 0.6f, 1.0f);
        } else {
            a.pose = P::Cheer;
            a.phase = p;
        }
    } else if (!b.grounded) {
        a.pose = P::Jump;
        a.phase = b.velocity.y < 0.0f ? 0.0f : 0.6f;
    } else if (std::abs(b.velocity.x) > 0.4f) {
        a.pose = P::Walk;
        a.phase = gamelib::fx::walkPhase(b.walked, 150.0f);
    } else {
        a.pose = P::Idle;
    }
    return a;
}

void WorldView::drawHero(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float alpha,
                         const float time) const
{
    const game::Hero& h = session.hero();
    anime::Avatar a = heroLook(session, alpha, time);
    a.expression = static_cast<anime::Expression>(m_expressionFrom);
    a.nextExpression = static_cast<anime::Expression>(m_expressionTo);
    a.blend = m_expressionBlend;
    gamelib::draw::Sprite sprite = anime::avatarSprite(a, shapes.avatar);
    // Squash and stretch, feet planted.
    const Vec2 sq = m_squash.scale();
    sprite.rect = gamelib::fx::scaledFromBottom(sprite.rect, {sq.x, sq.y});
    if (!h.body.grounded && !h.body.climbing && h.rollTicks == 0) {
        const float stretch = std::clamp(-h.body.velocity.y * 0.008f, -0.06f, 0.08f);
        sprite.rect = gamelib::fx::scaledFromBottom(sprite.rect, {1.0f - stretch * 0.6f, 1.0f + stretch});
    }
    if (h.rollTicks > 0) {
        // The dodge roll: a forward tumble around the body's middle.
        const float p = 1.0f - static_cast<float>(h.rollTicks) / static_cast<float>(std::max(h.rollLength, 1));
        sprite.rotation = static_cast<float>(h.body.facing) * gamelib::core::easeInOutQuad(p) * 2.0f * kPi;
        sprite.rect.y += kAvatarQuad * 0.18f * std::sin(p * kPi);
        sprite.params[1] = static_cast<float>(anime::AvatarPose::Jump);
    }
    // Invincibility: a soft blink rather than a hard flicker.
    if (h.vulnerable.invincible() && h.rollTicks == 0 && h.alive) {
        sprite.color.a = 0.55f + 0.45f * (0.5f + 0.5f * std::sin(time * 30.0f));
    }
    list.setLayer(kLayerHero);
    list.add(sprite);
}

void WorldView::cacheMap(const game::MapDef& def) const
{
    if (m_cached == def.id) {
        return;
    }
    m_cached = def.id;
    m_runs.clear();
    const auto& rows = def.text;
    const float t = game::kTile;
    // Foothold runs, row by row.
    for (int y = 0; y < def.rows; ++y) {
        int x = 0;
        while (x < def.columns) {
            const char c = rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
            const bool foot = c == game::kFoothold || ((c == game::kRope || c == game::kLadder) && x > 0 && x + 1 < def.columns
                                                       && (rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x - 1)] == game::kFoothold
                                                           || rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x + 1)] == game::kFoothold)
                                                       && (y == 0 || rows[static_cast<std::size_t>(y - 1)][static_cast<std::size_t>(x)] != c));
            if (!foot) {
                ++x;
                continue;
            }
            const int start = x;
            while (x < def.columns) {
                const char d = rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
                const bool cont = d == game::kFoothold
                                  || ((d == game::kRope || d == game::kLadder) && (y == 0 || rows[static_cast<std::size_t>(y - 1)][static_cast<std::size_t>(x)] != d));
                if (!cont) {
                    break;
                }
                ++x;
            }
            const float left = static_cast<float>(start) * t;
            const float width = static_cast<float>(x - start) * t;
            m_runs.push_back({.rect = {left - 10.0f, static_cast<float>(y) * t - 8.0f, width + 20.0f, 66.0f}, .worldLeft = left - 10.0f,
                              .kind = 1});
        }
    }
    // Ropes and ladders, column by column.
    for (int x = 0; x < def.columns; ++x) {
        int y = 0;
        while (y < def.rows) {
            const char c = rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
            if (c != game::kRope && c != game::kLadder) {
                ++y;
                continue;
            }
            const int start = y;
            while (y < def.rows && rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] == c) {
                ++y;
            }
            const float width = c == game::kRope ? 26.0f : 44.0f;
            m_runs.push_back({.rect = {static_cast<float>(x) * t + t * 0.5f - width * 0.5f, static_cast<float>(start) * t + 10.0f,
                                       width, static_cast<float>(y - start) * t + 6.0f},
                              .kind = c == game::kRope ? 2 : 3});
        }
    }
}

void WorldView::drawBackdrop(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float time) const
{
    (void)time;
    const game::MapDef& def = session.map();
    const float night = session.nightAmount();
    const RegionLook look = regionLook(def.region, def.dungeon, night);
    const auto region = static_cast<std::uint64_t>(backdropRegion(def.region));
    const Rect bounds = def.bounds();
    const Vec2 anchor{0.0f, bounds.bottom() - static_cast<float>(m_canvas.height)};
    const std::array layersList{
        gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = {0.0f, 0.0f}, .color = look.skyHorizon,
                                     .color2 = look.skyTop, .params = {0.0f, night}, .bits = region},
        gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = {0.06f, 0.03f}, .anchor = anchor, .color = look.cloud,
                                     .color2 = look.cloudShade, .params = {1.0f, night}, .bits = region},
        gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = {0.18f, 0.1f}, .anchor = anchor, .color = look.far,
                                     .color2 = look.farRim, .params = {2.0f, night}, .bits = region},
        gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = {0.4f, 0.25f}, .anchor = anchor, .color = look.near,
                                     .color2 = look.nearRim, .params = {3.0f, night}, .bits = region},
    };
    list.setLayer(layers::Background);
    gamelib::draw::drawParallax(list, std::span{layersList}.first(2), m_camera.view(), m_canvas);
    list.setLayer(layers::Parallax);
    gamelib::draw::drawParallax(list, std::span{layersList}.subspan(2), m_camera.view(), m_canvas);
    // The light layer over everything behind the terrain.
    const std::array lightLayer{gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = {0.3f, 0.0f},
                                                             .color = look.light, .params = {4.0f, night}, .bits = region}};
    list.setLayer(kLayerDecoBack - 5);
    gamelib::draw::drawParallax(list, lightLayer, m_camera.view(), m_canvas);
}

void WorldView::drawTerrain(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const
{
    const game::MapDef& def = session.map();
    cacheMap(def);
    const RegionLook look = regionLook(def.region, def.dungeon, 0.0f);
    const float style = static_cast<float>(look.terrain);
    gamelib::draw::TileSet tiles;
    tiles.set(gamelib::core::charTile(game::kGround),
              {.shape = shapes.terrain, .color = look.cap, .color2 = look.body, .params = {0.0f, style, 1.3f, 0.0f}});
    tiles.set(gamelib::core::charTile(game::kRampUp),
              {.shape = shapes.terrain, .color = look.cap, .color2 = look.body, .params = {4.0f, style, 1.3f, 0.0f}, .group = 0});
    tiles.set(gamelib::core::charTile(game::kRampDown),
              {.shape = shapes.terrain, .color = look.cap, .color2 = look.body, .params = {5.0f, style, 1.3f, 0.0f}, .group = 0});
    list.setLayer(kLayerTerrain);
    const Rect vis = visible().expanded(game::kTile);
    // Ramps join the ground under them (so no outline runs between), but the ground doesn't join a ramp beside it.
    gamelib::draw::drawTiles(list, def.tiles, 0, tiles, vis, {.outsideConnects = true});
    for (const Run& r : m_runs) {
        if (!gamelib::core::overlaps(r.rect, vis)) {
            continue;
        }
        list.setLayer(r.kind == 1 ? kLayerFoothold : kLayerClimb);
        list.add({.rect = r.rect,
                  .color = r.kind == 1 ? look.footCap : srgb(0xFFFFFF),
                  .color2 = look.footBody,
                  .shape = shapes.terrain,
                  .params = {static_cast<float>(r.kind), style, std::fmod(r.worldLeft * 0.013f, 7.0f), r.worldLeft}});
    }
}

void WorldView::drawDecos(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float time) const
{
    (void)time;
    const game::MapDef& def = session.map();
    const float night = session.nightAmount();
    const float lit = def.dungeon || def.region == game::Region::Mines ? 1.0f : gamelib::core::smoothstep(0.2f, 0.6f, night);
    const Rect vis = visible().expanded(300.0f);
    int index = 0;
    for (const game::Deco& d : def.decos) {
        ++index;
        const DecoSize size = decoSize(d.kind);
        const float w = size.width * d.scale;
        const float h = size.height * d.scale;
        if (d.at.x + w < vis.left() || d.at.x - w > vis.right()) {
            continue;
        }
        if (d.kind == game::DecoKind::House) {
            Color wall;
            Color roof;
            houseColors(d.variant, wall, roof);
            list.setLayer(kLayerHouses);
            list.add({.rect = {d.at.x - w * 0.5f, d.at.y - h * (122.0f / 128.0f), w, h},
                      .color = wall,
                      .color2 = roof,
                      .shape = shapes.building,
                      .params = {static_cast<float>(d.variant), lit, 0.35f, 0.0f},
                      .bits = static_cast<std::uint64_t>(index * 37 % 255) << 8});
            continue;
        }
        Color a;
        Color b;
        decoColors(d.kind, def.region, d.variant, a, b);
        list.setLayer(d.front ? kLayerFront : kLayerDecoBack);
        addDeco(list, shapes, static_cast<int>(d.kind), d.at, w, h, a, b, lit, static_cast<float>(d.variant),
                hashf(static_cast<float>(index)) * 10.0f);
    }
}

void WorldView::drawPortals(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float time) const
{
    (void)time;
    const game::MapDef& def = session.map();
    list.setLayer(kLayerPortal);
    for (int i = 0; i < static_cast<int>(def.portals.size()); ++i) {
        const game::Portal& p = def.portals[static_cast<std::size_t>(i)];
        const bool open = session.portalOpen(i);
        if (p.afterBoss && !open) {
            continue;
        }
        if (p.kind == game::PortalKind::Waystone) {
            const Color rune = regionAccent(static_cast<game::Region>(p.chapter));
            addDeco(list, shapes, static_cast<int>(game::DecoKind::Waystone), p.at, 140.0f, 210.0f, rune, rune,
                    open ? 1.0f : 0.0f, 0.0f, static_cast<float>(i));
            continue;
        }
        const Vec2 at = p.at + Vec2{p.kind == game::PortalKind::Edge ? (p.at.x < def.bounds().width * 0.5f ? 20.0f : -20.0f) : 0.0f,
                                    -86.0f};
        addFx(list, shapes, Fx::Portal, at, 200.0f, srgb(0xFFFFFF).scaled(1.3f),
              p.afterBoss ? srgb(0xFFD86A) : srgb(0x9FD8FF), open ? 1.0f : 0.25f, 0.0f, static_cast<float>(i));
    }
}

void WorldView::drawNpcs(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float time) const
{
    const Vec2 heroFeet = session.hero().body.box.bottomCenter();
    list.setLayer(kLayerNpc);
    for (const game::NpcHere& n : session.npcs()) {
        anime::Avatar a = npcAvatar(n.npc);
        a.feet = n.feet;
        a.height = kAvatarQuad;
        const float dx = heroFeet.x - n.feet.x;
        const bool near = std::abs(dx) < 260.0f && std::abs(heroFeet.y - n.feet.y) < 160.0f;
        a.facing = near ? (dx < 0.0f ? -1.0f : 1.0f) : static_cast<float>(n.facing);
        a.pose = anime::AvatarPose::Idle;
        if (near && a.expression == anime::Expression::Neutral) {
            a.expression = anime::Expression::Happy;
            a.nextExpression = anime::Expression::Happy;
        }
        // Brann hammers at the anvil now and then.
        if (n.npc == game::NpcId::Brann && !near) {
            const float cycle = std::fmod(time, 2.4f);
            if (cycle < 0.7f) {
                a.pose = anime::AvatarPose::Attack;
                a.phase = cycle / 0.7f;
            }
        }
        list.add(anime::avatarSprite(a, shapes.avatar));
    }
}

void WorldView::drawMonsters(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float alpha,
                             const float time) const
{
    (void)time;
    list.setLayer(kLayerMonster);
    for (const game::Monster& m : session.monsters()) {
        const Rect box = lerpR(m.previous, m.box, alpha);
        const game::Species& sp = m.kind();
        const MonsterColors colors = monsterColors(m.species, m.elite);
        anime::Monster look;
        look.feet = box.bottomCenter();
        look.height = monsterScale(sp.look, box.height, m.elite).quad;
        look.facing = static_cast<float>(m.facing);
        look.kind = static_cast<anime::MonsterKind>(monsterShaderKind(sp.look));
        look.body = colors.body;
        look.accent = colors.accent;
        look.elite = m.elite;
        look.seed = m.seed;
        using MS = anime::MonsterState;
        float progress = m.progress();
        MS state = MS::Idle;
        switch (m.state) {
        case game::MonsterState::Spawn:
            state = MS::Idle;
            look.body = look.body.withAlpha(progress);
            break;
        case game::MonsterState::Idle: state = MS::Idle; break;
        case game::MonsterState::Move:
            state = MS::Move;
            if (sp.behavior == game::Behavior::Hopper) {
                progress = m.grounded ? 0.05f : 0.2f + 0.6f * std::clamp((m.velocity.y + 7.0f) / 14.0f, 0.0f, 1.0f);
            } else {
                progress = std::fmod(box.center().x / 70.0f + 10.0f, 1.0f);
            }
            break;
        case game::MonsterState::Windup: state = MS::Windup; break;
        case game::MonsterState::Attack:
            state = MS::Move;
            progress = sp.behavior == game::Behavior::Hopper ? 0.5f : std::fmod(box.center().x / 50.0f + 10.0f, 1.0f);
            break;
        case game::MonsterState::Hit: state = MS::Hit; break;
        case game::MonsterState::Defeat: state = MS::Defeat; break;
        }
        look.state = state;
        look.progress = progress;
        list.add(anime::monsterSprite(look, shapes.monster));
    }
}

void WorldView::drawBoss(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float alpha,
                         const float time) const
{
    (void)time;
    if (!session.boss()) {
        return;
    }
    const game::Boss& b = *session.boss();
    const Rect box = lerpR(b.previous, b.box, alpha);
    anime::Boss look;
    look.feet = box.bottomCenter();
    look.height = b.info().size.y * 2.15f;
    look.facing = static_cast<float>(b.facing);
    look.phase = b.phase;
    look.kind = static_cast<int>(b.id);
    look.guarded = b.guarded;
    const MonsterColors bc = bossColors(b.id);
    look.body = bc.body;
    look.cap = bc.accent;
    using MS = anime::MonsterState;
    switch (b.state) {
    case game::BossState::Intro:
    case game::BossState::Idle: look.state = std::abs(b.velocity.x) > 0.3f ? MS::Move : MS::Idle; break;
    case game::BossState::Windup: look.state = MS::Windup; break;
    case game::BossState::Act: look.state = MS::Move; break;
    case game::BossState::Recover: look.state = MS::Idle; break;
    case game::BossState::Defeat: look.state = MS::Defeat; break;
    }
    look.progress = look.state == MS::Move ? std::fmod(box.center().x / 120.0f + 10.0f, 1.0f) : b.progress();
    if (session.ticks() - b.lastHitTick < 10 && b.alive()) {
        look.state = MS::Hit;
        look.progress = static_cast<float>(session.ticks() - b.lastHitTick) / 10.0f;
    }
    gamelib::draw::Sprite sprite = anime::bossSprite(look, shapes.boss);
    sprite.color.a = b.fade;
    list.setLayer(kLayerBoss);
    list.add(sprite);
}

void WorldView::drawLoot(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float alpha,
                         const float time) const
{
    list.setLayer(kLayerLoot);
    int i = 0;
    for (const game::Drop& d : session.drops()) {
        ++i;
        const Rect box = lerpR(d.previous, d.box, alpha);
        const float bob = d.grounded ? std::sin(time * 3.0f + static_cast<float>(i)) * 4.0f - 4.0f : 0.0f;
        const Vec2 at = box.center() + Vec2{0.0f, bob};
        if (d.coins > 0) {
            addFx(list, shapes, Fx::Coin, at, 40.0f, srgb(0xF6C845), srgb(0xFFFFFF), std::fmod(time * 0.8f + static_cast<float>(i) * 0.3f, 1.0f));
            continue;
        }
        const game::Rarity rarity = game::rarityOf(d.stack);
        const Color glow = rarityColor(rarity);
        addFx(list, shapes, Fx::LootGlow, at, 110.0f, glow.withAlpha(rarity == game::Rarity::Common ? 0.35f : 0.8f), glow.scaled(1.3f),
              0.0f, rarity >= game::Rarity::Epic ? 1.0f : 0.0f, static_cast<float>(i));
        const IconLook icon = itemIcon(d.stack.item);
        addIcon(list, shapes, Ui::ItemIcon, at, 54.0f, icon.id, icon.style, icon.a, icon.b);
    }
}

void WorldView::drawShots(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float time) const
{
    (void)time;
    list.setLayer(kLayerEffects - 10);
    for (const auto& s : session.shots().items()) {
        if (s.extra.hero) {
            const float ang = std::atan2(s.velocity.y, s.velocity.x);
            list.add({.rect = {s.position.x - 90.0f, s.position.y - 20.0f, 90.0f, 40.0f},
                      .color = srgb(0xFFF2B0).withAlpha(0.8f),
                      .color2 = srgb(0xFFC84A),
                      .shape = shapes.fx,
                      .rotation = ang,
                      .params = {static_cast<float>(Fx::Streak), 0.3f, 0.0f, 0.0f}});
            addFx(list, shapes, Fx::Star, s.position, 92.0f, srgb(0xFFF6C8).scaled(1.5f), srgb(0xFFC84A).scaled(1.2f), 0.0f, 0.0f,
                  static_cast<float>(s.id));
            continue;
        }
        Color core = srgb(0xFFFFFF);
        Color edge = srgb(0xFFFFFF);
        switch (s.extra.look) {
        case game::ShotLook::Spore: core = srgb(0xC8F4A0); edge = srgb(0x86D065); break;
        case game::ShotLook::Rock: core = srgb(0x9C98B4); edge = srgb(0x7FE6FF); break;
        case game::ShotLook::Bubble: core = srgb(0x9AE6C8); edge = srgb(0xFFFFFF); break;
        case game::ShotLook::Flame: core = srgb(0xFFB040); edge = srgb(0xFF6A2A); break;
        case game::ShotLook::Ice: core = srgb(0xC8F0FF); edge = srgb(0x7AC8FF); break;
        case game::ShotLook::Wave: core = srgb(0xFFF2D0); edge = srgb(0xFFD08A); break;
        case game::ShotLook::Star: core = srgb(0xFFF2B0); edge = srgb(0xFFC84A); break;
        }
        const float size = std::max(s.size.x, s.size.y) * (s.extra.groundWave ? 2.4f : 1.9f);
        addFx(list, shapes, Fx::Shot, s.position, size, core, edge, 0.0f, static_cast<float>(s.extra.look), static_cast<float>(s.id));
    }
    // The rain's warnings, on the arena floor.
    if (session.map().arena) {
        const float floorY = session.map().arena->bounds.bottom();
        for (const auto& item : session.rain().items()) {
            addFx(list, shapes, Fx::RainWarn, {item.value, floorY - 60.0f}, 140.0f, srgb(0xFF6A7A), srgb(0xFFB0B8), item.progress());
        }
    }
}

void WorldView::drawLights(gamelib::draw::DrawList& list, const game::Session& session, const float alpha, const float time) const
{
    (void)time;
    const game::MapDef& def = session.map();
    const float night = session.nightAmount();
    const bool dark = def.dungeon || def.region == game::Region::Mines;
    const float lightAmount = dark ? 1.0f : gamelib::core::smoothstep(0.15f, 0.7f, night);
    const RegionLook look = regionLook(def.region, def.dungeon, night);
    list.setPostProcess({.ambient = look.ambient,
                         .bloom = {.intensity = look.bloom, .threshold = 0.85f, .radius = 1.2f},
                         .grade = look.grade,
                         .overlayLayer = layers::Hud});
    if (lightAmount <= 0.01f) {
        return;
    }
    const Rect vis = visible().expanded(400.0f);
    int seed = 0;
    for (const game::Deco& d : def.decos) {
        ++seed;
        if (!vis.contains(d.at)) {
            continue;
        }
        using K = game::DecoKind;
        const DecoSize size = decoSize(d.kind);
        switch (d.kind) {
        case K::Lamp:
            list.light({.position = d.at + Vec2{size.width * 0.11f, -size.height * 0.78f}, .radius = 320.0f,
                        .color = srgb(0xFFC27A).withAlpha(lightAmount), .intensity = 1.1f, .flicker = 0.08f,
                        .seed = static_cast<std::uint32_t>(seed)});
            break;
        case K::Torch:
            list.light({.position = d.at + Vec2{0.0f, -size.height * 0.62f}, .radius = 300.0f,
                        .color = srgb(0xFF9E4A).withAlpha(lightAmount), .intensity = 1.2f, .flicker = 0.3f,
                        .seed = static_cast<std::uint32_t>(seed)});
            break;
        case K::Lantern:
            list.light({.position = d.at + Vec2{size.width * 0.1f, -size.height * 0.72f}, .radius = 260.0f,
                        .color = srgb(0xFFC27A).withAlpha(lightAmount), .intensity = 1.0f, .flicker = 0.12f,
                        .seed = static_cast<std::uint32_t>(seed)});
            break;
        case K::Crystal:
            list.light({.position = d.at + Vec2{0.0f, -50.0f}, .radius = 240.0f, .color = srgb(0x7FE6FF).withAlpha(lightAmount * 0.8f),
                        .intensity = 0.9f, .flicker = 0.1f, .seed = static_cast<std::uint32_t>(seed)});
            break;
        case K::House:
            // Warm light spilling from the windows.
            list.light({.position = d.at + Vec2{0.0f, -size.height * 0.4f * d.scale}, .radius = 360.0f * d.scale,
                        .color = srgb(0xFFB868).withAlpha(lightAmount * 0.9f), .intensity = 0.9f, .flicker = 0.05f,
                        .seed = static_cast<std::uint32_t>(seed)});
            if (d.variant == 4) {
                list.light({.position = d.at + Vec2{size.width * 0.05f, -60.0f}, .radius = 300.0f,
                            .color = srgb(0xFF8A3A), .intensity = 1.3f, .flicker = 0.35f, .seed = 77});
            }
            break;
        default: break;
        }
    }
    for (int i = 0; i < static_cast<int>(def.portals.size()); ++i) {
        const game::Portal& p = def.portals[static_cast<std::size_t>(i)];
        if (!session.portalOpen(i) || !vis.contains(p.at)) {
            continue;
        }
        list.light({.position = p.at + Vec2{0.0f, -90.0f}, .radius = 260.0f,
                    .color = (p.kind == game::PortalKind::Waystone ? regionAccent(static_cast<game::Region>(p.chapter)) : srgb(0x9FD8FF))
                                 .withAlpha(lightAmount),
                    .intensity = 0.9f, .flicker = 0.1f, .seed = static_cast<std::uint32_t>(i * 11)});
    }
    // The hero's lantern in the dark places, and always a little glow.
    const Vec2 feet = heroFeet(session, alpha);
    list.light({.position = feet + Vec2{0.0f, -80.0f}, .radius = dark ? 520.0f : 300.0f,
                .color = srgb(0xFFE2B0).withAlpha(dark ? 1.0f : lightAmount * 0.7f), .intensity = dark ? 1.15f : 0.8f,
                .flicker = dark ? 0.06f : 0.0f, .seed = 5});
    for (const auto& s : session.shots().items()) {
        if (s.extra.hero) {
            list.light({.position = s.position, .radius = 220.0f, .color = srgb(0xFFD870).withAlpha(lightAmount), .intensity = 1.0f});
        }
    }
    if (session.boss()) {
        const game::Boss& b = *session.boss();
        list.light({.position = b.box.center(), .radius = 420.0f, .color = srgb(0xFFE0C0).withAlpha(lightAmount * 0.6f), .intensity = 0.8f});
    }
}

void WorldView::drawLabels(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float alpha,
                           const float time) const
{
    using gamelib::draw::Align;
    list.setLayer(kLayerWorldUi);
    const auto tag = [&](const std::string_view text, const Vec2 at, const Color fill, const Color ink, const float size) {
        const float w = gamelib::draw::vectorTextWidth(text, size) + size * 0.9f;
        addUi(list, shapes, Ui::Tooltip, {at.x - w * 0.5f - 10.0f, at.y - 10.0f, w + 20.0f, size * 1.25f + 20.0f}, fill, fill.scaled(0.7f));
        gamelib::draw::drawVectorText(list, text, at.x, at.y + size * 0.12f, {.size = size, .color = ink, .weight = 1.25f, .align = Align::Center});
    };
    // NPCs: name tags and quest markers.
    for (const game::NpcHere& n : session.npcs()) {
        tag(game::npcName(n.npc), n.feet + Vec2{0.0f, 12.0f}, srgb(0x3A2E4A, 0.82f), srgb(0xFFF2D8), 24.0f);
        const gamelib::core::QuestMarker marker = session.quests().markerFor(static_cast<int>(n.npc));
        if (marker != gamelib::core::QuestMarker::None) {
            const float bob = std::sin(time * 3.0f + static_cast<float>(n.npc)) * 5.0f;
            const int kind = marker == gamelib::core::QuestMarker::Available ? 0 : marker == gamelib::core::QuestMarker::ReadyToTurnIn ? 2 : 1;
            const Color c = kind == 0 ? srgb(0xFFC23A) : kind == 2 ? srgb(0x4CC86A) : srgb(0xB8B0C8);
            addIcon(list, shapes, Ui::Marker, n.feet + Vec2{0.0f, -kAvatarQuad * 0.95f + bob}, 64.0f, static_cast<float>(kind), 0.0f, c, c);
        }
    }
    // Monsters: health bars after a hit, names for elites.
    for (const game::Monster& m : session.monsters()) {
        if (!m.alive() || m.state == game::MonsterState::Spawn) {
            continue;
        }
        const Rect box = lerpR(m.previous, m.box, alpha);
        const bool recent = session.ticks() - m.lastHitTick < 240;
        if (recent || m.elite) {
            const float w = std::max(box.width * 1.2f, 70.0f);
            const float drawn = monsterScale(m.kind().look, box.height, m.elite).top;
            const Rect bar{box.center().x - w * 0.5f, box.bottom() - drawn - 26.0f, w, 14.0f};
            addUi(list, shapes, Ui::Gauge, bar, srgb(0xF0606E), srgb(0xFFC8C8),
                  static_cast<float>(m.hp) / static_cast<float>(std::max(m.maxHp, 1)), 0.0f);
            const std::string name = std::format("Lv.{} {}{}", m.level, m.elite ? "elite " : "", m.kind().name);
            tag(name, {box.center().x, box.bottom() + 8.0f}, m.elite ? srgb(0x5A2E6A, 0.85f) : srgb(0x2E2A3A, 0.75f),
                m.elite ? srgb(0xFFD86A) : srgb(0xFFFFFF), 19.0f);
        }
    }
    // Portal labels.
    const game::MapDef& def = session.map();
    const Vec2 heroFeet = session.hero().body.box.bottomCenter();
    for (int i = 0; i < static_cast<int>(def.portals.size()); ++i) {
        const game::Portal& p = def.portals[static_cast<std::size_t>(i)];
        if (p.afterBoss && !session.portalOpen(i)) {
            continue;
        }
        if (std::abs(p.at.x - heroFeet.x) > 420.0f) {
            continue;
        }
        const std::string_view name = p.label.empty() ? game::mapDef(p.to).name : p.label;
        const bool open = session.portalOpen(i);
        const Vec2 at = p.at + Vec2{p.kind == game::PortalKind::Edge ? (p.at.x < def.bounds().width * 0.5f ? 60.0f : -60.0f) : 0.0f,
                                    p.kind == game::PortalKind::Waystone ? -240.0f : -210.0f};
        tag(open ? name : "sealed", at, open ? srgb(0x2E4A6A, 0.8f) : srgb(0x4A3A3A, 0.7f), open ? srgb(0xD8F2FF) : srgb(0xC8B8B8), 22.0f);
    }
}

void WorldView::draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Effects& effects,
                     const float alpha, const float time, const Vec2 shake) const
{
    list.setView(m_camera.view(shake));
    drawBackdrop(list, shapes, session, time);
    drawDecos(list, shapes, session, time);
    drawTerrain(list, shapes, session);
    drawPortals(list, shapes, session, time);
    const Vec2 feet = heroFeet(session, alpha);
    list.setLayer(kLayerNpc - 5);
    effects.drawBehind(list, shapes, feet, time);
    drawNpcs(list, shapes, session, time);
    drawLoot(list, shapes, session, alpha, time);
    drawBoss(list, shapes, session, alpha, time);
    drawMonsters(list, shapes, session, alpha, time);
    // The hyper jump's afterimages behind the hero.
    list.setLayer(kLayerHero - 1);
    for (const auto& g : effects.ghosts()) {
        anime::Avatar a = heroLook(session, alpha, time);
        a.feet = g.value.feet;
        a.facing = static_cast<float>(g.value.facing);
        a.pose = anime::AvatarPose::Jump;
        gamelib::draw::Sprite s = anime::avatarSprite(a, shapes.avatar);
        s.color = srgb(0x9FE6FF).withAlpha(0.45f * (1.0f - g.progress()));
        s.color2 = srgb(0xBFF2FF);
        list.add(s);
    }
    drawHero(list, shapes, session, alpha, time);
    // A Tailwind swirl around the hero while it lasts.
    if (session.character().buffs.active(game::kTailwindBuff)) {
        list.setLayer(kLayerHero + 1);
        addFx(list, shapes, Fx::Wind, feet + Vec2{0.0f, -62.0f}, 190.0f, srgb(0xE8FFF4).withAlpha(0.55f), srgb(0x9AF0C8), 0.0f);
    }
    drawShots(list, shapes, session, time);
    list.setLayer(kLayerEffects);
    effects.drawFront(list, shapes, feet, session.hero().body.facing, time);
    drawLabels(list, shapes, session, alpha, time);
    drawLights(list, session, alpha, time);
    list.setView({});
}

} // namespace wayfarer::app
