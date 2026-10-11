#include "items.hpp"

#include <gamelib/core/weighted.hpp>

#include <algorithm>
#include <array>
#include <map>
#include <string>
#include <vector>

namespace wayfarer::game {

namespace {

// helpers

using gamelib::core::ItemDef;
using gamelib::core::ItemStack;
using gamelib::core::ItemStat;

constexpr int stat(const Bonus b) { return static_cast<int>(b); }

// Avatar styles (the shader's slot values).
constexpr int kWeaponSword = 1, kWeaponStaff = 2, kWeaponDagger = 4, kWeaponAxe = 5;
constexpr int kHatHeadband = 1, kHatWizard = 2, kHatBeanie = 3, kHatHelmet = 4, kHatRibbon = 5;
constexpr int kTopTunic = 0, kTopRobe = 1, kTopVest = 2, kTopArmor = 3;
constexpr int kShoesBoots = 0, kShoesShoes = 1, kShoesGreaves = 2;

constexpr std::array<int, 6> kLevelOfTier{1, 1, 7, 13, 19, 25};
constexpr std::array<int, 6> kAttackOfTier{5, 12, 24, 38, 54, 72};
constexpr std::array<int, 6> kHelmDefense{1, 4, 8, 13, 19, 26};
constexpr std::array<int, 6> kArmorDefense{2, 7, 14, 22, 31, 42};
constexpr std::array<int, 6> kShoeDefense{1, 3, 6, 9, 12, 16};
constexpr std::array<int, 6> kShoeSpeed{0, 3, 4, 5, 6, 8};
constexpr std::array<int, 6> kShoeJump{0, 1, 2, 2, 3, 4};

struct Catalogue {
    gamelib::core::ItemRegistry registry;
    std::map<int, ItemInfo> info;
    std::array<std::vector<int>, 6> byTier;
};

void add(Catalogue& c, ItemDef def, ItemInfo info)
{
    info.id = def.id;
    c.info[def.id] = info;
    if (info.kind == ItemKind::Equip) {
        c.byTier[static_cast<std::size_t>(info.tier)].push_back(def.id);
    }
    c.registry.add(def);
}

void addWeapon(Catalogue& c, const int tier, const int variant, std::string name, const WeaponKind kind, const int color,
               std::string_view text)
{
    const int base = kAttackOfTier[static_cast<std::size_t>(tier)];
    std::vector<ItemStat> stats;
    int style = kWeaponSword;
    switch (kind) {
    case WeaponKind::Sword: stats = {{stat(Bonus::Attack), base}}; break;
    case WeaponKind::Dagger:
        stats = {{stat(Bonus::Attack), base * 85 / 100}, {stat(Bonus::Crit), 4 + tier}, {stat(Bonus::Agi), tier}};
        style = kWeaponDagger;
        break;
    case WeaponKind::Axe:
        stats = {{stat(Bonus::Attack), base * 118 / 100}, {stat(Bonus::Str), tier * 2}};
        style = kWeaponAxe;
        break;
    case WeaponKind::Staff:
        stats = {{stat(Bonus::Attack), base * 60 / 100}, {stat(Bonus::Magic), base}, {stat(Bonus::Mp), 10 * tier}};
        style = kWeaponStaff;
        break;
    }
    const int price = 40 + tier * tier * 260;
    add(c,
        {.id = equipItem(tier, Slot::Weapon, variant), .name = std::move(name), .kind = static_cast<int>(ItemKind::Equip),
         .price = price, .sellPrice = price / 4, .equipSlot = static_cast<int>(Slot::Weapon),
         .levelRequirement = kLevelOfTier[static_cast<std::size_t>(tier)], .stats = std::move(stats)},
        {.kind = ItemKind::Equip, .tier = tier, .style = style, .color = color, .text = text});
}

void addGear(Catalogue& c, const int tier, const Slot slot, const int variant, std::string name, const int style,
             const int color, std::vector<ItemStat> stats, std::string_view text)
{
    const int price = 30 + tier * tier * 180;
    add(c,
        {.id = equipItem(tier, slot, variant), .name = std::move(name), .kind = static_cast<int>(ItemKind::Equip),
         .price = price, .sellPrice = price / 4, .equipSlot = static_cast<int>(slot),
         .levelRequirement = kLevelOfTier[static_cast<std::size_t>(tier)], .stats = std::move(stats)},
        {.kind = ItemKind::Equip, .tier = tier, .style = style, .color = color, .text = text});
}

void addTier(Catalogue& c, const int t)
{
    const auto ts = static_cast<std::size_t>(t);
    const int helm = kHelmDefense[ts];
    const int armor = kArmorDefense[ts];
    const auto shoes = [&](const int style, const int color, std::string name, std::string_view text) {
        addGear(c, t, Slot::Shoes, 0, std::move(name), style, color,
                {{stat(Bonus::Defense), kShoeDefense[ts]}, {stat(Bonus::Speed), kShoeSpeed[ts]}, {stat(Bonus::Jump), kShoeJump[ts]}},
                text);
    };
    const auto rings = [&](std::string might, std::string wonder) {
        addGear(c, t, Slot::Ring, 0, std::move(might), 0, 2,
                {{stat(Bonus::Str), 2 * t}, {stat(Bonus::Attack), t * 2}}, "a warm band that steadies the sword arm");
        addGear(c, t, Slot::Ring, 1, std::move(wonder), 0, 8,
                {{stat(Bonus::Mag), 2 * t}, {stat(Bonus::Mp), 12 * t}}, "a cool band that hums with quiet magic");
    };
    switch (t) {
    case 1:
        addWeapon(c, 1, 0, "Bellwood Blade", WeaponKind::Sword, 5, "a light blade with a leaf on the guard");
        addWeapon(c, 1, 1, "Acorn Dirk", WeaponKind::Dagger, 12, "quick, and lucky in the hand");
        addGear(c, 1, Slot::Helmet, 0, "Sprout Band", kHatHeadband, 5, {{stat(Bonus::Defense), helm}, {stat(Bonus::Hp), 8}},
                "a green headband tied with a bow");
        addGear(c, 1, Slot::Helmet, 1, "Petal Ribbon", kHatRibbon, 11, {{stat(Bonus::Defense), helm}, {stat(Bonus::Mp), 10}},
                "a ribbon dyed with forest petals");
        addGear(c, 1, Slot::Armor, 0, "Fern Tunic", kTopTunic, 5, {{stat(Bonus::Defense), armor}, {stat(Bonus::Hp), 15}},
                "soft green cloth that smells of rain");
        shoes(kShoesBoots, 12, "Mossy Boots", "boots that never slip on wet logs");
        rings("Clover Ring", "Dewdrop Ring");
        break;
    case 2:
        addWeapon(c, 2, 0, "Glimmer Saber", WeaponKind::Sword, 4, "the ore in its steel catches any light");
        addWeapon(c, 2, 1, "Ore Splitter", WeaponKind::Axe, 3, "a miner's axe, heavy and honest");
        addGear(c, 2, Slot::Helmet, 0, "Lantern Cap", kHatBeanie, 3, {{stat(Bonus::Defense), helm}, {stat(Bonus::Hp), 20}},
                "a padded cap for low tunnel ceilings");
        addGear(c, 2, Slot::Armor, 0, "Miner's Vest", kTopVest, 3, {{stat(Bonus::Defense), armor}, {stat(Bonus::Hp), 30}},
                "a vest with a pocket for every pebble");
        shoes(kShoesBoots, 14, "Hobnail Boots", "studded soles that grip the stone");
        rings("Ember Ring", "Geode Ring");
        break;
    case 3:
        addWeapon(c, 3, 0, "Reedcutter", WeaponKind::Sword, 6, "a long blade as thin as a reed");
        addWeapon(c, 3, 1, "Willow Staff", WeaponKind::Staff, 10, "a staff that remembers spells");
        addGear(c, 3, Slot::Helmet, 0, "Bog Witch Hat", kHatWizard, 10, {{stat(Bonus::Defense), helm}, {stat(Bonus::Mp), 40}},
                "a tall hat with a bent tip and a mind of its own");
        addGear(c, 3, Slot::Armor, 0, "Mistweave Robe", kTopRobe, 10,
                {{stat(Bonus::Defense), armor}, {stat(Bonus::Mag), 4}, {stat(Bonus::Mp), 30}}, "woven from marsh mist at dawn");
        addGear(c, 3, Slot::Armor, 1, "Reed Jerkin", kTopVest, 6, {{stat(Bonus::Defense), armor + 3}, {stat(Bonus::Hp), 50}},
                "tough reeds stitched over leather");
        shoes(kShoesShoes, 6, "Wader Shoes", "light shoes that dry in a moment");
        rings("Lily Ring", "Moss Ring");
        break;
    case 4:
        addWeapon(c, 4, 0, "Sunsteel Sword", WeaponKind::Sword, 15, "forged from the ruins' golden steel");
        addWeapon(c, 4, 1, "Rune Dagger", WeaponKind::Dagger, 7, "its runes glow before a lucky strike");
        addGear(c, 4, Slot::Helmet, 0, "Sunspire Helm", kHatHelmet, 15, {{stat(Bonus::Defense), helm}, {stat(Bonus::Hp), 60}},
                "a polished helm with a crest of sun-feathers");
        addGear(c, 4, Slot::Armor, 0, "Warden Plate", kTopArmor, 15, {{stat(Bonus::Defense), armor}, {stat(Bonus::Hp), 80}},
                "the plate of the old ruin wardens");
        shoes(kShoesGreaves, 15, "Gilded Greaves", "light gold greaves with winged ankles");
        rings("Sun Ring", "Moon Ring");
        break;
    case 5:
        addWeapon(c, 5, 0, "Frostbrand", WeaponKind::Sword, 7, "a blade of starlit ice that never melts");
        addWeapon(c, 5, 1, "Aurora Staff", WeaponKind::Staff, 11, "the northern lights caught in an orb");
        addGear(c, 5, Slot::Helmet, 0, "Frost Helm", kHatHelmet, 7, {{stat(Bonus::Defense), helm}, {stat(Bonus::Hp), 100}},
                "a helm rimed with frost, warm inside");
        addGear(c, 5, Slot::Armor, 0, "Glacier Mail", kTopArmor, 7, {{stat(Bonus::Defense), armor}, {stat(Bonus::Hp), 130}},
                "mail as blue as a glacier's heart");
        shoes(kShoesGreaves, 7, "Snowstep Greaves", "they leave no tracks in the snow");
        rings("Star Ring", "Comet Ring");
        break;
    default: break;
    }
}

Catalogue build()
{
    Catalogue c;
    const auto use = [&](const int id, std::string name, const int price, const int heal, const int mana, std::string_view text) {
        add(c,
            {.id = id, .name = std::move(name), .kind = static_cast<int>(ItemKind::Use), .stackLimit = 99, .price = price,
             .sellPrice = price / 4},
            {.kind = ItemKind::Use, .heal = heal, .mana = mana, .text = text});
    };
    use(kRedPotion, "Red Potion", 25, 60, 0, "restores 60 hp");
    use(kOrangePotion, "Orange Potion", 80, 180, 0, "restores 180 hp");
    use(kWhitePotion, "White Potion", 220, 450, 0, "restores 450 hp");
    use(kBluePotion, "Blue Potion", 30, 0, 40, "restores 40 mp");
    use(kManaTonic, "Mana Tonic", 120, 0, 150, "restores 150 mp");
    use(kReturnScroll, "Return Scroll", 60, 0, 0, "carries you home to Hearthvale");

    constexpr std::array<std::string_view, kMaterialCount> names{
        "Soft Cap", "Jelly Drop", "Acorn", "Twig", "Glimmer Ore", "Flitter Fluff", "Ember Jelly", "Swirl Shell", "Bog Jelly",
        "Marsh Cap", "Wisp Light", "Rune Pebble", "Old Bark", "Frost Fur", "Snow Feather", "Ice Shard"};
    for (int m = 0; m < kMaterialCount; ++m) {
        const int tier = m < 4 ? 1 : m < 7 ? 2 : m < 10 ? 3 : m < 13 ? 4 : 5;
        const int price = 6 + tier * tier * 6;
        add(c,
            {.id = kFirstMaterial + m, .name = std::string{names[static_cast<std::size_t>(m)]},
             .kind = static_cast<int>(ItemKind::Material), .stackLimit = 99, .price = price * 3, .sellPrice = price},
            {.kind = ItemKind::Material, .tier = tier, .style = m,
             .text = m == static_cast<int>(Material::GlimmerOre) ? "the blacksmith turns it into sharper steel"
                                                                 : "a keepsake from a defeated monster"});
    }
    add(c, {.id = kElderLetter, .name = "Elder's Letter", .kind = static_cast<int>(ItemKind::Quest)},
        {.kind = ItemKind::Quest, .style = 0, .text = "sealed with green wax; for brann at the forge"});
    add(c, {.id = kStarCharm, .name = "Star Charm", .kind = static_cast<int>(ItemKind::Quest)},
        {.kind = ItemKind::Quest, .style = 1, .text = "lumi's lost charm; it twinkles at night"});

    // Starter gear (tier 0).
    add(c,
        {.id = kStarterSword, .name = "Wooden Sword", .kind = static_cast<int>(ItemKind::Equip), .price = 20, .sellPrice = 5,
         .equipSlot = static_cast<int>(Slot::Weapon), .levelRequirement = 1, .stats = {{stat(Bonus::Attack), kAttackOfTier[0]}}},
        {.kind = ItemKind::Equip, .tier = 0, .style = kWeaponSword, .color = 12, .text = "a practice sword, light and true"});
    add(c,
        {.id = kStarterTunic, .name = "Traveler's Tunic", .kind = static_cast<int>(ItemKind::Equip), .price = 20,
         .sellPrice = 5, .equipSlot = static_cast<int>(Slot::Armor), .levelRequirement = 1,
         .stats = {{stat(Bonus::Defense), kArmorDefense[0]}}},
        {.kind = ItemKind::Equip, .tier = 0, .style = kTopTunic, .color = 8, .text = "it has seen many roads"});
    add(c,
        {.id = kStarterShoes, .name = "Worn Shoes", .kind = static_cast<int>(ItemKind::Equip), .price = 15, .sellPrice = 4,
         .equipSlot = static_cast<int>(Slot::Shoes), .levelRequirement = 1, .stats = {{stat(Bonus::Defense), kShoeDefense[0]}}},
        {.kind = ItemKind::Equip, .tier = 0, .style = kShoesShoes, .color = 12, .text = "comfortable, if a little thin"});
    for (int t = 1; t <= 5; ++t) {
        addTier(c, t);
    }
    return c;
}

const Catalogue& catalogue()
{
    static const Catalogue c = build();
    return c;
}

// The bonuses a slot rolls, with weights.
const gamelib::core::WeightedTable<Bonus>& bonusTable(const Slot slot)
{
    static const std::array<gamelib::core::WeightedTable<Bonus>, kSlotCount> tables = [] {
        std::array<gamelib::core::WeightedTable<Bonus>, kSlotCount> t;
        auto& weapon = t[static_cast<std::size_t>(Slot::Weapon)];
        weapon.add(Bonus::Str, 4);
        weapon.add(Bonus::Agi, 3);
        weapon.add(Bonus::Mag, 3);
        weapon.add(Bonus::Attack, 5);
        weapon.add(Bonus::Magic, 3);
        weapon.add(Bonus::Crit, 3);
        auto& helm = t[static_cast<std::size_t>(Slot::Helmet)];
        helm.add(Bonus::Vit, 4);
        helm.add(Bonus::Mag, 3);
        helm.add(Bonus::Hp, 4);
        helm.add(Bonus::Mp, 4);
        helm.add(Bonus::Defense, 3);
        auto& armor = t[static_cast<std::size_t>(Slot::Armor)];
        armor.add(Bonus::Vit, 4);
        armor.add(Bonus::Str, 3);
        armor.add(Bonus::Hp, 5);
        armor.add(Bonus::Defense, 4);
        auto& shoes = t[static_cast<std::size_t>(Slot::Shoes)];
        shoes.add(Bonus::Agi, 5);
        shoes.add(Bonus::Speed, 4);
        shoes.add(Bonus::Jump, 3);
        shoes.add(Bonus::Defense, 2);
        auto& ring = t[static_cast<std::size_t>(Slot::Ring)];
        ring.add(Bonus::Str, 3);
        ring.add(Bonus::Agi, 3);
        ring.add(Bonus::Vit, 3);
        ring.add(Bonus::Mag, 3);
        ring.add(Bonus::Crit, 2);
        ring.add(Bonus::Hp, 2);
        ring.add(Bonus::Mp, 2);
        return t;
    }();
    return tables[static_cast<std::size_t>(slot)];
}

int rollValue(const Bonus bonus, const int tier, gamelib::core::Rng& rng)
{
    const int t = std::max(tier, 1);
    switch (bonus) {
    case Bonus::Str:
    case Bonus::Agi:
    case Bonus::Vit:
    case Bonus::Mag: return rng.range(1, 2) + t * rng.range(1, 2);
    case Bonus::Attack:
    case Bonus::Magic: return rng.range(1, 3) * t + rng.range(0, 2);
    case Bonus::Defense: return rng.range(1, 3) * t;
    case Bonus::Hp: return rng.range(2, 5) * 6 * t;
    case Bonus::Mp: return rng.range(2, 5) * 4 * t;
    case Bonus::Crit: return rng.range(1, 3);
    case Bonus::Speed: return rng.range(2, 4);
    case Bonus::Jump: return rng.range(1, 3);
    case Bonus::Upgrade:
    case Bonus::Count: break;
    }
    return 1;
}

} // namespace

std::string_view regionName(const Region region)
{
    switch (region) {
    case Region::Town: return "Hearthvale";
    case Region::Forest: return "Bellwood Forest";
    case Region::Mines: return "Glimmerdeep Mines";
    case Region::Marsh: return "Murkmallow Marsh";
    case Region::Ruins: return "Sunspire Ruins";
    case Region::Peak: return "Frostcrown Peak";
    case Region::Count: break;
    }
    return "";
}

std::string_view coreName(const Core stat)
{
    switch (stat) {
    case Core::Str: return "Strength";
    case Core::Agi: return "Agility";
    case Core::Vit: return "Vitality";
    case Core::Mag: return "Magic";
    case Core::Count: break;
    }
    return "";
}

std::string_view coreShort(const Core stat)
{
    switch (stat) {
    case Core::Str: return "STR";
    case Core::Agi: return "AGI";
    case Core::Vit: return "VIT";
    case Core::Mag: return "MAG";
    case Core::Count: break;
    }
    return "";
}

std::string_view bonusName(const Bonus bonus)
{
    switch (bonus) {
    case Bonus::Str: return "STR";
    case Bonus::Agi: return "AGI";
    case Bonus::Vit: return "VIT";
    case Bonus::Mag: return "MAG";
    case Bonus::Attack: return "Attack";
    case Bonus::Magic: return "Magic Attack";
    case Bonus::Defense: return "Defense";
    case Bonus::Hp: return "Max HP";
    case Bonus::Mp: return "Max MP";
    case Bonus::Crit: return "Critical %";
    case Bonus::Speed: return "Speed %";
    case Bonus::Jump: return "Jump %";
    case Bonus::Upgrade: return "Upgrade";
    case Bonus::Count: break;
    }
    return "";
}

std::string_view slotName(const Slot slot)
{
    switch (slot) {
    case Slot::Weapon: return "Weapon";
    case Slot::Helmet: return "Hat";
    case Slot::Armor: return "Armor";
    case Slot::Shoes: return "Shoes";
    case Slot::Ring: return "Ring";
    case Slot::Count: break;
    }
    return "";
}

std::string_view rarityName(const Rarity rarity)
{
    switch (rarity) {
    case Rarity::Common: return "Common";
    case Rarity::Rare: return "Rare";
    case Rarity::Epic: return "Epic";
    case Rarity::Legendary: return "Legendary";
    case Rarity::Count: break;
    }
    return "";
}

std::string_view skillName(const SkillId skill)
{
    switch (skill) {
    case SkillId::Combo: return "Blade Dance";
    case SkillId::HyperJump: return "Hyper Jump";
    case SkillId::Roll: return "Dodge Roll";
    case SkillId::Sweep: return "Crescent Sweep";
    case SkillId::Starbolt: return "Starbolt";
    case SkillId::Tailwind: return "Tailwind";
    case SkillId::Bloom: return "Bloom";
    case SkillId::Count: break;
    }
    return "";
}

const gamelib::core::ItemRegistry& itemRegistry()
{
    return catalogue().registry;
}

const ItemInfo& itemInfo(const int id)
{
    static const ItemInfo none{};
    const auto it = catalogue().info.find(id);
    return it == catalogue().info.end() ? none : it->second;
}

std::span<const int> equipmentOfTier(const int tier)
{
    return catalogue().byTier[static_cast<std::size_t>(std::clamp(tier, 0, 5))];
}

int upgradeOf(const ItemStack& stack)
{
    for (const ItemStat& b : stack.bonuses) {
        if (b.stat == stat(Bonus::Upgrade)) {
            return b.value;
        }
    }
    return 0;
}

int stackBonus(const ItemStack& stack, const Bonus bonus)
{
    if (stack.empty()) {
        return 0;
    }
    int sum = 0;
    if (const ItemDef* def = itemRegistry().find(stack.item)) {
        for (const ItemStat& s : def->stats) {
            sum += s.stat == stat(bonus) ? s.value : 0;
        }
        const int up = upgradeOf(stack);
        const int tier = std::max(itemInfo(stack.item).tier, 1);
        if (up > 0 && def->equipSlot == static_cast<int>(Slot::Weapon) && (bonus == Bonus::Attack || bonus == Bonus::Magic)) {
            int base = 0;
            for (const ItemStat& s : def->stats) {
                base += s.stat == stat(bonus) ? s.value : 0;
            }
            sum += base > 0 ? up * (2 + tier * 2) : 0;
        } else if (up > 0 && def->equipSlot != static_cast<int>(Slot::Weapon) && bonus == Bonus::Defense) {
            sum += up * (1 + tier);
        } else if (up > 0 && def->equipSlot == static_cast<int>(Slot::Ring) && bonus == Bonus::Hp) {
            sum += up * 10 * tier;
        }
    }
    for (const ItemStat& b : stack.bonuses) {
        sum += b.stat == stat(bonus) && bonus != Bonus::Upgrade ? b.value : 0;
    }
    return sum;
}

ItemStack rollEquipment(const int item, gamelib::core::Rng& rng, const bool elite)
{
    ItemStack stack{.item = item, .count = 1};
    const ItemDef* def = itemRegistry().find(item);
    if (def == nullptr || def->equipSlot < 0) {
        return stack;
    }
    // Rarity: common 66, rare 24, epic 8, legendary 2 (elites: 30 / 40 / 22 / 8).
    gamelib::core::WeightedTable<int> rarity;
    rarity.add(0, elite ? 30u : 66u);
    rarity.add(1, elite ? 40u : 24u);
    rarity.add(2, elite ? 22u : 8u);
    rarity.add(3, elite ? 8u : 2u);
    stack.rarity = rarity.pick(rng);
    const int tier = itemInfo(item).tier;
    const auto& table = bonusTable(static_cast<Slot>(def->equipSlot));
    for (const Bonus b : table.pickDistinct(rng, static_cast<std::size_t>(stack.rarity))) {
        int value = rollValue(b, tier, rng);
        if (stack.rarity == 3) {
            value += value / 2; // legendary lines roll higher
        }
        stack.bonuses.push_back({stat(b), value});
    }
    return stack;
}

ItemStack rollRandomEquipment(const int tier, gamelib::core::Rng& rng, const bool elite)
{
    const std::span<const int> items = equipmentOfTier(tier);
    if (items.empty()) {
        return {};
    }
    return rollEquipment(items[static_cast<std::size_t>(rng.range(0, static_cast<int>(items.size()) - 1))], rng, elite);
}

UpgradeCost upgradeCost(const ItemStack& stack)
{
    const int up = upgradeOf(stack);
    const int tier = std::max(itemInfo(stack.item).tier, 1);
    return {.coins = static_cast<std::int64_t>((up + 1) * (up + 1) * 60 * tier), .ore = (up + 1) * 2 + tier - 1};
}

void applyUpgrade(ItemStack& stack)
{
    for (ItemStat& b : stack.bonuses) {
        if (b.stat == stat(Bonus::Upgrade)) {
            b.value = std::min(b.value + 1, kMaxUpgrade);
            return;
        }
    }
    stack.bonuses.push_back({stat(Bonus::Upgrade), 1});
}

int sellValue(const ItemStack& stack)
{
    const ItemDef* def = itemRegistry().find(stack.item);
    if (def == nullptr) {
        return 0;
    }
    const int base = def->sellPrice;
    return base * (100 + stack.rarity * 60 + upgradeOf(stack) * 40) / 100;
}

} // namespace wayfarer::game
