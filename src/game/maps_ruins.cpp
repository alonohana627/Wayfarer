// The Sunspire Ruins: sandstone steps, broken columns and golden arches - the
// Sunspire Steps, the Hall of Pillars, the Vault and the Warden's Court.

#include "maps.hpp"

namespace wayfarer::game {

namespace {

// helpers

using K = DecoKind;
using S = SpeciesId;

void ruins1(MapDef& def)
{
    def.name = "Sunspire Steps";
    constexpr int g = 27;
    MapBuilder b{112, 32};
    b.ground(0, 111, g);
    // Terraces climbing to the right.
    b.rampUp(14, g, 2).ground(16, 30, g - 2).rampUp(31, g - 2, 2).ground(33, 50, g - 4).rampDown(51, g, 4);
    b.foothold(20, 28, g - 6).rope(22, g - 6, g - 3).foothold(36, 46, g - 8).rope(40, g - 8, g - 5);
    b.foothold(42, 52, g - 12).rope(44, g - 12, g - 9);
    b.ground(60, 76, g - 3).ladder(59, g - 3, g - 1).foothold(62, 74, g - 7).rope(66, g - 7, g - 4);
    b.foothold(80, 90, g - 4).rope(84, g - 4, g - 1).foothold(86, 98, g - 8).rope(88, g - 8, g - 5).foothold(92, 102, g - 12).rope(94, g - 12, g - 9);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Edge, .at = feetOn(1.5f, g), .to = MapId::Town, .toPortal = 3, .label = "Hearthvale"},
        {.kind = PortalKind::Edge, .at = feetOn(110.5f, g), .to = MapId::Ruins2, .toPortal = 0, .label = "Hall of Pillars"},
    };
    def.spawns = {
        {S::RunePebblet, runOn(16, 30, g - 2), 2}, {S::RunePebblet, runOn(33, 50, g - 4), 2}, {S::OldStump, runOn(20, 28, g - 6), 1},
        {S::OldStump, runOn(36, 46, g - 8), 1},    {S::Wisp, runOn(40, 56, g - 15), 1},       {S::RunePebblet, runOn(60, 76, g - 3), 2},
        {S::OldStump, runOn(62, 74, g - 7), 1},    {S::RunePebblet, runOn(78, 108, g), 2},    {S::Wisp, runOn(84, 100, g - 15), 1},
        {S::OldStump, runOn(86, 98, g - 8), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Column, {tx(6.0f), y}, 1, 1.0f, false},     {K::Sign, {tx(9.0f), y}, 0, 1.0f, false},
        {K::Column, {tx(24.0f), (g - 2) * kTile}, 0, 1.1f, false}, {K::Grass, {tx(29.0f), (g - 2) * kTile}, 0, 1.0f, true},
        {K::Arch, {tx(42.0f), (g - 4) * kTile}, 0, 1.0f, false}, {K::Column, {tx(56.0f), y}, 1, 1.2f, false},
        {K::Rock, {tx(70.0f), (g - 3) * kTile}, 0, 1.0f, false}, {K::Column, {tx(78.0f), y}, 0, 1.0f, false},
        {K::Torch, {tx(88.0f), y}, 0, 1.0f, false},     {K::Column, {tx(104.0f), y}, 1, 1.0f, false},
        {K::Grass, {tx(100.0f), y}, 0, 1.0f, true},
    };
}

void ruins2(MapDef& def)
{
    def.name = "Hall of Pillars";
    constexpr int g = 29;
    MapBuilder b{108, 34};
    b.ground(0, 107, g);
    // Standing pillars with ledges between them.
    for (int i = 0; i < 4; ++i) {
        const int x = 14 + i * 22;
        b.block(x, x + 2, g - 9, g).ladder(x - 1, g - 9, g - 1);
        b.foothold(x + 3, x + 12, g - 9);
        b.foothold(x + 6, x + 16, g - 5).rope(x + 14, g - 5, g - 1);
    }
    b.foothold(20, 32, g - 13).rope(24, g - 13, g - 10).foothold(64, 76, g - 13).rope(70, g - 13, g - 10);
    b.foothold(40, 56, g - 16).rope(46, g - 16, g - 10);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Edge, .at = feetOn(1.5f, g), .to = MapId::Ruins1, .toPortal = 1, .label = "Sunspire Steps"},
        {.kind = PortalKind::Door, .at = feetOn(104.0f, g), .to = MapId::RuinsDeep, .toPortal = 0, .label = "The Vault"},
    };
    def.spawns = {
        {S::RunePebblet, runOn(2, 13, g), 1},     {S::OldStump, runOn(17, 34, g), 2},        {S::SandTurban, runOn(39, 56, g), 2},
        {S::RunePebblet, runOn(61, 78, g), 2},    {S::SandTurban, runOn(83, 102, g), 2},     {S::Wisp, runOn(20, 40, g - 17), 1},
        {S::Wisp, runOn(60, 80, g - 17), 1},      {S::OldStump, runOn(39, 48, g - 9), 1},    {S::RunePebblet, runOn(83, 92, g - 9), 1},
        {S::SandTurban, runOn(20, 32, g - 13), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Torch, {tx(4.0f), y}, 0, 1.0f, false},       {K::Banner, {tx(11.0f), y}, 0, 1.0f, false},
        {K::Column, {tx(30.0f), y}, 1, 1.0f, false},     {K::Torch, {tx(37.0f), y}, 0, 1.0f, false},
        {K::Banner, {tx(55.0f), y}, 1, 1.0f, false},     {K::Column, {tx(74.0f), y}, 0, 1.1f, false},
        {K::Torch, {tx(81.0f), y}, 0, 1.0f, false},      {K::Arch, {tx(104.0f), y}, 0, 1.0f, false},
        {K::Grass, {tx(95.0f), y}, 0, 1.0f, true},
    };
}

void ruinsDeep(MapDef& def)
{
    def.name = "The Vault";
    def.dungeon = true;
    def.eliteChance = 14;
    constexpr int g = 23;
    MapBuilder b{94, 28};
    b.ground(0, 93, g);
    b.foothold(8, 20, 19).rope(10, 19, g - 1).foothold(16, 26, 15).rope(18, 15, 18);
    b.rampUp(30, g, 2).ground(32, 48, g - 2).rampDown(49, g, 2);
    b.foothold(34, 46, 17).rope(36, 17, 20).foothold(38, 44, 13).rope(42, 13, 16);
    b.block(56, 59, g - 4, g).ladder(55, g - 4, g - 1).foothold(60, 70, g - 4).foothold(66, 78, g - 8).rope(68, g - 8, g - 5);
    b.foothold(80, 88, 19).rope(82, 19, g - 1);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Door, .at = feetOn(3.0f, g), .to = MapId::Ruins2, .toPortal = 1, .label = "Hall of Pillars"},
        {.kind = PortalKind::Door, .at = feetOn(90.0f, g), .to = MapId::RuinsArena, .toPortal = 0, .label = "The Warden's Court"},
    };
    def.spawns = {
        {S::OldStump, runOn(6, 28, g), 2},      {S::SandTurban, runOn(8, 20, 19), 1},    {S::Wisp, runOn(14, 30, 11), 1},
        {S::RunePebblet, runOn(32, 48, g - 2), 2}, {S::OldStump, runOn(34, 46, 17), 1},   {S::Wisp, runOn(40, 56, 9), 1},
        {S::SandTurban, runOn(52, 92, g), 2},   {S::RunePebblet, runOn(60, 70, g - 4), 1}, {S::OldStump, runOn(66, 78, g - 8), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Torch, {tx(6.0f), y}, 0, 1.0f, false},     {K::Column, {tx(24.0f), y}, 1, 1.0f, false},
        {K::Torch, {tx(40.0f), (g - 2) * kTile}, 0, 1.0f, false}, {K::Column, {tx(52.0f), y}, 0, 1.0f, false},
        {K::Banner, {tx(74.0f), y}, 0, 1.0f, false},   {K::Torch, {tx(86.0f), y}, 0, 1.0f, false},
        {K::Torch, {tx(93.0f), y}, 0, 1.0f, false},
    };
}

void ruinsArena(MapDef& def)
{
    def.name = "The Warden's Court";
    def.dungeon = true;
    constexpr int g = 19;
    MapBuilder b{44, 24};
    b.ground(0, 43, g);
    b.foothold(5, 11, 16).foothold(32, 38, 16);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Door, .at = feetOn(2.5f, g), .to = MapId::RuinsDeep, .toPortal = 1, .label = "The Vault"},
        {.kind = PortalKind::Door, .at = feetOn(22.0f, g), .to = MapId::Town, .toPortal = 3, .label = "Hearthvale", .afterBoss = true},
    };
    def.arena = Arena{.boss = BossId::Warden, .bossFeet = feetOn(31.0f, g), .bounds = {tx(1.0f), 0.0f, tx(42.0f), g * kTile}};
    const float y = g * kTile;
    def.decos = {
        {K::Column, {tx(2.0f), y}, 0, 1.3f, false}, {K::Column, {tx(42.0f), y}, 0, 1.3f, false},
        {K::Torch, {tx(6.0f), y}, 0, 1.0f, false},  {K::Torch, {tx(38.0f), y}, 0, 1.0f, false},
        {K::Banner, {tx(16.0f), y}, 1, 1.0f, false}, {K::Banner, {tx(28.0f), y}, 0, 1.0f, false},
    };
}

} // namespace

void buildRuins(MapDef& def)
{
    def.region = Region::Ruins;
    switch (def.id) {
    case MapId::Ruins1: ruins1(def); break;
    case MapId::Ruins2: ruins2(def); break;
    case MapId::RuinsDeep: ruinsDeep(def); break;
    case MapId::RuinsArena: ruinsArena(def); break;
    default: break;
    }
}

} // namespace wayfarer::game
