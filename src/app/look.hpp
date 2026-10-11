#pragma once

// How Wayfarer looks, as data: the interface palette, each region's sky,
// silhouettes, ground and lighting by day and by night, the hero's avatar
// from the equipment worn, the six townsfolk, the monsters' colors (elites
// deeper and decorated), item icons and rarity colors. Presentation only.

#include "anime_look.hpp"

#include "character.hpp"
#include "items.hpp"
#include "maps.hpp"
#include "monsters.hpp"

#include <gamelib/core/color.hpp>
#include <gamelib/draw/effects.hpp>

namespace wayfarer::app {

using gamelib::core::Color;

[[nodiscard]] inline Color srgb(const std::uint32_t hex, const float alpha = 1.0f)
{
    return gamelib::core::srgb(hex).withAlpha(alpha);
}

// The interface: cream windows with a soft teal title bar and warm ink.
namespace ui_color {
inline const Color kWindow = srgb(0xFFF7E8);
inline const Color kTitle = srgb(0x8CCAD6);
inline const Color kInk = srgb(0x4E3D33);
inline const Color kSoftInk = srgb(0x8A7460);
inline const Color kGold = srgb(0xF2B33D);
inline const Color kSlot = srgb(0xF4E7CF);
inline const Color kHp = srgb(0xF0606E);
inline const Color kMp = srgb(0x5E9CF2);
inline const Color kXp = srgb(0xF6C845);
inline const Color kGood = srgb(0x4CB86A);
inline const Color kBad = srgb(0xE0525E);
inline const Color kWhite = srgb(0xFFFFFF);
inline const Color kButton = srgb(0xFFE7B8);
} // namespace ui_color

struct RegionLook {
    Color skyTop, skyHorizon;
    Color cloud, cloudShade;
    Color far, farRim;
    Color near, nearRim;
    Color light;           // the light layer's tint (rays, mist)
    Color cap, body;       // ground: the top and the earth
    Color footCap, footBody;
    int terrain = 0;       // the terrain shader's style
    Color ambient;         // the light buffer's base
    gamelib::draw::ColorGrade grade;
    float bloom = 0.35f;
};

// A region's look at a time of day (0 day .. 1 night); dungeons are dimmer.
[[nodiscard]] RegionLook regionLook(game::Region region, bool dungeon, float night);
[[nodiscard]] int backdropRegion(game::Region region);

// Avatars.
[[nodiscard]] anime::Avatar heroAvatar(const game::Character& character);
[[nodiscard]] anime::Avatar npcAvatar(game::NpcId npc);

struct MonsterColors {
    Color body;
    Color accent;
};
[[nodiscard]] MonsterColors monsterColors(game::SpeciesId species, bool elite);
[[nodiscard]] int monsterShaderKind(game::Archetype archetype);

// Items.
struct IconLook {
    float id = 0.0f;    // ui.frag item icon family
    float style = 0.0f;
    Color a;
    Color b;
};
[[nodiscard]] IconLook itemIcon(int item);
[[nodiscard]] Color rarityColor(game::Rarity rarity);
[[nodiscard]] Color paletteColor(int index); // kAnimePalette

[[nodiscard]] Color regionAccent(game::Region region);

} // namespace wayfarer::app
