// Hearthvale: the hub town. A long street of distinct houses - the elder's
// cottage, the inn, the plaza with the five waystones, Pippa's shop, Brann's
// forge, the bakery and Lumi's observatory on the hill - with lamps, trees,
// flowers and a few wooden footholds.

#include "maps.hpp"

namespace wayfarer::game {

namespace {

// helpers

constexpr int kColumns = 128;
constexpr int kRows = 26;
constexpr int kGroundRow = 21;

} // namespace

// House styles (Deco::variant for DecoKind::House).
// 0 inn, 1 shop, 2 cottage, 3 tree hut, 4 forge, 5 bakery, 6 observatory.

void buildTown(MapDef& def)
{
    def.name = "Hearthvale";
    def.region = Region::Town;
    MapBuilder b{kColumns, kRows};
    b.ground(0, kColumns - 1, kGroundRow);
    // Lumi's hill at the east end.
    b.rampUp(108, kGroundRow, 2);
    b.ground(110, kColumns - 1, kGroundRow - 2);
    // A lookout over the plaza.
    b.foothold(38, 43, 17);
    b.ladder(37, 17, kGroundRow - 1);
    def.text = b.take();

    const float g = static_cast<float>(kGroundRow);
    for (int r = 1; r <= 5; ++r) {
        const auto region = static_cast<Region>(r);
        def.portals.push_back({.kind = PortalKind::Waystone, .at = feetOn(52.5f + static_cast<float>(r - 1) * 3.0f, kGroundRow),
                               .to = regionEntry(region), .toPortal = 0, .chapter = r, .label = regionName(region)});
    }

    using K = DecoKind;
    def.decos = {
        {K::Arch, {tx(3.5f), g * kTile}, 0, 1.0f, false},
        {K::Fence, {tx(7.0f), g * kTile}, 0, 1.0f, false},
        {K::House, {tx(13.5f), g * kTile}, 2, 1.0f, false},
        {K::Flowers, {tx(19.0f), g * kTile}, 0, 1.0f, true},
        {K::House, {tx(26.0f), g * kTile}, 0, 1.0f, false},
        {K::Barrel, {tx(31.2f), g * kTile}, 0, 1.0f, false},
        {K::Tree, {tx(35.0f), g * kTile}, 0, 1.0f, false},
        {K::Bush, {tx(37.5f), g * kTile}, 0, 1.0f, true},
        {K::Lamp, {tx(40.0f), g * kTile}, 0, 1.0f, false},
        {K::Well, {tx(45.5f), g * kTile}, 0, 1.0f, false},
        {K::Bench, {tx(48.5f), g * kTile}, 0, 1.0f, false},
        {K::Flowers, {tx(50.5f), g * kTile}, 1, 1.0f, true},
        {K::Banner, {tx(51.0f), g * kTile}, 0, 1.0f, false},
        {K::Banner, {tx(66.0f), g * kTile}, 1, 1.0f, false},
        {K::Lamp, {tx(68.5f), g * kTile}, 0, 1.0f, false},
        {K::House, {tx(75.5f), g * kTile}, 1, 1.0f, false},
        {K::Crate, {tx(81.5f), g * kTile}, 0, 1.0f, false},
        {K::Barrel, {tx(82.6f), g * kTile}, 1, 1.0f, false},
        {K::House, {tx(89.0f), g * kTile}, 4, 1.0f, false},
        {K::Anvil, {tx(85.2f), g * kTile}, 0, 1.0f, false},
        {K::Lamp, {tx(93.0f), g * kTile}, 0, 1.0f, false},
        {K::House, {tx(103.0f), g * kTile}, 5, 1.0f, false},
        {K::FlowerPot, {tx(97.5f), g * kTile}, 0, 1.0f, false},
        {K::Tree, {tx(41.5f), 17.0f * kTile + 2.0f}, 1, 0.8f, false},
        {K::Tree, {tx(107.5f), g * kTile}, 1, 1.1f, false},
        {K::Bush, {tx(111.0f), (g - 2.0f) * kTile}, 1, 1.0f, true},
        {K::House, {tx(119.5f), (g - 2.0f) * kTile}, 6, 1.0f, false},
        {K::Fence, {tx(125.5f), (g - 2.0f) * kTile}, 0, 1.0f, false},
        {K::Grass, {tx(9.0f), g * kTile}, 0, 1.0f, true},
        {K::Grass, {tx(43.0f), g * kTile}, 0, 1.0f, true},
        {K::Grass, {tx(71.0f), g * kTile}, 0, 1.0f, true},
        {K::Grass, {tx(114.0f), (g - 2.0f) * kTile}, 0, 1.0f, true},
    };

    def.npcs = {
        {NpcId::Dorran, feetOn(7.0f, kGroundRow), feetOn(60.0f, kGroundRow)},
        {NpcId::Elder, feetOn(18.5f, kGroundRow), feetOn(31.5f, kGroundRow)},
        {NpcId::Pippa, feetOn(72.0f, kGroundRow), feetOn(72.0f, kGroundRow)},
        {NpcId::Brann, feetOn(86.8f, kGroundRow), feetOn(86.8f, kGroundRow)},
        {NpcId::Nell, feetOn(99.0f, kGroundRow), {-1.0f, 0.0f}},
        {NpcId::Lumi, {-1.0f, 0.0f}, feetOn(115.0f, kGroundRow - 2)},
    };
}

} // namespace wayfarer::game
