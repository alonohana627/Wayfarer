// The Glimmerdeep Mines: wooden scaffolds and ladders over glowing ore - the
// Glimmer Gate, the tall Lanternway, the Deep Shaft and Rumblerock's Hollow.

#include "maps.hpp"

namespace wayfarer::game {

namespace {

// helpers

using K = DecoKind;
using S = SpeciesId;

void mines1(MapDef& def)
{
    def.name = "Glimmer Gate";
    constexpr int g = 25;
    MapBuilder b{104, 30};
    b.ground(0, 103, g);
    b.rampUp(14, g, 2).ground(16, 34, g - 2).rampDown(35, g, 2);
    b.foothold(4, 12, 21).ladder(10, 21, g - 1);
    b.foothold(20, 30, 18).ladder(24, 18, g - 3);
    b.foothold(26, 34, 14).ladder(30, 14, 17);
    b.block(46, 49, g - 6, g).ladder(45, g - 6, g - 1).foothold(50, 58, g - 6);
    b.foothold(60, 70, g - 3);
    b.foothold(66, 76, g - 7).ladder(68, g - 7, g - 4);
    b.foothold(80, 92, g - 4).ladder(84, g - 4, g - 1).foothold(86, 98, g - 8).ladder(90, g - 8, g - 5);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Edge, .at = feetOn(1.5f, g), .to = MapId::Town, .toPortal = 1, .label = "Hearthvale"},
        {.kind = PortalKind::Edge, .at = feetOn(102.5f, g), .to = MapId::Mines2, .toPortal = 0, .label = "Lanternway"},
    };
    def.spawns = {
        {S::Pebblet, runOn(16, 34, g - 2), 2},  {S::Pebblet, runOn(39, 44, g), 1},     {S::Pebblet, runOn(52, 102, g), 4},
        {S::EmberMochi, runOn(20, 30, 18), 1},  {S::EmberMochi, runOn(50, 58, g - 6), 1}, {S::EmberMochi, runOn(80, 92, g - 4), 1},
        {S::Flitter, runOn(24, 36, 12), 1},     {S::Flitter, runOn(62, 76, g - 9), 1}, {S::Flitter, runOn(86, 98, g - 10), 1},
        {S::EmberMochi, runOn(66, 76, g - 7), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Torch, {tx(4.0f), y}, 0, 1.0f, false},        {K::Sign, {tx(7.0f), y}, 0, 1.0f, false},
        {K::Crystal, {tx(13.0f), y}, 0, 1.0f, false},     {K::Cart, {tx(22.0f), (g - 2) * kTile}, 0, 1.0f, false},
        {K::Lantern, {tx(31.0f), (g - 2) * kTile}, 0, 1.0f, false}, {K::Rock, {tx(41.0f), y}, 0, 1.0f, false},
        {K::Crystal, {tx(54.0f), (g - 6) * kTile}, 1, 0.9f, false}, {K::Torch, {tx(58.0f), y}, 0, 1.0f, false},
        {K::Barrel, {tx(63.0f), y}, 0, 1.0f, false},      {K::Crate, {tx(64.2f), y}, 0, 1.0f, false},
        {K::Cart, {tx(78.0f), y}, 0, 1.0f, false},         {K::Crystal, {tx(90.0f), (g - 4) * kTile}, 0, 0.8f, false},
        {K::Lantern, {tx(99.0f), y}, 0, 1.0f, false},     {K::Crystal, {tx(101.0f), y}, 1, 1.1f, false},
    };
}

void mines2(MapDef& def)
{
    def.name = "Lanternway";
    constexpr int g = 31;
    MapBuilder b{100, 36};
    b.ground(0, 99, g);
    b.foothold(8, 40, g - 5).ladder(10, g - 5, g - 1).ladder(36, g - 5, g - 1);
    b.foothold(30, 70, g - 10).ladder(38, g - 10, g - 6);
    b.foothold(55, 95, g - 15).ladder(60, g - 15, g - 11);
    b.foothold(70, 96, g - 5).ladder(92, g - 5, g - 1).ladder(74, g - 10, g - 6).foothold(72, 80, g - 10);
    b.foothold(14, 26, g - 12).ladder(16, g - 12, g - 6);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Edge, .at = feetOn(1.5f, g), .to = MapId::Mines1, .toPortal = 1, .label = "Glimmer Gate"},
        {.kind = PortalKind::Door, .at = feetOn(92.0f, g - 15), .to = MapId::MinesDeep, .toPortal = 0, .label = "The Deep Shaft"},
    };
    def.spawns = {
        {S::Pebblet, runOn(2, 98, g), 4},           {S::LanternCap, runOn(8, 40, g - 5), 2}, {S::EmberMochi, runOn(30, 70, g - 10), 2},
        {S::LanternCap, runOn(55, 88, g - 15), 2}, {S::Pebblet, runOn(70, 96, g - 5), 2},   {S::Flitter, runOn(20, 50, g - 14), 2},
        {S::Flitter, runOn(60, 90, g - 19), 1},     {S::EmberMochi, runOn(14, 26, g - 12), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Lantern, {tx(5.0f), y}, 0, 1.0f, false},      {K::Crystal, {tx(20.0f), y}, 1, 1.0f, false},
        {K::Cart, {tx(45.0f), y}, 0, 1.0f, false},        {K::Torch, {tx(52.0f), y}, 0, 1.0f, false},
        {K::Crystal, {tx(66.0f), y}, 0, 1.2f, false},     {K::Barrel, {tx(84.0f), y}, 0, 1.0f, false},
        {K::Lantern, {tx(24.0f), (g - 5) * kTile}, 0, 1.0f, false}, {K::Crystal, {tx(50.0f), (g - 10) * kTile}, 1, 0.8f, false},
        {K::Torch, {tx(88.0f), (g - 15) * kTile}, 0, 1.0f, false}, {K::Torch, {tx(96.0f), (g - 15) * kTile}, 0, 1.0f, false},
        {K::Crystal, {tx(78.0f), (g - 5) * kTile}, 0, 0.8f, false}, {K::Crate, {tx(30.0f), (g - 5) * kTile}, 0, 1.0f, false},
    };
}

void minesDeep(MapDef& def)
{
    def.name = "The Deep Shaft";
    def.dungeon = true;
    def.eliteChance = 14;
    constexpr int g = 23;
    MapBuilder b{92, 28};
    b.ground(0, 91, g);
    b.block(16, 22, g - 3, g).ladder(15, g - 3, g - 1).foothold(23, 30, g - 3);
    b.foothold(26, 36, g - 7).ladder(28, g - 7, g - 4);
    b.rampUp(40, g, 3).ground(43, 58, g - 3).rampDown(59, g, 3);
    b.foothold(46, 56, g - 7).ladder(47, g - 7, g - 4).foothold(48, 54, g - 11).ladder(51, g - 11, g - 8);
    b.foothold(66, 80, g - 4).ladder(70, g - 4, g - 1).foothold(74, 84, g - 8).ladder(80, g - 8, g - 5);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Door, .at = feetOn(3.0f, g), .to = MapId::Mines2, .toPortal = 1, .label = "Lanternway"},
        {.kind = PortalKind::Door, .at = feetOn(88.0f, g), .to = MapId::MinesArena, .toPortal = 0, .label = "Rumblerock's Hollow"},
    };
    def.spawns = {
        {S::LanternCap, runOn(5, 14, g), 1},    {S::Pebblet, runOn(16, 22, g - 3), 1},   {S::EmberMochi, runOn(26, 36, g - 7), 1},
        {S::Pebblet, runOn(24, 39, g), 2},      {S::LanternCap, runOn(43, 58, g - 3), 2}, {S::Flitter, runOn(44, 58, g - 13), 1},
        {S::EmberMochi, runOn(46, 56, g - 7), 1}, {S::Pebblet, runOn(62, 86, g), 2},     {S::LanternCap, runOn(74, 84, g - 8), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Torch, {tx(6.0f), y}, 0, 1.0f, false},       {K::Crystal, {tx(12.0f), y}, 1, 1.2f, false},
        {K::Crystal, {tx(28.0f), y}, 0, 1.0f, false},    {K::Cart, {tx(36.0f), y}, 0, 1.0f, false},
        {K::Crystal, {tx(52.0f), (g - 3) * kTile}, 1, 1.3f, false}, {K::Torch, {tx(62.0f), y}, 0, 1.0f, false},
        {K::Crystal, {tx(77.0f), y}, 0, 1.0f, false},    {K::Torch, {tx(85.0f), y}, 0, 1.0f, false},
        {K::Torch, {tx(91.0f), y}, 0, 1.0f, false},
    };
}

void minesArena(MapDef& def)
{
    def.name = "Rumblerock's Hollow";
    def.dungeon = true;
    constexpr int g = 19;
    MapBuilder b{44, 24};
    b.ground(0, 43, g);
    b.foothold(5, 11, 16).foothold(32, 38, 16);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Door, .at = feetOn(2.5f, g), .to = MapId::MinesDeep, .toPortal = 1, .label = "The Deep Shaft"},
        {.kind = PortalKind::Door, .at = feetOn(22.0f, g), .to = MapId::Town, .toPortal = 1, .label = "Hearthvale", .afterBoss = true},
    };
    def.arena = Arena{.boss = BossId::Rumblerock, .bossFeet = feetOn(31.0f, g), .bounds = {tx(1.0f), 0.0f, tx(42.0f), g * kTile}};
    const float y = g * kTile;
    def.decos = {
        {K::Crystal, {tx(1.5f), y}, 1, 1.5f, false}, {K::Crystal, {tx(42.0f), y}, 0, 1.6f, false},
        {K::Torch, {tx(5.0f), y}, 0, 1.0f, false},   {K::Torch, {tx(39.0f), y}, 0, 1.0f, false},
        {K::Crystal, {tx(15.0f), y}, 0, 0.7f, false}, {K::Crystal, {tx(28.0f), y}, 1, 0.7f, false},
    };
}

} // namespace

void buildMines(MapDef& def)
{
    def.region = Region::Mines;
    switch (def.id) {
    case MapId::Mines1: mines1(def); break;
    case MapId::Mines2: mines2(def); break;
    case MapId::MinesDeep: minesDeep(def); break;
    case MapId::MinesArena: minesArena(def); break;
    default: break;
    }
}

} // namespace wayfarer::game
