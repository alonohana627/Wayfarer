#pragma once

// Wayfarer's items: the catalogue (GameLib's core::ItemRegistry, built once),
// what each item is for (potions, the return scroll, materials, quest items
// and equipment by tier), its look on the hero, and the loot rules - the
// rarity roll and the random bonuses rolled from weighted tables, and the
// blacksmith's upgrades.

#include "rules.hpp"

#include <gamelib/core/items.hpp>
#include <gamelib/core/rng.hpp>

#include <span>
#include <string_view>

namespace wayfarer::game {

// Use items.
inline constexpr int kRedPotion = 1;    // +60 HP
inline constexpr int kOrangePotion = 2; // +180 HP
inline constexpr int kWhitePotion = 3;  // +450 HP
inline constexpr int kBluePotion = 4;   // +40 MP
inline constexpr int kManaTonic = 5;    // +150 MP
inline constexpr int kReturnScroll = 6; // back to Hearthvale

// Materials, one per monster species group (index = MaterialId - kFirstMaterial).
inline constexpr int kFirstMaterial = 100;
enum class Material : std::uint8_t {
    SoftCap, JellyDrop, Acorn, Twig,           // forest
    GlimmerOre, FlitterFluff, EmberJelly,      // mines
    SwirlShell, BogJelly, MarshCap,            // marsh
    WispLight, RunePebble, OldBark,            // ruins
    FrostFur, SnowFeather, IceShard,           // peak
    Count
};
inline constexpr int kMaterialCount = static_cast<int>(Material::Count);
[[nodiscard]] constexpr int materialItem(const Material m) { return kFirstMaterial + static_cast<int>(m); }

// Quest items.
inline constexpr int kElderLetter = 200;
inline constexpr int kStarCharm = 201;

// Equipment ids: 1000 + tier * 100 + slot * 10 + variant.
[[nodiscard]] constexpr int equipItem(const int tier, const Slot slot, const int variant)
{
    return 1000 + tier * 100 + static_cast<int>(slot) * 10 + variant;
}
inline constexpr int kStarterSword = 1000;  // tier 0
inline constexpr int kStarterTunic = 1020;
inline constexpr int kStarterShoes = 1030;

// The avatar's weapon kinds (the hero's swing and the drawing follow them).
enum class WeaponKind : std::uint8_t { Sword, Staff, Dagger, Axe };

// What the catalogue knows beyond core::ItemDef.
struct ItemInfo {
    int id = 0;
    ItemKind kind = ItemKind::Material;
    int tier = 0;           // 0 starter, 1..5 by region
    int style = 0;          // the look: weapon kind, hat, top or shoe style
    int color = 0;          // a palette index for the look
    int heal = 0;           // potions: HP restored
    int mana = 0;           // potions: MP restored
    std::string_view text;  // one line for the item panel
};

// The catalogue, built once.
[[nodiscard]] const gamelib::core::ItemRegistry& itemRegistry();
[[nodiscard]] const ItemInfo& itemInfo(int id); // a default ItemInfo for unknown ids
[[nodiscard]] std::span<const int> equipmentOfTier(int tier);

[[nodiscard]] inline Rarity rarityOf(const gamelib::core::ItemStack& stack)
{
    return static_cast<Rarity>(std::clamp(stack.rarity, 0, kRarityCount - 1));
}
[[nodiscard]] int upgradeOf(const gamelib::core::ItemStack& stack);
inline constexpr int kMaxUpgrade = 5;

// A stack's total for one bonus: the item's fixed stats, its rolled
// bonuses and what its upgrades add (attack on weapons, defense elsewhere).
[[nodiscard]] int stackBonus(const gamelib::core::ItemStack& stack, Bonus bonus);

// A new piece of equipment as it drops: the rarity rolled (elites roll
// better) and that many random bonuses rolled from the slot's table.
[[nodiscard]] gamelib::core::ItemStack rollEquipment(int item, gamelib::core::Rng& rng, bool elite);
[[nodiscard]] gamelib::core::ItemStack rollRandomEquipment(int tier, gamelib::core::Rng& rng, bool elite);

// The blacksmith: what the next upgrade costs, and doing it.
struct UpgradeCost {
    std::int64_t coins = 0;
    int ore = 0; // glimmer ore
};
[[nodiscard]] UpgradeCost upgradeCost(const gamelib::core::ItemStack& stack);
void applyUpgrade(gamelib::core::ItemStack& stack);

// The equipment's sell value (rarity and upgrades raise it).
[[nodiscard]] int sellValue(const gamelib::core::ItemStack& stack);

} // namespace wayfarer::game
