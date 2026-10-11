#include "look.hpp"

#include <array>

namespace wayfarer::app {

namespace {

// helpers

using game::Region;
using gamelib::core::lerp;

constexpr std::array<std::uint32_t, 16> kPalette{0xF7F1E3, 0x4A3F66, 0xEC5662, 0xF89A48, 0xFAD65E, 0x86D065, 0x3FA067, 0x72C8F2,
                                                 0x4E80E6, 0x3D4C92, 0xA06CE4, 0xF790B6, 0x9C6438, 0xDDB584, 0x8E9DBC, 0xF4C24E};

RegionLook day(const Region r)
{
    RegionLook l;
    switch (r) {
    case Region::Town:
        l = {srgb(0x8FD3F7), srgb(0xE6F6FF), srgb(0xFFFFFF), srgb(0xD6E6F6), srgb(0x9FD8A0), srgb(0xD6F2C6), srgb(0x6FBF77),
             srgb(0xB4E6A0), srgb(0xFFF2C8), srgb(0xE6D6BC), srgb(0xB57E52), srgb(0x8FD46A), srgb(0xB57E52), 5, srgb(0xFFFFFF), {}, 0.3f};
        break;
    case Region::Forest:
        l = {srgb(0x84CFF0), srgb(0xE8FAE6), srgb(0xFFFFFF), srgb(0xD2E8EE), srgb(0x88C98A), srgb(0xC8EEB4), srgb(0x5AA86A),
             srgb(0x9ADA8A), srgb(0xFFF6C8), srgb(0x7CCB58), srgb(0xA86E44), srgb(0x86D065), srgb(0xA86E44), 0, srgb(0xFFFFFF), {}, 0.3f};
        break;
    case Region::Mines:
        l = {srgb(0x2A2340), srgb(0x4A3A5E), srgb(0x6A5A88), srgb(0x4A3E66), srgb(0x5A4A78), srgb(0x8A7AB0), srgb(0x3E3458),
             srgb(0x6A5C8C), srgb(0xFFB070), srgb(0x8E87A8), srgb(0x6E6488), srgb(0xC8955A), srgb(0x7A6E92), 1, srgb(0x9A8EB8), {}, 0.6f};
        break;
    case Region::Marsh:
        l = {srgb(0x9EC9C2), srgb(0xE6F2D8), srgb(0xF6FFF6), srgb(0xC8DCD2), srgb(0x7FA88E), srgb(0xB8D8B0), srgb(0x55806A),
             srgb(0x8AB890), srgb(0xE6F6EC), srgb(0x6FBF8A), srgb(0x6E5A48), srgb(0x7FCC8E), srgb(0x6E5A48), 2, srgb(0xF2FFF6), {}, 0.35f};
        break;
    case Region::Ruins:
        l = {srgb(0xF2B88A), srgb(0xFFEEC8), srgb(0xFFF6E6), srgb(0xF2D2B8), srgb(0xD9A878), srgb(0xFFE2B0), srgb(0xB88458),
             srgb(0xE8C08A), srgb(0xFFE0A0), srgb(0xE8C77A), srgb(0xD8A870), srgb(0xE8D29A), srgb(0xC89A68), 3, srgb(0xFFFFFF), {}, 0.4f};
        break;
    case Region::Peak:
        l = {srgb(0x8CB8F0), srgb(0xEAF4FF), srgb(0xFFFFFF), srgb(0xD0E0F4), srgb(0x9AB0D8), srgb(0xF4F8FF), srgb(0x7890C0),
             srgb(0xE8F0FF), srgb(0xF0F6FF), srgb(0xF2F7FF), srgb(0x7E8CB0), srgb(0xF4F8FF), srgb(0x8A96B8), 4, srgb(0xFFFFFF), {}, 0.4f};
        break;
    case Region::Count: break;
    }
    l.grade.saturation = 1.08f;
    return l;
}

RegionLook nightOf(const RegionLook& d, const Region r)
{
    RegionLook n = d;
    n.skyTop = srgb(0x1B2350);
    n.skyHorizon = srgb(0x5A5490);
    n.cloud = srgb(0x6E70A8);
    n.cloudShade = srgb(0x4A4C80);
    n.far = lerp(d.far, srgb(0x3A4078), 0.7f);
    n.farRim = lerp(d.farRim, srgb(0x8A90C8), 0.6f);
    n.near = lerp(d.near, srgb(0x2E3468), 0.7f);
    n.nearRim = lerp(d.nearRim, srgb(0x6A70B0), 0.6f);
    n.light = srgb(0xA8B0FF);
    n.ambient = r == Region::Mines ? d.ambient : srgb(0x6A70B8);
    n.grade.tint = {0.92f, 0.95f, 1.05f, 1.0f};
    n.bloom = d.bloom + 0.25f;
    return n;
}

} // namespace

RegionLook regionLook(const Region region, const bool dungeon, const float night)
{
    const RegionLook d = day(region);
    // The mines have no sky to darken; dungeons stay dusky whatever the hour.
    const float t = region == Region::Mines ? 0.0f : std::clamp(dungeon ? std::max(night, 0.55f) : night, 0.0f, 1.0f);
    const RegionLook n = nightOf(d, region);
    RegionLook l = d;
    l.skyTop = lerp(d.skyTop, n.skyTop, t);
    l.skyHorizon = lerp(d.skyHorizon, n.skyHorizon, t);
    l.cloud = lerp(d.cloud, n.cloud, t);
    l.cloudShade = lerp(d.cloudShade, n.cloudShade, t);
    l.far = lerp(d.far, n.far, t);
    l.farRim = lerp(d.farRim, n.farRim, t);
    l.near = lerp(d.near, n.near, t);
    l.nearRim = lerp(d.nearRim, n.nearRim, t);
    l.light = lerp(d.light, n.light, t);
    l.ambient = lerp(d.ambient, n.ambient, t);
    l.grade.tint = lerp(d.grade.tint, n.grade.tint, t);
    l.bloom = d.bloom + (n.bloom - d.bloom) * t;
    if (dungeon && region == Region::Mines) {
        l.ambient = srgb(0x7A6E98);
    }
    // Dusk: a warm horizon on the way to night.
    const float dusk = std::clamp(1.0f - std::abs(t - 0.4f) / 0.4f, 0.0f, 1.0f);
    if (region != Region::Mines) {
        l.skyHorizon = lerp(l.skyHorizon, srgb(0xFFB48A), dusk * 0.7f);
        l.cloud = lerp(l.cloud, srgb(0xFFC8B0), dusk * 0.5f);
    }
    return l;
}

int backdropRegion(const Region region)
{
    return static_cast<int>(region);
}

Color paletteColor(const int index)
{
    return srgb(kPalette[static_cast<std::size_t>(index & 15)]);
}

Color regionAccent(const Region region)
{
    switch (region) {
    case Region::Town: return srgb(0xF2B33D);
    case Region::Forest: return srgb(0x6CC86A);
    case Region::Mines: return srgb(0x7FE0FF);
    case Region::Marsh: return srgb(0x7AD8B0);
    case Region::Ruins: return srgb(0xFFB45C);
    case Region::Peak: return srgb(0xB8E2FF);
    case Region::Count: break;
    }
    return srgb(0xFFFFFF);
}

anime::Avatar heroAvatar(const game::Character& c)
{
    using namespace anime;
    Avatar a;
    a.hairColor = srgb(0x8A4B32);
    a.hair = Hair::Short;
    a.eyes = 0;
    a.skin = 0;
    a.accent = Swatch::Gold;
    a.bottom = Bottom::Pants;
    a.bottomColor = Swatch::Navy;
    const auto& weapon = c.gear.at(static_cast<int>(game::Slot::Weapon));
    const auto& helm = c.gear.at(static_cast<int>(game::Slot::Helmet));
    const auto& armor = c.gear.at(static_cast<int>(game::Slot::Armor));
    const auto& shoes = c.gear.at(static_cast<int>(game::Slot::Shoes));
    a.weapon = weapon.empty() ? Weapon::None : static_cast<Weapon>(game::itemInfo(weapon.item).style);
    if (!helm.empty()) {
        const game::ItemInfo& info = game::itemInfo(helm.item);
        a.hat = static_cast<Hat>(info.style);
        a.hatColor = static_cast<Swatch>(info.color);
    }
    if (!armor.empty()) {
        const game::ItemInfo& info = game::itemInfo(armor.item);
        a.top = static_cast<Top>(info.style);
        a.topColor = paletteColor(info.color);
        if (info.tier >= 4) {
            a.cape = Cape::Cape;
            a.capeColor = info.tier == 5 ? Swatch::Navy : Swatch::Red;
        } else if (info.tier == 2) {
            a.cape = Cape::Scarf;
            a.capeColor = Swatch::Orange;
        }
    } else {
        a.top = Top::Tunic;
        a.topColor = srgb(0xF4EEE0);
    }
    if (!shoes.empty()) {
        const game::ItemInfo& info = game::itemInfo(shoes.item);
        a.shoes = static_cast<Shoes>(info.style);
        a.shoeColor = static_cast<Swatch>(info.color);
    }
    a.gloves = armor.empty() || game::itemInfo(armor.item).tier < 3 ? Gloves::None
               : game::itemInfo(armor.item).tier >= 4               ? Gloves::Gauntlets
                                                                    : Gloves::Cloth;
    a.seed = 3;
    return a;
}

anime::Avatar npcAvatar(const game::NpcId npc)
{
    using namespace anime;
    Avatar a;
    a.seed = static_cast<std::uint8_t>(40 + static_cast<int>(npc) * 37);
    switch (npc) {
    case game::NpcId::Elder:
        a.hairColor = srgb(0xE4E2EE);
        a.hair = Hair::Long;
        a.top = Top::Robe;
        a.topColor = srgb(0x5BAA74);
        a.accent = Swatch::Gold;
        a.weapon = Weapon::Staff;
        a.eyes = 6;
        a.skin = 1;
        a.expression = Expression::Happy;
        a.nextExpression = Expression::Happy;
        break;
    case game::NpcId::Pippa:
        a.hairColor = srgb(0xF58FB5);
        a.hair = Hair::TwinTails;
        a.hat = Hat::Ribbon;
        a.hatColor = Swatch::Yellow;
        a.top = Top::Vest;
        a.topColor = srgb(0xFF9EC0);
        a.bottom = Bottom::Skirt;
        a.bottomColor = Swatch::Pink;
        a.shoes = Shoes::Shoes;
        a.shoeColor = Swatch::Red;
        a.accent = Swatch::Yellow;
        a.eyes = 7;
        a.skin = 0;
        break;
    case game::NpcId::Brann:
        a.hairColor = srgb(0x4A3328);
        a.hair = Hair::Short;
        a.hat = Hat::Headband;
        a.hatColor = Swatch::Red;
        a.top = Top::Vest;
        a.topColor = srgb(0x8A5A3C);
        a.bottomColor = Swatch::Slate;
        a.gloves = Gloves::Cloth;
        a.accent = Swatch::Brown;
        a.weapon = Weapon::Axe;
        a.eyes = 2;
        a.skin = 2;
        break;
    case game::NpcId::Lumi:
        a.hairColor = srgb(0xB9A3F0);
        a.hair = Hair::Bob;
        a.hat = Hat::Wizard;
        a.hatColor = Swatch::Navy;
        a.top = Top::Robe;
        a.topColor = srgb(0x7C6CD8);
        a.accent = Swatch::Sky;
        a.weapon = Weapon::Staff;
        a.eyes = 3;
        a.skin = 0;
        break;
    case game::NpcId::Dorran:
        a.hairColor = srgb(0x2E3A5C);
        a.hair = Hair::Short;
        a.hat = Hat::Helmet;
        a.hatColor = Swatch::Slate;
        a.top = Top::Armor;
        a.topColor = srgb(0x9AAACC);
        a.bottomColor = Swatch::Navy;
        a.shoes = Shoes::Greaves;
        a.shoeColor = Swatch::Slate;
        a.gloves = Gloves::Gauntlets;
        a.cape = Cape::Cape;
        a.capeColor = Swatch::Red;
        a.accent = Swatch::Red;
        a.weapon = Weapon::Sword;
        a.eyes = 2;
        a.skin = 2;
        break;
    case game::NpcId::Nell:
        a.hairColor = srgb(0xF2964A);
        a.hair = Hair::Ponytail;
        a.top = Top::Tunic;
        a.topColor = srgb(0xFAD65E);
        a.bottom = Bottom::Shorts;
        a.bottomColor = Swatch::Brown;
        a.shoes = Shoes::Shoes;
        a.shoeColor = Swatch::Leaf;
        a.accent = Swatch::Red;
        a.eyes = 1;
        a.skin = 0;
        a.expression = Expression::Happy;
        a.nextExpression = Expression::Happy;
        break;
    case game::NpcId::Count: break;
    }
    return a;
}

MonsterColors monsterColors(const game::SpeciesId s, const bool elite)
{
    using S = game::SpeciesId;
    MonsterColors c{srgb(0xF6E8D2), srgb(0x5CC8B0)};
    switch (s) {
    case S::Bellcap: c = {srgb(0xF6E8D2), srgb(0x5CC8B0)}; break;
    case S::PinkMochi: c = {srgb(0xF7A8C8), srgb(0xFFE0EC)}; break;
    case S::Pudgy: c = {srgb(0xF6E2C8), srgb(0xC98A52)}; break;
    case S::Stumpling: c = {srgb(0xA86E44), srgb(0x86D065)}; break;
    case S::Pebblet: c = {srgb(0xA49CC0), srgb(0x7FE6FF)}; break;
    case S::Flitter: c = {srgb(0xB79CE6), srgb(0x6E58A8)}; break;
    case S::EmberMochi: c = {srgb(0xFF9E62), srgb(0xFFD27A)}; break;
    case S::LanternCap: c = {srgb(0xF6E8D2), srgb(0xF4C24E)}; break;
    case S::Turbanshell: c = {srgb(0xDDEA9C), srgb(0x5CB8A0)}; break;
    case S::BogMochi: c = {srgb(0x8CD06E), srgb(0xD8F4B8)}; break;
    case S::Murkcap: c = {srgb(0xEAE2F2), srgb(0x9A6AD8)}; break;
    case S::ReedFlitter: c = {srgb(0x7CC8A8), srgb(0x3E8E7A)}; break;
    case S::Wisp: c = {srgb(0xFFF2D2), srgb(0xF4C24E)}; break;
    case S::RunePebblet: c = {srgb(0xDEC08E), srgb(0xFF9A48)}; break;
    case S::OldStump: c = {srgb(0x8E7A68), srgb(0xBACB7A)}; break;
    case S::SandTurban: c = {srgb(0xF2DCAE), srgb(0xD88A5A)}; break;
    case S::SnowPudgy: c = {srgb(0xF6FAFF), srgb(0x9CC8F0)}; break;
    case S::FrostWisp: c = {srgb(0xDDF2FF), srgb(0x7AC8FF)}; break;
    case S::IcePebblet: c = {srgb(0xBADCF4), srgb(0xA8F0FF)}; break;
    case S::FrostMochi: c = {srgb(0xC8E6FF), srgb(0xF4FAFF)}; break;
    case S::Count: break;
    }
    if (elite) {
        // Elites: the same creature in a richer, deeper coat.
        c.accent = lerp(c.accent, srgb(0xB05AE0), 0.45f);
        c.body = lerp(c.body, srgb(0xF4C24E), 0.25f);
    }
    return c;
}

int monsterShaderKind(const game::Archetype a)
{
    return static_cast<int>(a); // the shader's kinds follow the archetype order
}

Color rarityColor(const game::Rarity r)
{
    switch (r) {
    case game::Rarity::Common: return srgb(0xE8E2D2);
    case game::Rarity::Rare: return srgb(0x5CA8FF);
    case game::Rarity::Epic: return srgb(0xB57CFF);
    case game::Rarity::Legendary: return srgb(0xFFC23A);
    case game::Rarity::Count: break;
    }
    return srgb(0xFFFFFF);
}

IconLook itemIcon(const int item)
{
    const game::ItemInfo& info = game::itemInfo(item);
    const gamelib::core::ItemDef* def = game::itemRegistry().find(item);
    IconLook l{.a = srgb(0xFFFFFF), .b = srgb(0xFFFFFF)};
    switch (info.kind) {
    case game::ItemKind::Use:
        if (item == game::kReturnScroll) {
            l = {1.0f, 0.0f, srgb(0x5BB86A), srgb(0xFFFFFF)};
        } else {
            const std::uint32_t hex = item == game::kRedPotion      ? 0xEC5662
                                      : item == game::kOrangePotion ? 0xF89A48
                                      : item == game::kWhitePotion  ? 0xF4F0FF
                                      : item == game::kBluePotion   ? 0x5C9CEE
                                                                    : 0x9A6CF0;
            l = {0.0f, 0.0f, srgb(hex), srgb(0xFFFFFF)};
        }
        break;
    case game::ItemKind::Material: {
        using M = game::Material;
        const auto m = static_cast<M>(info.style);
        std::array<std::uint32_t, 2> col{0xFFFFFF, 0xFFFFFF};
        switch (m) {
        case M::SoftCap: col = {0x5CC8B0, 0xFFF3D6}; break;
        case M::JellyDrop: col = {0xF7A8C8, 0xFFFFFF}; break;
        case M::Acorn: col = {0xC8823F, 0x8A5A3C}; break;
        case M::Twig: col = {0x9A6440, 0x86D065}; break;
        case M::GlimmerOre: col = {0x9C98B4, 0x7FE6FF}; break;
        case M::FlitterFluff: col = {0xC8B4F0, 0xFFFFFF}; break;
        case M::EmberJelly: col = {0xFF8A4A, 0xFFFFFF}; break;
        case M::SwirlShell: col = {0x5CB8A0, 0xFFFFFF}; break;
        case M::BogJelly: col = {0x86C86A, 0xFFFFFF}; break;
        case M::MarshCap: col = {0x9A6AD8, 0xEAE2F2}; break;
        case M::WispLight: col = {0xFFD27A, 0xFFFFFF}; break;
        case M::RunePebble: col = {0xDEC08E, 0xFF9A48}; break;
        case M::OldBark: col = {0x8E7A68, 0xBACB7A}; break;
        case M::FrostFur: col = {0xF4F8FF, 0xFFFFFF}; break;
        case M::SnowFeather: col = {0xDCE8FF, 0xFFFFFF}; break;
        case M::IceShard: col = {0xA8E8FF, 0xFFFFFF}; break;
        case M::Count: break;
        }
        l = {2.0f, static_cast<float>(info.style), srgb(col[0]), srgb(col[1])};
        break;
    }
    case game::ItemKind::Quest: l = {info.style == 0 ? 8.0f : 9.0f, 0.0f, srgb(0xFFD84E), srgb(0xFFFFFF)}; break;
    case game::ItemKind::Equip: {
        const int slot = def != nullptr ? def->equipSlot : 0;
        const Color tierBlade = info.tier == 0 ? srgb(0xC8955A)
                                : info.tier == 4 ? srgb(0xFFE08A)
                                : info.tier == 5 ? srgb(0xBDEBFF)
                                                 : srgb(0xDCE6F5);
        switch (static_cast<game::Slot>(slot)) {
        case game::Slot::Weapon: l = {3.0f, static_cast<float>(info.style), tierBlade, paletteColor(info.color)}; break;
        case game::Slot::Helmet: l = {4.0f, static_cast<float>(info.style), paletteColor(info.color), srgb(0xF4C24E)}; break;
        case game::Slot::Armor: l = {5.0f, static_cast<float>(info.style), paletteColor(info.color), srgb(0xF4C24E)}; break;
        case game::Slot::Shoes: l = {6.0f, static_cast<float>(info.style), paletteColor(info.color), srgb(0xFFFFFF)}; break;
        case game::Slot::Ring: l = {7.0f, 0.0f, paletteColor(info.color), srgb(0xFFFFFF)}; break;
        case game::Slot::Count: break;
        }
        break;
    }
    }
    return l;
}

} // namespace wayfarer::app

namespace wayfarer::app {
MonsterScale monsterScale(const game::Archetype archetype, const float boxHeight, const bool elite)
{
    // Design units from the feet to the top of each creature (feet at +50 of 128).
    float design = 84.0f;
    switch (archetype) {
    case game::Archetype::Bellcap: design = 84.0f; break;
    case game::Archetype::Mochi: design = 57.0f; break;
    case game::Archetype::Turbanshell: design = 61.0f; break;
    case game::Archetype::Pudgy: design = 54.0f; break;
    case game::Archetype::Stumpling: design = 68.0f; break;
    case game::Archetype::Wisp: design = 66.0f; break;
    case game::Archetype::Pebblet: design = 64.0f; break;
    case game::Archetype::Flitter: design = 70.0f; break;
    case game::Archetype::Count: break;
    }
    // The drawing stands a third taller than the box (its hitbox is forgiving).
    const float drawn = boxHeight * 1.3f;
    const float quad = drawn * 128.0f / design / (elite ? 1.3f : 1.0f);
    return {.quad = quad, .top = drawn};
}
} // namespace wayfarer::app

namespace wayfarer::app {
MonsterColors bossColors(const game::BossId boss)
{
    switch (boss) {
    case game::BossId::Grandcap: return {srgb(0xF3DFC2), srgb(0x5CC8B0)};
    case game::BossId::Rumblerock: return {srgb(0xA49CC0), srgb(0x7FE6FF)};
    case game::BossId::Murkshell: return {srgb(0xDDEA9C), srgb(0x8E6AD8)};
    case game::BossId::Warden: return {srgb(0xFFF2D2), srgb(0xFFB040)};
    case game::BossId::Hush: return {srgb(0xF4F0FF), srgb(0x3A3478)};
    case game::BossId::Count: break;
    }
    return {srgb(0xFFFFFF), srgb(0xFFFFFF)};
}
} // namespace wayfarer::app
