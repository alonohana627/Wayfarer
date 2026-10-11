// Placeholder layouts for the regions not yet designed: two fields, a
// dungeon and an arena with the same portal structure as the forest.

#include "maps.hpp"

#include <array>

namespace wayfarer::game {

namespace {

// helpers

struct RegionMaps {
    MapId first;
    MapId second;
    MapId deep;
    MapId arena;
    BossId boss;
    SpeciesId a;
    SpeciesId b;
    SpeciesId c;
};

void layout(MapDef& def, const RegionMaps& r, const int which)
{
    constexpr int g = 23;
    MapBuilder b{which == 3 ? 44 : 96, 28};
    b.ground(0, which == 3 ? 43 : 95, which == 3 ? 19 : g);
    if (which != 3) {
        b.foothold(14, 24, 20).foothold(30, 42, 17).rope(36, 17, g - 1).foothold(50, 62, 20).foothold(70, 80, 17).rope(74, 17, g - 1);
    } else {
        b.foothold(5, 11, 15).foothold(32, 38, 15);
    }
    def.text = b.take();
    const MapId prev = which == 0 ? MapId::Town : which == 1 ? r.first : which == 2 ? r.second : r.deep;
    const int prevPortal = which == 0 ? static_cast<int>(def.region) - 1 : 1;
    const PortalKind backKind = which == 0 || which == 1 ? PortalKind::Edge : PortalKind::Door;
    def.portals.push_back({.kind = backKind, .at = feetOn(which >= 2 ? 3.0f : 1.5f, which == 3 ? 19 : g), .to = prev,
                           .toPortal = prevPortal});
    if (which < 3) {
        const MapId next = which == 0 ? r.second : which == 1 ? r.deep : r.arena;
        def.portals.push_back({.kind = which == 0 ? PortalKind::Edge : PortalKind::Door,
                               .at = feetOn(which == 0 ? 94.5f : 92.0f, g), .to = next, .toPortal = 0});
        def.spawns = {{r.a, runOn(8, 40, g), 3}, {r.b, runOn(44, 90, g), 3}, {r.c, runOn(30, 42, 17), 1},
                      {r.a, runOn(50, 62, 20), 1}, {r.b, runOn(70, 80, 17), 1}, {r.c, runOn(14, 24, 20), 1}};
    } else {
        def.portals.push_back({.kind = PortalKind::Door, .at = feetOn(22.0f, 19), .to = MapId::Town,
                               .toPortal = static_cast<int>(def.region) - 1, .label = "Hearthvale", .afterBoss = true});
        def.arena = Arena{.boss = r.boss, .bossFeet = feetOn(31.0f, 19), .bounds = {tx(1.0f), 0.0f, tx(42.0f), 19 * kTile}};
    }
    def.dungeon = which >= 2;
}

void build(MapDef& def, const RegionMaps& r, const std::array<std::string_view, 4>& names)
{
    const int which = def.id == r.first ? 0 : def.id == r.second ? 1 : def.id == r.deep ? 2 : 3;
    def.name = names[static_cast<std::size_t>(which)];
    layout(def, r, which);
}

} // namespace

void buildMines(MapDef& def)
{
    def.region = Region::Mines;
    build(def,
          {MapId::Mines1, MapId::Mines2, MapId::MinesDeep, MapId::MinesArena, BossId::Rumblerock, SpeciesId::Pebblet,
           SpeciesId::EmberMochi, SpeciesId::Flitter},
          {"Glimmer Gate", "Lanternway", "The Deep Shaft", "Rumblerock's Hollow"});
}

void buildMarsh(MapDef& def)
{
    def.region = Region::Marsh;
    build(def,
          {MapId::Marsh1, MapId::Marsh2, MapId::MarshDeep, MapId::MarshArena, BossId::Murkshell, SpeciesId::Turbanshell,
           SpeciesId::BogMochi, SpeciesId::ReedFlitter},
          {"Reedwater Banks", "Mallow Mire", "The Sunken Grotto", "Murkshell's Pool"});
}

void buildRuins(MapDef& def)
{
    def.region = Region::Ruins;
    build(def,
          {MapId::Ruins1, MapId::Ruins2, MapId::RuinsDeep, MapId::RuinsArena, BossId::Warden, SpeciesId::RunePebblet,
           SpeciesId::OldStump, SpeciesId::Wisp},
          {"Sunspire Steps", "Hall of Pillars", "The Vault", "The Warden's Court"});
}

void buildPeak(MapDef& def)
{
    def.region = Region::Peak;
    build(def,
          {MapId::Peak1, MapId::Peak2, MapId::PeakDeep, MapId::PeakArena, BossId::Hush, SpeciesId::SnowPudgy,
           SpeciesId::IcePebblet, SpeciesId::FrostWisp},
          {"Snowcap Trail", "Frostwind Ridge", "The Ice Caves", "The Crown of Frost"});
}

} // namespace wayfarer::game
