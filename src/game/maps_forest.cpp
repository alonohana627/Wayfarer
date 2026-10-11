// Bellwood Forest: sunny meadows under round trees (Bellwood Path, Mossy
// Hollow), the dim Rootway under the old trees and Grandcap's Glade.

#include "maps.hpp"

namespace wayfarer::game {

namespace {

// helpers

using K = DecoKind;
using S = SpeciesId;

void forest1(MapDef& def)
{
    def.name = "Bellwood Path";
    constexpr int g = 23;
    MapBuilder b{112, 28};
    b.ground(0, 111, g);
    b.rampUp(13, g, 1).ground(14, 30, g - 1).rampDown(31, g, 1);
    b.foothold(17, 24, 19).foothold(21, 29, 16);
    b.foothold(36, 44, 20).foothold(46, 53, 17).rope(49, 17, g - 1).foothold(39, 47, 13).rope(43, 13, 19);
    b.rampUp(57, g, 3).ground(60, 72, g - 3).rampDown(73, g, 3);
    b.foothold(62, 70, 17).foothold(64, 69, 14);
    b.foothold(80, 90, 20).foothold(86, 96, 17).foothold(92, 100, 14).rope(98, 14, g - 1).foothold(102, 108, 20);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Edge, .at = feetOn(1.5f, g), .to = MapId::Town, .toPortal = 0, .label = "Hearthvale"},
        {.kind = PortalKind::Edge, .at = feetOn(110.5f, g), .to = MapId::Forest2, .toPortal = 0, .label = "Mossy Hollow"},
    };
    def.spawns = {
        {S::Bellcap, runOn(34, 56, g), 3},   {S::Bellcap, runOn(76, 108, g), 3}, {S::Bellcap, runOn(61, 72, g - 3), 2},
        {S::Bellcap, runOn(15, 29, g - 1), 1}, {S::PinkMochi, runOn(36, 44, 20), 1}, {S::PinkMochi, runOn(46, 53, 17), 1},
        {S::PinkMochi, runOn(86, 96, 17), 2}, {S::PinkMochi, runOn(62, 70, 17), 1}, {S::PinkMochi, runOn(39, 47, 13), 1},
        {S::Bellcap, runOn(80, 90, 20), 1},  {S::PinkMochi, runOn(92, 100, 14), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Sign, {tx(5.0f), y}, 0, 1.0f, false},     {K::Tree, {tx(9.0f), y}, 0, 1.2f, false},
        {K::Flowers, {tx(11.0f), y}, 0, 1.0f, true},  {K::Tree, {tx(24.0f), (g - 1) * kTile}, 1, 1.0f, false},
        {K::Bush, {tx(19.5f), (g - 1) * kTile}, 0, 1.0f, true}, {K::BigShroom, {tx(34.0f), y}, 0, 1.0f, false},
        {K::Tree, {tx(40.0f), y}, 0, 1.3f, false},    {K::Grass, {tx(45.0f), y}, 0, 1.0f, true},
        {K::Rock, {tx(54.0f), y}, 0, 1.0f, false},    {K::Tree, {tx(66.0f), (g - 3) * kTile}, 1, 1.2f, false},
        {K::Flowers, {tx(70.0f), (g - 3) * kTile}, 1, 1.0f, true}, {K::BigShroom, {tx(78.0f), y}, 1, 0.8f, false},
        {K::Tree, {tx(84.0f), y}, 0, 1.1f, false},    {K::Bush, {tx(91.0f), y}, 1, 1.0f, true},
        {K::Tree, {tx(103.0f), y}, 1, 1.25f, false},  {K::Grass, {tx(100.0f), y}, 0, 1.0f, true},
        {K::Sign, {tx(107.5f), y}, 1, 1.0f, false},   {K::Flowers, {tx(60.0f), y}, 0, 1.0f, true},
    };
}

void forest2(MapDef& def)
{
    def.name = "Mossy Hollow";
    constexpr int g = 25;
    MapBuilder b{120, 30};
    b.ground(0, 119, g);
    b.foothold(6, 14, 22).foothold(10, 18, 19).rope(16, 19, 21);
    b.ground(21, 40, g - 5).ladder(20, g - 5, g - 1).rampDown(41, g, 5);
    b.foothold(24, 32, 17).foothold(28, 36, 14);
    b.foothold(50, 58, 22).foothold(55, 64, 19).foothold(60, 70, 16).rope(66, 16, g - 1).foothold(66, 74, 13);
    b.foothold(84, 92, 22).foothold(88, 98, 19).rope(96, 19, g - 1).foothold(92, 100, 16);
    b.rampUp(104, g, 1).ground(105, 119, g - 1);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Edge, .at = feetOn(1.5f, g), .to = MapId::Forest1, .toPortal = 1, .label = "Bellwood Path"},
        {.kind = PortalKind::Door, .at = feetOn(113.0f, g - 1), .to = MapId::ForestDeep, .toPortal = 0, .label = "Rootway"},
    };
    def.spawns = {
        {S::PinkMochi, runOn(2, 19, g), 2},   {S::Pudgy, runOn(22, 40, g - 5), 2},  {S::PinkMochi, runOn(24, 32, 17), 1},
        {S::Stumpling, runOn(46, 82, g), 3},  {S::Pudgy, runOn(55, 64, 19), 1},     {S::PinkMochi, runOn(60, 70, 16), 1},
        {S::Stumpling, runOn(66, 74, 13), 1}, {S::Pudgy, runOn(84, 103, g), 2},     {S::Stumpling, runOn(88, 98, 19), 1},
        {S::PinkMochi, runOn(92, 100, 16), 1}, {S::Pudgy, runOn(50, 58, 22), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Tree, {tx(4.0f), y}, 1, 1.2f, false},       {K::Bush, {tx(8.0f), y}, 0, 1.0f, true},
        {K::Tree, {tx(26.0f), (g - 5) * kTile}, 0, 1.3f, false}, {K::BigShroom, {tx(37.0f), (g - 5) * kTile}, 0, 1.1f, false},
        {K::Flowers, {tx(31.0f), (g - 5) * kTile}, 1, 1.0f, true}, {K::Rock, {tx(48.0f), y}, 1, 1.0f, false},
        {K::Tree, {tx(53.0f), y}, 0, 1.2f, false},      {K::BigShroom, {tx(62.0f), y}, 1, 1.0f, false},
        {K::Grass, {tx(70.0f), y}, 0, 1.0f, true},      {K::Tree, {tx(78.0f), y}, 1, 1.4f, false},
        {K::Bush, {tx(86.0f), y}, 1, 1.0f, true},       {K::Tree, {tx(101.0f), y}, 0, 1.1f, false},
        {K::Torch, {tx(110.5f), (g - 1) * kTile}, 0, 1.0f, false}, {K::Torch, {tx(115.5f), (g - 1) * kTile}, 0, 1.0f, false},
        {K::Flowers, {tx(108.0f), (g - 1) * kTile}, 0, 1.0f, true},
    };
}

void forestDeep(MapDef& def)
{
    def.name = "The Rootway";
    def.dungeon = true;
    def.eliteChance = 14;
    constexpr int g = 23;
    MapBuilder b{100, 28};
    b.ground(0, 99, g);
    b.rampUp(12, g, 2).ground(14, 22, g - 2).rampDown(23, g, 2);
    b.foothold(16, 24, 17).foothold(26, 34, 14).rope(30, 14, 22);
    b.block(38, 41, g - 4, g).ladder(37, g - 4, g - 1).foothold(42, 50, g - 4);
    b.foothold(46, 54, 15).rope(50, 15, g - 5);
    b.rampUp(58, g, 2).ground(60, 74, g - 2).rampDown(75, g, 2);
    b.foothold(62, 72, 17).foothold(66, 70, 14);
    b.foothold(80, 88, 19).foothold(84, 92, 16).rope(86, 16, g - 1);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Door, .at = feetOn(3.0f, g), .to = MapId::Forest2, .toPortal = 1, .label = "Mossy Hollow"},
        {.kind = PortalKind::Door, .at = feetOn(96.0f, g), .to = MapId::ForestArena, .toPortal = 0, .label = "Grandcap's Glade"},
    };
    def.spawns = {
        {S::Stumpling, runOn(6, 36, g), 2}, {S::Pudgy, runOn(14, 22, g - 2), 1}, {S::Bellcap, runOn(16, 24, 17), 1},
        {S::Stumpling, runOn(26, 34, 14), 1}, {S::Pudgy, runOn(42, 57, g), 2},  {S::Bellcap, runOn(42, 50, g - 4), 1},
        {S::Stumpling, runOn(46, 54, 15), 1}, {S::Pudgy, runOn(60, 74, g - 2), 2}, {S::Bellcap, runOn(62, 72, 17), 1},
        {S::Stumpling, runOn(77, 92, g), 2},  {S::PinkMochi, runOn(84, 92, 16), 1},
    };
    const float y = g * kTile;
    def.decos = {
        {K::Torch, {tx(5.5f), y}, 0, 1.0f, false},      {K::BigShroom, {tx(10.0f), y}, 1, 1.2f, false},
        {K::BigShroom, {tx(20.0f), (g - 2) * kTile}, 0, 0.8f, false}, {K::Rock, {tx(35.0f), y}, 0, 1.0f, false},
        {K::Torch, {tx(44.0f), (g - 4) * kTile}, 0, 1.0f, false}, {K::BigShroom, {tx(56.0f), y}, 1, 1.0f, false},
        {K::Grass, {tx(64.0f), (g - 2) * kTile}, 0, 1.0f, true}, {K::BigShroom, {tx(72.0f), (g - 2) * kTile}, 0, 1.3f, false},
        {K::Torch, {tx(93.5f), y}, 0, 1.0f, false},     {K::Rock, {tx(80.0f), y}, 1, 1.0f, false},
    };
}

void forestArena(MapDef& def)
{
    def.name = "Grandcap's Glade";
    def.dungeon = true;
    constexpr int g = 19;
    MapBuilder b{44, 24};
    b.ground(0, 43, g);
    b.foothold(5, 11, 15).foothold(32, 38, 15);
    def.text = b.take();
    def.portals = {
        {.kind = PortalKind::Door, .at = feetOn(2.5f, g), .to = MapId::ForestDeep, .toPortal = 1, .label = "The Rootway"},
        {.kind = PortalKind::Door, .at = feetOn(22.0f, g), .to = MapId::Town, .toPortal = 0, .label = "Hearthvale",
         .afterBoss = true},
    };
    def.arena = Arena{.boss = BossId::Grandcap, .bossFeet = feetOn(31.0f, g), .bounds = {tx(1.0f), 0.0f, tx(42.0f), g * kTile}};
    const float y = g * kTile;
    def.decos = {
        {K::BigShroom, {tx(1.5f), y}, 0, 1.4f, false}, {K::BigShroom, {tx(42.0f), y}, 1, 1.5f, false},
        {K::Torch, {tx(5.0f), y}, 0, 1.0f, false},     {K::Torch, {tx(39.0f), y}, 0, 1.0f, false},
        {K::Grass, {tx(14.0f), y}, 0, 1.0f, true},     {K::Grass, {tx(29.0f), y}, 0, 1.0f, true},
    };
}

} // namespace

void buildForest(MapDef& def)
{
    def.region = Region::Forest;
    switch (def.id) {
    case MapId::Forest1: forest1(def); break;
    case MapId::Forest2: forest2(def); break;
    case MapId::ForestDeep: forestDeep(def); break;
    case MapId::ForestArena: forestArena(def); break;
    default: break;
    }
}

} // namespace wayfarer::game
