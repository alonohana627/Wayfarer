#include "maps.hpp"

#include <array>
#include <stdexcept>
#include <string_view>

namespace wayfarer::game {

namespace {

// helpers

std::array<MapDef, kMapCount> buildAll()
{
    std::array<MapDef, kMapCount> maps;
    for (int i = 0; i < kMapCount; ++i) {
        maps[static_cast<std::size_t>(i)].id = static_cast<MapId>(i);
    }
    buildTown(maps[static_cast<std::size_t>(MapId::Town)]);
    for (const MapId id : {MapId::Forest1, MapId::Forest2, MapId::ForestDeep, MapId::ForestArena}) {
        buildForest(maps[static_cast<std::size_t>(id)]);
    }
    for (const MapId id : {MapId::Mines1, MapId::Mines2, MapId::MinesDeep, MapId::MinesArena}) {
        buildMines(maps[static_cast<std::size_t>(id)]);
    }
    for (const MapId id : {MapId::Marsh1, MapId::Marsh2, MapId::MarshDeep, MapId::MarshArena}) {
        buildMarsh(maps[static_cast<std::size_t>(id)]);
    }
    for (const MapId id : {MapId::Ruins1, MapId::Ruins2, MapId::RuinsDeep, MapId::RuinsArena}) {
        buildRuins(maps[static_cast<std::size_t>(id)]);
    }
    for (const MapId id : {MapId::Peak1, MapId::Peak2, MapId::PeakDeep, MapId::PeakArena}) {
        buildPeak(maps[static_cast<std::size_t>(id)]);
    }
    for (MapDef& m : maps) {
        if (m.text.empty()) {
            throw std::logic_error("map without tiles: " + std::to_string(static_cast<int>(m.id)));
        }
        m.rows = static_cast<int>(m.text.size());
        m.columns = static_cast<int>(m.text.front().size());
        std::vector<std::string_view> views(m.text.begin(), m.text.end());
        const std::array<gamelib::core::LayerMapping, 1> layers{gamelib::core::charTile};
        m.tiles = gamelib::core::TileMap::fromText(views, kTile, layers);
    }
    return maps;
}

} // namespace

const gamelib::core::TileShapes& tileShapes()
{
    using gamelib::core::TileShape;
    // Ropes are climbed like ladders: their top cell is a floor, so climbing
    // up a rope ends standing on the foothold it hangs from.
    static const gamelib::core::TileShapes shapes{{kGround, TileShape::Solid},     {kFoothold, TileShape::OneWay},
                                                  {kRope, TileShape::Ladder},      {kLadder, TileShape::Ladder},
                                                  {kRampUp, TileShape::RampUpRight}, {kRampDown, TileShape::RampUpLeft}};
    return shapes;
}

const MapDef& mapDef(const MapId id)
{
    static const std::array<MapDef, kMapCount> maps = buildAll();
    return maps[static_cast<std::size_t>(id)];
}

MapId regionEntry(const Region region)
{
    switch (region) {
    case Region::Town: return MapId::Town;
    case Region::Forest: return MapId::Forest1;
    case Region::Mines: return MapId::Mines1;
    case Region::Marsh: return MapId::Marsh1;
    case Region::Ruins: return MapId::Ruins1;
    case Region::Peak: return MapId::Peak1;
    case Region::Count: break;
    }
    return MapId::Town;
}

int regionChapter(const Region region)
{
    return std::max(static_cast<int>(region), 1);
}

MapBuilder::MapBuilder(const int columns, const int rows)
    : m_width{columns}
    , m_height{rows}
    , m_rows(static_cast<std::size_t>(rows), std::string(static_cast<std::size_t>(columns), kEmpty))
{
}

void MapBuilder::set(const int x, const int y, const char c)
{
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        m_rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = c;
    }
}

char MapBuilder::at(const int x, const int y) const
{
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return kEmpty;
    }
    return m_rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
}

MapBuilder& MapBuilder::ground(const int x0, const int x1, const int top)
{
    return block(x0, x1, top, m_height - 1);
}

MapBuilder& MapBuilder::block(const int x0, const int x1, const int top, const int bottom)
{
    for (int y = top; y <= bottom; ++y) {
        for (int x = x0; x <= x1; ++x) {
            set(x, y, kGround);
        }
    }
    return *this;
}

MapBuilder& MapBuilder::clear(const int x0, const int x1, const int top, const int bottom)
{
    for (int y = top; y <= bottom; ++y) {
        for (int x = x0; x <= x1; ++x) {
            set(x, y, kEmpty);
        }
    }
    return *this;
}

MapBuilder& MapBuilder::foothold(const int x0, const int x1, const int row)
{
    for (int x = x0; x <= x1; ++x) {
        set(x, row, kFoothold);
    }
    return *this;
}

MapBuilder& MapBuilder::rope(const int x, const int top, const int bottom)
{
    for (int y = top; y <= bottom; ++y) {
        set(x, y, kRope);
    }
    return *this;
}

MapBuilder& MapBuilder::ladder(const int x, const int top, const int bottom)
{
    for (int y = top; y <= bottom; ++y) {
        set(x, y, kLadder);
    }
    return *this;
}

MapBuilder& MapBuilder::rampUp(const int x, const int fromRow, const int length)
{
    // Rising to the right: each ramp tile sits on a solid column one higher than the last.
    for (int i = 0; i < length; ++i) {
        const int row = fromRow - 1 - i;
        set(x + i, row, kRampUp);
        for (int y = row + 1; y < m_height; ++y) {
            set(x + i, y, kGround);
        }
    }
    return *this;
}

MapBuilder& MapBuilder::rampDown(const int x, const int fromRow, const int length)
{
    // Falling to the right: the mirror of rampUp, starting `length` rows above fromRow.
    for (int i = 0; i < length; ++i) {
        const int row = fromRow - length + i;
        set(x + i, row, kRampDown);
        for (int y = row + 1; y < m_height; ++y) {
            set(x + i, y, kGround);
        }
    }
    return *this;
}

} // namespace wayfarer::game
