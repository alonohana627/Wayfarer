#pragma once

// The world's maps: Hearthvale and five regions of two fields, a dungeon and
// a guardian's arena each. A map is a tile layer built from a few calls
// (ground with hills, floating footholds, ropes and ladders) plus what stands
// on it: portals (walked through at a map's edge, entered with up at doors
// and waystones), monster spawns on a run of floor, decorations (houses,
// trees, lamps, props - drawn by the app), NPC places by day and night and
// the guardian's arena.

#include "monsters.hpp"
#include "rules.hpp"

#include <gamelib/core/math.hpp>
#include <gamelib/core/tile_collision.hpp>
#include <gamelib/core/tile_map.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace wayfarer::game {

enum class MapId : std::uint8_t {
    Town,
    Forest1, Forest2, ForestDeep, ForestArena,
    Mines1, Mines2, MinesDeep, MinesArena,
    Marsh1, Marsh2, MarshDeep, MarshArena,
    Ruins1, Ruins2, RuinsDeep, RuinsArena,
    Peak1, Peak2, PeakDeep, PeakArena,
    Count
};
inline constexpr int kMapCount = static_cast<int>(MapId::Count);

// Tiles (the level text's characters).
inline constexpr char kEmpty = ' ';
inline constexpr char kGround = '#';
inline constexpr char kFoothold = '=';
inline constexpr char kRope = '|';
inline constexpr char kLadder = 'H';
inline constexpr char kRampUp = '/';   // rises to the right
inline constexpr char kRampDown = '\\'; // rises to the left

[[nodiscard]] const gamelib::core::TileShapes& tileShapes();

enum class PortalKind : std::uint8_t {
    Edge,     // walked into at the map's left or right end
    Door,     // up at the doorway (a cave mouth, a gate)
    Waystone, // Hearthvale's stones to the regions (open by chapter)
};

struct Portal {
    PortalKind kind = PortalKind::Door;
    gamelib::core::Vec2 at; // feet point where the hero arrives and where it is entered
    MapId to = MapId::Town;
    int toPortal = 0;       // index of the arrival portal in the target map
    int chapter = 0;        // closed until this chapter (waystones), 0 = always open
    std::string_view label; // shown over it
    bool afterBoss = false; // open once the map's guardian is beaten (an arena's way home)
};

struct SpawnDef {
    SpeciesId species = SpeciesId::Bellcap;
    gamelib::core::Rect run; // floor run in world units: x range to patrol, bottom = the floor's top
    int count = 1;
};

enum class DecoKind : std::uint8_t {
    House, Tree, Pine, BigShroom, Bush, Flowers, Grass, Rock, Crystal, Lamp, Fence, Sign, Barrel, Crate, FlowerPot,
    Well, Bench, Cart, Reeds, Column, Arch, Torch, Waystone, Snowpile, Lantern, Banner, Anvil, Stall, Count
};

struct Deco {
    DecoKind kind = DecoKind::Tree;
    gamelib::core::Vec2 at; // ground point (bottom middle)
    int variant = 0;        // style (house style, tree kind, ...)
    float scale = 1.0f;
    bool front = false;     // drawn in front of the characters
};

enum class NpcId : std::uint8_t { Elder, Pippa, Brann, Lumi, Dorran, Nell, Count };
inline constexpr int kNpcCount = static_cast<int>(NpcId::Count);

struct NpcPlace {
    NpcId npc = NpcId::Elder;
    gamelib::core::Vec2 day;   // feet by day (x < 0: indoors)
    gamelib::core::Vec2 night; // feet at night (x < 0: indoors)
};

enum class BossId : std::uint8_t { Grandcap, Rumblerock, Murkshell, Warden, Hush, Count };
inline constexpr int kBossCount = static_cast<int>(BossId::Count);

struct Arena {
    BossId boss = BossId::Grandcap;
    gamelib::core::Vec2 bossFeet;
    gamelib::core::Rect bounds; // where the boss moves
};

struct MapDef {
    MapId id = MapId::Town;
    std::string_view name;
    Region region = Region::Town;
    bool dungeon = false;  // darker, lantern-lit
    int columns = 0;
    int rows = 0;
    std::vector<std::string> text;
    std::vector<Portal> portals;
    std::vector<SpawnDef> spawns;
    std::vector<Deco> decos;
    std::vector<NpcPlace> npcs;
    std::optional<Arena> arena;
    int eliteChance = 6;   // percent per spawn
    gamelib::core::TileMap tiles{1, 1, kTile};

    [[nodiscard]] gamelib::core::Rect bounds() const
    {
        return {0.0f, 0.0f, static_cast<float>(columns) * kTile, static_cast<float>(rows) * kTile};
    }
};

[[nodiscard]] const MapDef& mapDef(MapId id);
[[nodiscard]] MapId regionEntry(Region region); // the first field of a region
[[nodiscard]] int regionChapter(Region region);  // 1..5

// Builds a map's tile text from a few calls.
class MapBuilder {
public:
    MapBuilder(int columns, int rows);

    // Solid ground from row `top` down to the bottom between two columns (inclusive).
    MapBuilder& ground(int x0, int x1, int top);
    // A solid block (a cliff, a pillar).
    MapBuilder& block(int x0, int x1, int top, int bottom);
    // A floating one-way foothold on a row.
    MapBuilder& foothold(int x0, int x1, int row);
    // A rope or a ladder from row top to row bottom (inclusive).
    MapBuilder& rope(int x, int top, int bottom);
    MapBuilder& ladder(int x, int top, int bottom);
    // A 45 degree ramp: `length` tiles rising from ground row `fromRow` to the right (up) or left (down).
    MapBuilder& rampUp(int x, int fromRow, int length);
    MapBuilder& rampDown(int x, int fromRow, int length);
    MapBuilder& clear(int x0, int x1, int top, int bottom);

    [[nodiscard]] std::vector<std::string> take() { return std::move(m_rows); }
    [[nodiscard]] char at(int x, int y) const;

private:
    void set(int x, int y, char c);
    int m_width;
    int m_height;
    std::vector<std::string> m_rows;
};

// World points for map authoring: a tile's top-left, a feet point on top of a row.
[[nodiscard]] constexpr float tx(const float column) { return column * kTile; }
[[nodiscard]] constexpr gamelib::core::Vec2 feetOn(const float column, const int row)
{
    return {column * kTile, static_cast<float>(row) * kTile};
}
// A floor run between columns (inclusive) whose top is row `row`.
[[nodiscard]] constexpr gamelib::core::Rect runOn(const int x0, const int x1, const int row)
{
    return {static_cast<float>(x0) * kTile, static_cast<float>(row) * kTile - 4.0f, static_cast<float>(x1 - x0 + 1) * kTile, 4.0f};
}

// Region map makers (maps_*.cpp).
void buildTown(MapDef& def);
void buildForest(MapDef& def);
void buildMines(MapDef& def);
void buildMarsh(MapDef& def);
void buildRuins(MapDef& def);
void buildPeak(MapDef& def);

} // namespace wayfarer::game
