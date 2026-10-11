#pragma once

// Wayfarer's shared vocabulary and numbers: the tick rate and tile size, the
// regions, the four core stats and the item bonuses, equipment slots,
// rarities, the skills, and the per-tick input. Every rule runs on whole
// ticks at kTickRate, so the same per-tick input always gives the same run.

#include <gamelib/core/math.hpp>

#include <cstdint>
#include <string_view>

namespace wayfarer::game {

inline constexpr int kTickRate = 60;
inline constexpr float kTile = 48.0f; // world units per tile; the canvas shows 40 x 22.5 tiles

inline constexpr int kMaxLevel = 30;
inline constexpr int kPointsPerLevel = 5;      // ability points (core stats)
inline constexpr int kSkillPointsPerLevel = 3; // skill points
inline constexpr int kBagSlots = 24;
inline constexpr int kStartCore = 5; // every core stat at level 1

// The hero's collision box (the chibi drawing is about 120 units tall).
inline constexpr gamelib::core::Vec2 kHeroSize{34.0f, 92.0f};

enum class Region : std::uint8_t { Town, Forest, Mines, Marsh, Ruins, Peak, Count };
inline constexpr int kRegionCount = static_cast<int>(Region::Count);
[[nodiscard]] std::string_view regionName(Region region);

// The four core stats, raised with ability points.
enum class Core : std::uint8_t { Str, Agi, Vit, Mag, Count };
inline constexpr int kCoreCount = static_cast<int>(Core::Count);
[[nodiscard]] std::string_view coreName(Core stat);
[[nodiscard]] std::string_view coreShort(Core stat);

// What an item adds (ItemStat::stat). Upgrade holds the blacksmith's +N.
enum class Bonus : std::uint8_t { Str, Agi, Vit, Mag, Attack, Magic, Defense, Hp, Mp, Crit, Speed, Jump, Upgrade, Count };
inline constexpr int kBonusCount = static_cast<int>(Bonus::Count);
[[nodiscard]] std::string_view bonusName(Bonus bonus);

enum class Slot : std::uint8_t { Weapon, Helmet, Armor, Shoes, Ring, Count };
inline constexpr int kSlotCount = static_cast<int>(Slot::Count);
[[nodiscard]] std::string_view slotName(Slot slot);

enum class ItemKind : std::uint8_t { Equip, Use, Material, Quest };

enum class Rarity : std::uint8_t { Common, Rare, Epic, Legendary, Count };
inline constexpr int kRarityCount = static_cast<int>(Rarity::Count);
[[nodiscard]] std::string_view rarityName(Rarity rarity);

// The skills in the skill window's order. Combo is the basic attack's
// passive mastery; the others are used from hotkeys (the hyper jump from
// the jump key in the air).
enum class SkillId : std::uint8_t { Combo, HyperJump, Roll, Sweep, Starbolt, Tailwind, Bloom, Count };
inline constexpr int kSkillCount = static_cast<int>(SkillId::Count);
[[nodiscard]] std::string_view skillName(SkillId skill);

// The four skill hotkeys (sweep, starbolt, tailwind, bloom) in order.
inline constexpr int kHotkeyCount = 4;
[[nodiscard]] constexpr SkillId hotkeySkill(const int index)
{
    return static_cast<SkillId>(static_cast<int>(SkillId::Sweep) + index);
}

// One tick of player input, after key mapping (never raw keys).
struct Input {
    int move = 0;  // -1 left, 0, 1 right
    int climb = 0; // -1 up, 0, 1 down (ropes, ladders, drop through, the hyper jump's direction)
    bool jumpPressed = false;
    bool jumpHeld = false;
    bool attackPressed = false;
    bool attackHeld = false;
    bool rollPressed = false;
    bool upPressed = false; // portals, doors, talking
    bool skillPressed[kHotkeyCount] = {};
    bool hpPotion = false;
    bool mpPotion = false;

    friend bool operator==(const Input&, const Input&) = default;
};

} // namespace wayfarer::game
