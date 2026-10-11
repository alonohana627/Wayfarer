// Frostcrown Peak: snowy ledges and pines climbing into the sky - the
// Snowcap Trail, Frostwind Ridge, the Ice Caves and the Crown of Frost.

#include "maps.hpp"

namespace wayfarer::game {

namespace {

// helpers

using K = DecoKind;
using S = SpeciesId;

void peak1(MapDef& def)
{
    def.name = "Snowcap Trail";
    constexpr int g = 29;
    MapBuilder b{112, 34};
    b.ground(0, 111, g);
    b.rampUp(12, g, 2).ground(14, 28, g - 2).rampUp(29, g - 2, 2).ground(31, 44, g - 4).rampUp(45, g - 4, 2).ground(47, 62, g - 6);
    b.rampDown(63, g, 6);
    b.foothold(18, 26, g - 6).rope(20, g - 6, g - 3).foothold(34, 42, g - 8).rope(36, g - 8, g - 5);
    b.foothold(50, 60, g - 10).rope(52, g - 10, g - 7).foothold(54, 62, g - 14).rope(58, g - 14, g - 11);
    b.foothold(72, 82, g - 4).rope(74, g - 4, g - 1).foothold(78, 90, g - 8).rope(80, g - 8, g - 5).foothold(86, 98, g - 12).rope(90, g - 12, g - 9);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Edge, .at = feetOn(1.5f, g), .to = MapId::Town, .toPortal = 4, .label = "Hearthvale"},
        {.kind = PortalKind::Edge, .at = feetOn(110.5f, g), .to = MapId::Peak2, .toPortal = 0, .label = "Frostwind Ridge"},
    };
    def.spawns = {
        {S::SnowPudgy, runOn(14, 28, g - 2), 1}, {S::IcePebblet, runOn(31, 44, g - 4), 2}, {S::SnowPudgy, runOn(47, 62, g - 6), 2},
        {S::IcePebblet, runOn(18, 26, g - 6), 1}, {S::FrostWisp, runOn(40, 60, g - 16), 1}, {S::SnowPudgy, runOn(70, 108, g), 3},
        {S::IcePebblet, runOn(78, 90, g - 8), 1}, {S::FrostWisp, runOn(84, 100, g - 15), 1}, {S::FrostMochi, runOn(50, 60, g - 10), 1},
        {S::FrostMochi, runOn(86, 98, g - 12), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Pine, {tx(5.0f), y}, 1, 1.0f, false},     {K::Sign, {tx(9.0f), y}, 0, 1.0f, false},
        {K::Pine, {tx(22.0f), (g - 2) * kTile}, 1, 1.1f, false}, {K::Snowpile, {tx(40.0f), (g - 4) * kTile}, 0, 1.0f, true},
        {K::Pine, {tx(56.0f), (g - 6) * kTile}, 1, 1.2f, false}, {K::Rock, {tx(68.0f), y}, 0, 1.0f, false},
        {K::Pine, {tx(76.0f), y}, 1, 1.0f, false},    {K::Snowpile, {tx(94.0f), y}, 0, 1.2f, true},
        {K::Pine, {tx(104.0f), y}, 1, 1.3f, false},   {K::Lantern, {tx(108.0f), y}, 0, 1.0f, false},
    };
}

void peak2(MapDef& def)
{
    def.name = "Frostwind Ridge";
    constexpr int g = 37;
    MapBuilder b{96, 42};
    b.ground(0, 95, g);
    // A tall climb: ledges zig-zag up to the cave mouth near the top.
    b.foothold(6, 20, g - 4).rope(8, g - 4, g - 1);
    b.foothold(16, 32, g - 8).rope(18, g - 8, g - 5);
    b.foothold(28, 44, g - 12).rope(30, g - 12, g - 9);
    b.foothold(40, 56, g - 16).rope(42, g - 16, g - 13);
    b.foothold(52, 68, g - 20).rope(54, g - 20, g - 17);
    b.foothold(64, 80, g - 24).rope(66, g - 24, g - 21);
    b.block(78, 95, g - 28, g - 26).ladder(77, g - 28, g - 25);
    b.foothold(60, 74, g - 4).rope(70, g - 4, g - 1).foothold(70, 88, g - 8).rope(72, g - 8, g - 5);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Edge, .at = feetOn(1.5f, g), .to = MapId::Peak1, .toPortal = 1, .label = "Snowcap Trail"},
        {.kind = PortalKind::Door, .at = feetOn(90.0f, g - 28), .to = MapId::PeakDeep, .toPortal = 0, .label = "The Ice Caves"},
    };
    def.spawns = {
        {S::SnowPudgy, runOn(2, 94, g), 3},       {S::IcePebblet, runOn(16, 32, g - 8), 1}, {S::FrostMochi, runOn(28, 44, g - 12), 1},
        {S::IcePebblet, runOn(40, 56, g - 16), 1}, {S::FrostMochi, runOn(52, 68, g - 20), 1}, {S::SnowPudgy, runOn(64, 80, g - 24), 1},
        {S::FrostWisp, runOn(20, 50, g - 22), 1}, {S::FrostWisp, runOn(50, 80, g - 30), 1},   {S::IcePebblet, runOn(78, 94, g - 28), 1},
        {S::FrostMochi, runOn(70, 88, g - 8), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Pine, {tx(4.0f), y}, 1, 1.2f, false},     {K::Snowpile, {tx(25.0f), y}, 0, 1.0f, true},
        {K::Pine, {tx(46.0f), y}, 1, 1.3f, false},    {K::Rock, {tx(58.0f), y}, 0, 1.1f, false},
        {K::Pine, {tx(90.0f), y}, 1, 1.1f, false},    {K::Pine, {tx(84.0f), (g - 28) * kTile}, 1, 1.0f, false},
        {K::Torch, {tx(87.0f), (g - 28) * kTile}, 0, 1.0f, false}, {K::Torch, {tx(93.0f), (g - 28) * kTile}, 0, 1.0f, false},
        {K::Snowpile, {tx(36.0f), (g - 12) * kTile}, 0, 0.8f, true},
    };
}

void peakDeep(MapDef& def)
{
    def.name = "The Ice Caves";
    def.dungeon = true;
    def.eliteChance = 16;
    constexpr int g = 23;
    MapBuilder b{96, 28};
    b.ground(0, 95, g);
    b.foothold(8, 18, 19).rope(10, 19, g - 1).foothold(14, 24, 15).rope(16, 15, 18);
    b.rampUp(28, g, 2).ground(30, 46, g - 2).rampDown(47, g, 2);
    b.foothold(32, 44, 17).rope(34, 17, 20).foothold(36, 42, 13).rope(38, 13, 16);
    b.block(54, 57, g - 4, g).ladder(53, g - 4, g - 1).foothold(58, 70, g - 4).foothold(64, 76, g - 8).rope(66, g - 8, g - 5);
    b.foothold(80, 88, 19).rope(82, 19, g - 1);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Door, .at = feetOn(3.0f, g), .to = MapId::Peak2, .toPortal = 1, .label = "Frostwind Ridge"},
        {.kind = PortalKind::Door, .at = feetOn(92.0f, g), .to = MapId::PeakArena, .toPortal = 0, .label = "The Crown of Frost"},
    };
    def.spawns = {
        {S::IcePebblet, runOn(5, 26, g), 2},     {S::FrostMochi, runOn(8, 18, 19), 1},     {S::FrostWisp, runOn(14, 30, 10), 1},
        {S::SnowPudgy, runOn(30, 46, g - 2), 2}, {S::IcePebblet, runOn(32, 44, 17), 1},    {S::FrostWisp, runOn(40, 60, 9), 1},
        {S::SnowPudgy, runOn(50, 90, g), 2},     {S::FrostMochi, runOn(58, 70, g - 4), 1}, {S::IcePebblet, runOn(64, 76, g - 8), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Crystal, {tx(6.0f), y}, 0, 1.1f, false},  {K::Torch, {tx(12.0f), y}, 0, 1.0f, false},
        {K::Crystal, {tx(26.0f), y}, 1, 0.9f, false}, {K::Snowpile, {tx(40.0f), (g - 2) * kTile}, 0, 1.0f, true},
        {K::Crystal, {tx(60.0f), y}, 0, 1.2f, false}, {K::Torch, {tx(78.0f), y}, 0, 1.0f, false},
        {K::Crystal, {tx(88.0f), y}, 0, 1.0f, false}, {K::Torch, {tx(95.0f), y}, 0, 1.0f, false},
    };
}

void peakArena(MapDef& def)
{
    def.name = "The Crown of Frost";
    def.dungeon = true;
    constexpr int g = 19;
    MapBuilder b{48, 26};
    b.ground(0, 47, g);
    b.foothold(5, 14, 16).foothold(33, 42, 16).foothold(19, 28, 13);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Door, .at = feetOn(2.5f, g), .to = MapId::PeakDeep, .toPortal = 1, .label = "The Ice Caves"},
        {.kind = PortalKind::Door, .at = feetOn(24.0f, g), .to = MapId::Town, .toPortal = 4, .label = "Hearthvale", .afterBoss = true},
    };
    def.arena = Arena{.boss = BossId::Hush, .bossFeet = feetOn(34.0f, g), .bounds = {tx(1.0f), 0.0f, tx(46.0f), g * kTile}};
    const float y = g * kTile;
    def.decos = {
        {K::Crystal, {tx(1.5f), y}, 0, 1.6f, false}, {K::Crystal, {tx(46.0f), y}, 1, 1.6f, false},
        {K::Pine, {tx(9.0f), y}, 1, 1.0f, false},    {K::Pine, {tx(39.0f), y}, 1, 1.0f, false},
        {K::Snowpile, {tx(17.0f), y}, 0, 1.0f, true}, {K::Snowpile, {tx(31.0f), y}, 0, 1.0f, true},
    };
}

} // namespace

void buildPeak(MapDef& def)
{
    def.region = Region::Peak;
    switch (def.id) {
    case MapId::Peak1: peak1(def); break;
    case MapId::Peak2: peak2(def); break;
    case MapId::PeakDeep: peakDeep(def); break;
    case MapId::PeakArena: peakArena(def); break;
    default: break;
    }
}

} // namespace wayfarer::game
