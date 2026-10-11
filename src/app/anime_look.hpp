// C++ side of the anime shaders (copied from GameLib's shaders/samples/anime and
// grown with the game): packs each shader's fields into a draw::Sprite exactly
// as the shader unpacks them.
#pragma once

#include <gamelib/core/color.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/draw/sprite.hpp>

#include <algorithm>
#include <cstdint>

namespace wayfarer::app::anime {

// Indices into kAnimePalette (gamelib/anime.glsl).
enum class Swatch : std::uint8_t {
    Cream, Plum, Red, Orange, Yellow, Leaf, Forest, Sky, Blue, Navy, Purple, Pink, Brown, Tan, Slate, Gold
};

// The face kit's named expressions (kAnime* in gamelib/anime_face.glsl).
enum class Expression : std::uint8_t { Neutral, Happy, Angry, Sad, Surprised, Hurt, Blink, Determined, Knocked };

// ---------------------------------------------------------------------------
// avatar.frag

enum class AvatarPose : std::uint8_t { Idle, Walk, Jump, Attack, Hurt, Cheer, Climb };
enum class Hair : std::uint8_t { Short, Ponytail, Bob, Long, TwinTails };
enum class Hat : std::uint8_t { None, Headband, Wizard, Beanie, Helmet, Ribbon };
enum class Top : std::uint8_t { Tunic, Robe, Vest, Armor, Sailor };
enum class Bottom : std::uint8_t { Pants, Shorts, Skirt };
enum class Shoes : std::uint8_t { Boots, Shoes, Greaves };
enum class Gloves : std::uint8_t { None, Cloth, Gauntlets };
enum class Weapon : std::uint8_t { None, Sword, Staff, Bow, Dagger, Axe };
enum class Cape : std::uint8_t { None, Cape, Scarf };

// One chibi avatar: base looks (hair, eyes, skin) plus equipment slots.
struct Avatar {
    gamelib::core::Vec2 feet;      // where the feet touch the ground
    float height = 96.0f; // the quad height; the avatar is about 0.77 of it
    float facing = 1.0f;  // -1 faces left
    AvatarPose pose = AvatarPose::Idle;
    float phase = 0.0f;   // walk / climb cycle or attack progress, 0..1
    gamelib::core::Color hairColor = gamelib::core::srgb(0x6B4A3A);
    gamelib::core::Color topColor = gamelib::core::srgb(0x4E80E6);
    Hair hair = Hair::Short;
    Hat hat = Hat::None;
    Top top = Top::Tunic;
    Bottom bottom = Bottom::Pants;
    Shoes shoes = Shoes::Boots;
    Gloves gloves = Gloves::None;
    Weapon weapon = Weapon::None;
    Cape cape = Cape::None;
    Swatch hatColor = Swatch::Red;
    Swatch bottomColor = Swatch::Navy;
    Swatch shoeColor = Swatch::Brown;
    Swatch capeColor = Swatch::Red;
    Swatch accent = Swatch::Gold;
    std::uint8_t eyes = 0; // 0 blue, 1 green, 2 amber, 3 violet, 4 red, 5 teal, 6 brown, 7 pink
    std::uint8_t skin = 0; // 0..3, light to dark
    Expression expression = Expression::Neutral;
    Expression nextExpression = Expression::Neutral; // blended in by `blend`
    float blend = 0.0f;
    std::uint8_t seed = 0; // staggers blinks and breathing
    bool skeleton = false; // debug: draw the rig's bones and anchors on top
};

[[nodiscard]] inline std::uint64_t packAvatarBits(const Avatar& a)
{
    const auto u = [](auto v) { return static_cast<std::uint64_t>(v); };
    std::uint64_t lo = u(a.hair) | u(a.hat) << 3 | u(a.top) << 6 | u(a.bottom) << 9 | u(a.shoes) << 12
                       | u(a.gloves) << 14 | u(a.weapon) << 16 | u(a.cape) << 19 | u(a.eyes & 7u) << 21
                       | u(a.skin & 3u) << 24 | u(a.expression) << 26 | u(a.skeleton) << 30;
    std::uint64_t hi = u(a.nextExpression) | u(a.hatColor) << 4 | u(a.bottomColor) << 8 | u(a.shoeColor) << 12
                       | u(a.capeColor) << 16 | u(a.accent) << 20 | u(a.seed) << 24;
    return lo | hi << 32;
}

// The quad is square, its bottom 8/128 below the feet (a margin for outlines).
[[nodiscard]] inline gamelib::draw::Sprite avatarSprite(const Avatar& a, const gamelib::draw::ShapeId shape)
{
    const float size = a.height;
    return {.rect = {a.feet.x - size * 0.5f, a.feet.y - size * (120.0f / 128.0f), size, size},
            .color = a.hairColor,
            .color2 = a.topColor,
            .shape = shape,
            .params = {a.facing < 0.0f ? -1.0f : 1.0f, static_cast<float>(a.pose), a.phase, std::clamp(a.blend, 0.0f, 1.0f)},
            .bits = packAvatarBits(a)};
}

// ---------------------------------------------------------------------------
// monster.frag and boss.frag

enum class MonsterKind : std::uint8_t { Bellcap, Mochi, Turbanshell, Pudgy, Stumpling, Wisp };
// The shared states of gamelib/anime_monster.glsl (kAnimeIdle...).
enum class MonsterState : std::uint8_t { Idle, Move, Windup, Hit, Defeat };

struct Monster {
    gamelib::core::Vec2 feet;
    float height = 96.0f; // the quad height; elites are drawn 1.3x larger
    float facing = 1.0f;
    MonsterKind kind = MonsterKind::Bellcap;
    MonsterState state = MonsterState::Idle;
    float progress = 0.0f; // 0..1 through the state (one hop or step while moving)
    gamelib::core::Color body = gamelib::core::srgb(0xF6E8D2);
    gamelib::core::Color accent = gamelib::core::srgb(0x5CC8B0);
    bool elite = false;
    std::uint8_t seed = 0;
};

// The quad is square with the feet 14/128 above its bottom.
[[nodiscard]] inline gamelib::draw::Sprite monsterSprite(const Monster& m, const gamelib::draw::ShapeId shape)
{
    const float size = m.height * (m.elite ? 1.3f : 1.0f);
    return {.rect = {m.feet.x - size * 0.5f, m.feet.y - size * (114.0f / 128.0f), size, size},
            .color = m.body,
            .color2 = m.accent,
            .shape = shape,
            .params = {static_cast<float>(m.kind), static_cast<float>(m.state), m.progress, m.facing < 0.0f ? -1.0f : 1.0f},
            .bits = (m.elite ? 1u : 0u) | static_cast<std::uint64_t>(m.seed) << 8};
}

// The boss (the Elder Bellcap): a giant fungus with arms, a crown and phase colors.
struct Boss {
    gamelib::core::Vec2 feet;
    float height = 360.0f; // the quad height
    float facing = -1.0f;
    MonsterState state = MonsterState::Idle;
    float progress = 0.0f;
    int phase = 0; // 0 calm colors, 1 angry (glowing eyes, darker cap), 2 enraged (aura)
    gamelib::core::Color body = gamelib::core::srgb(0xF3DFC2);
    gamelib::core::Color cap = gamelib::core::srgb(0xA06CE4);
    int kind = 0;        // boss.frag kinds (game::BossId order)
    bool guarded = false; // in its shell
};

[[nodiscard]] inline gamelib::draw::Sprite bossSprite(const Boss& b, const gamelib::draw::ShapeId shape)
{
    const float size = b.height;
    return {.rect = {b.feet.x - size * 0.5f, b.feet.y - size * (114.0f / 128.0f), size, size},
            .color = b.body,
            .color2 = b.cap,
            .shape = shape,
            .params = {static_cast<float>(b.phase), static_cast<float>(b.state), b.progress, b.facing < 0.0f ? -1.0f : 1.0f},
            .bits = static_cast<std::uint64_t>(b.kind & 7) | (b.guarded ? 8u : 0u)};
}

// ---------------------------------------------------------------------------
// building.frag and prop.frag

enum class BuildingStyle : std::uint8_t { Inn, Shop, Cottage, TreeHut };
// Sign icons (kAnimeIcon* in gamelib/anime_building.glsl).
enum class SignIcon : std::uint8_t { Mug, Potion, Sword, Bread, Star, Leaf };

struct Building {
    gamelib::core::Vec2 ground;     // the middle of its base on the ground
    float height = 320.0f; // the (square) quad's size; the ground is 6/128 above its bottom
    BuildingStyle style = BuildingStyle::Inn;
    gamelib::core::Color wall = gamelib::core::srgb(0xF7EBD3);
    gamelib::core::Color roof = gamelib::core::srgb(0xEC5662);
    float night = 0.0f; // 0..1: windows and lanterns lit
    float wind = 0.3f;  // -1..1: chimney smoke drift
    SignIcon icon = SignIcon::Mug;
    std::uint8_t seed = 0;
};

[[nodiscard]] inline gamelib::draw::Sprite buildingSprite(const Building& b, const gamelib::draw::ShapeId shape)
{
    const float size = b.height;
    return {.rect = {b.ground.x - size * 0.5f, b.ground.y - size * (122.0f / 128.0f), size, size},
            .color = b.wall,
            .color2 = b.roof,
            .shape = shape,
            .params = {static_cast<float>(b.style), std::clamp(b.night, 0.0f, 1.0f), b.wind, 0.0f},
            .bits = static_cast<std::uint64_t>(b.icon) | static_cast<std::uint64_t>(b.seed) << 8};
}

enum class PropKind : std::uint8_t {
    Barrel, Crate, Fence, FlowerPot, StreetLamp, Signpost, Rope, Ladder, RopeBridge, GrassFoothold, WoodFoothold,
    StoneFoothold
};

// A prop fills its rect: standing props have their ground 6/128 above the
// rect's bottom; ropes and ladders run its full height, bridges and
// footholds its full width.
struct Prop {
    gamelib::core::Rect rect;
    PropKind kind = PropKind::Barrel;
    gamelib::core::Color color = gamelib::core::srgb(0xF7F1E3);
    float lit = 0.0f;
    float seed = 0.0f;
};

[[nodiscard]] inline gamelib::draw::Sprite propSprite(const Prop& p, const gamelib::draw::ShapeId shape)
{
    return {.rect = p.rect,
            .color = p.color,
            .shape = shape,
            .params = {static_cast<float>(p.kind), std::clamp(p.lit, 0.0f, 1.0f), p.seed, 0.0f}};
}

} // namespace wayfarer::app::anime
