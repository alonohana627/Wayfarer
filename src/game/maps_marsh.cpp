// Murkmallow Marsh: mossy banks, reeds and willows over still water - the
// Reedwater Banks, the Mallow Mire, the Sunken Grotto and Murkshell's Pool.

#include "maps.hpp"

namespace wayfarer::game {

namespace {

// helpers

using K = DecoKind;
using S = SpeciesId;

void marsh1(MapDef& def)
{
    def.name = "Reedwater Banks";
    constexpr int g = 23;
    MapBuilder b{110, 28};
    b.ground(0, 109, g);
    b.rampUp(10, g, 1).ground(11, 24, g - 1).rampDown(25, g, 1);
    b.foothold(14, 22, 19).foothold(18, 28, 16).rope(26, 16, g - 1);
    b.foothold(34, 42, 20).foothold(38, 48, 17).rope(46, 17, g - 1).foothold(44, 52, 14).rope(50, 14, g - 1);
    b.rampUp(56, g, 2).ground(58, 70, g - 2).rampDown(71, g, 2);
    b.foothold(60, 68, 17).rope(62, 17, 20).foothold(64, 72, 13).rope(66, 13, 16);
    b.foothold(78, 88, 20).foothold(84, 96, 17).rope(94, 17, g - 1).foothold(90, 100, 14).rope(92, 14, 16);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Edge, .at = feetOn(1.5f, g), .to = MapId::Town, .toPortal = 2, .label = "Hearthvale"},
        {.kind = PortalKind::Edge, .at = feetOn(108.5f, g), .to = MapId::Marsh2, .toPortal = 0, .label = "Mallow Mire"},
    };
    def.spawns = {
        {S::Turbanshell, runOn(11, 24, g - 1), 1}, {S::Turbanshell, runOn(28, 55, g), 2}, {S::Turbanshell, runOn(74, 106, g), 3},
        {S::BogMochi, runOn(14, 22, 19), 1},       {S::BogMochi, runOn(38, 48, 17), 1},   {S::BogMochi, runOn(58, 70, g - 2), 2},
        {S::BogMochi, runOn(84, 96, 17), 1},       {S::ReedFlitter, runOn(40, 54, 10), 1}, {S::ReedFlitter, runOn(86, 100, 10), 1},
        {S::BogMochi, runOn(64, 72, 13), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Reeds, {tx(4.0f), y}, 0, 1.0f, true},      {K::Sign, {tx(7.0f), y}, 0, 1.0f, false},
        {K::Tree, {tx(17.0f), (g - 1) * kTile}, 0, 1.3f, false}, {K::Reeds, {tx(30.0f), y}, 0, 1.2f, true},
        {K::BigShroom, {tx(36.0f), y}, 0, 0.9f, false}, {K::Rock, {tx(53.0f), y}, 1, 1.0f, false},
        {K::Tree, {tx(64.0f), (g - 2) * kTile}, 1, 1.2f, false}, {K::Reeds, {tx(75.0f), y}, 0, 1.0f, true},
        {K::Bush, {tx(82.0f), y}, 0, 1.0f, true},      {K::Tree, {tx(98.0f), y}, 0, 1.3f, false},
        {K::Reeds, {tx(104.0f), y}, 0, 1.1f, true},    {K::Lantern, {tx(106.0f), y}, 0, 1.0f, false},
    };
}

void marsh2(MapDef& def)
{
    def.name = "Mallow Mire";
    constexpr int g = 27;
    MapBuilder b{112, 32};
    b.ground(0, 112, g);
    // Islands of higher ground between low channels.
    b.ground(8, 22, g - 2).ground(30, 46, g - 3).ladder(29, g - 3, g - 1).ground(54, 66, g - 2).ground(76, 96, g - 3).ladder(75, g - 3, g - 1);
    b.foothold(12, 20, g - 6).rope(16, g - 6, g - 3);
    b.foothold(32, 44, g - 7).rope(36, g - 7, g - 4).foothold(38, 50, g - 11).rope(40, g - 11, g - 8);
    b.foothold(56, 66, g - 6).rope(58, g - 6, g - 3).foothold(60, 72, g - 10).rope(64, g - 10, g - 7);
    b.foothold(80, 94, g - 7).rope(90, g - 7, g - 4).foothold(84, 92, g - 11).rope(86, g - 11, g - 8);
    b.rampUp(100, g, 1).ground(101, 111, g - 1);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Edge, .at = feetOn(1.5f, g), .to = MapId::Marsh1, .toPortal = 1, .label = "Reedwater Banks"},
        {.kind = PortalKind::Door, .at = feetOn(106.0f, g - 1), .to = MapId::MarshDeep, .toPortal = 0, .label = "The Sunken Grotto"},
    };
    def.spawns = {
        {S::Murkcap, runOn(8, 22, g - 2), 2},    {S::Turbanshell, runOn(30, 46, g - 3), 2}, {S::BogMochi, runOn(32, 44, g - 7), 1},
        {S::Murkcap, runOn(38, 50, g - 11), 1},  {S::Turbanshell, runOn(54, 66, g - 2), 1}, {S::Murkcap, runOn(60, 72, g - 10), 1},
        {S::BogMochi, runOn(76, 96, g - 3), 2},  {S::Murkcap, runOn(80, 94, g - 7), 1},     {S::ReedFlitter, runOn(20, 40, g - 14), 1},
        {S::ReedFlitter, runOn(60, 80, g - 14), 1}, {S::ReedFlitter, runOn(84, 100, g - 14), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Reeds, {tx(5.0f), y}, 0, 1.2f, true},      {K::Tree, {tx(14.0f), (g - 2) * kTile}, 0, 1.2f, false},
        {K::Reeds, {tx(25.0f), y}, 0, 1.0f, true},     {K::BigShroom, {tx(42.0f), (g - 3) * kTile}, 0, 1.1f, false},
        {K::Reeds, {tx(51.0f), y}, 0, 1.1f, true},     {K::Tree, {tx(63.0f), (g - 2) * kTile}, 1, 1.1f, false},
        {K::Reeds, {tx(71.0f), y}, 0, 1.0f, true},     {K::Tree, {tx(82.0f), (g - 3) * kTile}, 0, 1.3f, false},
        {K::Reeds, {tx(98.0f), y}, 0, 1.0f, true},     {K::Lantern, {tx(103.0f), (g - 1) * kTile}, 0, 1.0f, false},
        {K::Torch, {tx(109.0f), (g - 1) * kTile}, 0, 1.0f, false},
    };
}

void marshDeep(MapDef& def)
{
    def.name = "The Sunken Grotto";
    def.dungeon = true;
    def.eliteChance = 14;
    constexpr int g = 23;
    MapBuilder b{94, 28};
    b.ground(0, 93, g);
    b.foothold(8, 18, 19).rope(12, 19, g - 1).foothold(14, 26, 15).rope(16, 15, 18);
    b.block(30, 34, g - 4, g).ladder(29, g - 4, g - 1).foothold(35, 44, g - 4).foothold(40, 50, g - 8).rope(42, g - 8, g - 5);
    b.rampUp(54, g, 2).ground(56, 70, g - 2).rampDown(71, g, 2);
    b.foothold(58, 68, 17).rope(60, 17, g - 3).foothold(76, 86, 19).rope(80, 19, g - 1).foothold(80, 90, 15).rope(84, 15, 18);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Door, .at = feetOn(3.0f, g), .to = MapId::Marsh2, .toPortal = 1, .label = "Mallow Mire"},
        {.kind = PortalKind::Door, .at = feetOn(90.0f, g), .to = MapId::MarshArena, .toPortal = 0, .label = "Murkshell's Pool"},
    };
    def.spawns = {
        {S::Turbanshell, runOn(6, 28, g), 2},   {S::Murkcap, runOn(8, 18, 19), 1},       {S::BogMochi, runOn(14, 26, 15), 1},
        {S::Murkcap, runOn(30, 34, g - 4), 1},  {S::BogMochi, runOn(40, 50, g - 8), 1},  {S::Turbanshell, runOn(36, 53, g), 1},
        {S::Murkcap, runOn(56, 70, g - 2), 2},  {S::ReedFlitter, runOn(56, 72, 12), 1},  {S::Turbanshell, runOn(74, 88, g), 2},
        {S::BogMochi, runOn(80, 90, 15), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Torch, {tx(6.0f), y}, 0, 1.0f, false},       {K::BigShroom, {tx(20.0f), y}, 0, 1.2f, false},
        {K::Reeds, {tx(27.0f), y}, 0, 1.0f, true},       {K::Crystal, {tx(46.0f), y}, 0, 0.9f, false},
        {K::BigShroom, {tx(62.0f), (g - 2) * kTile}, 0, 1.0f, false}, {K::Reeds, {tx(74.0f), y}, 0, 1.0f, true},
        {K::Torch, {tx(86.0f), y}, 0, 1.0f, false},      {K::Torch, {tx(93.0f), y}, 0, 1.0f, false},
    };
}

void marshArena(MapDef& def)
{
    def.name = "Murkshell's Pool";
    def.dungeon = true;
    constexpr int g = 19;
    MapBuilder b{44, 24};
    b.ground(0, 43, g);
    b.foothold(5, 11, 16).foothold(32, 38, 16);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Door, .at = feetOn(2.5f, g), .to = MapId::MarshDeep, .toPortal = 1, .label = "The Sunken Grotto"},
        {.kind = PortalKind::Door, .at = feetOn(22.0f, g), .to = MapId::Town, .toPortal = 2, .label = "Hearthvale", .afterBoss = true},
    };
    def.arena = Arena{.boss = BossId::Murkshell, .bossFeet = feetOn(30.0f, g), .bounds = {tx(1.0f), 0.0f, tx(42.0f), g * kTile}};
    const float y = g * kTile;
    def.decos = {
        {K::Reeds, {tx(1.5f), y}, 0, 1.4f, true}, {K::Reeds, {tx(42.0f), y}, 0, 1.4f, true},
        {K::Torch, {tx(5.0f), y}, 0, 1.0f, false}, {K::Torch, {tx(39.0f), y}, 0, 1.0f, false},
        {K::BigShroom, {tx(13.0f), y}, 0, 0.9f, false},
    };
}

} // namespace

void buildMarsh(MapDef& def)
{
    def.region = Region::Marsh;
    switch (def.id) {
    case MapId::Marsh1: marsh1(def); break;
    case MapId::Marsh2: marsh2(def); break;
    case MapId::MarshDeep: marshDeep(def); break;
    case MapId::MarshArena: marshArena(def); break;
    default: break;
    }
}

} // namespace wayfarer::game
