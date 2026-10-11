// Hearthvale: the hub town. From the west gate: the waystone plaza at the
// town's edge, the elder's townhouse, the inn, the market square with its
// fountain and stalls under bunting, Pippa's shop, a cottage, Brann's forge,
// a tree-top hut, a townhouse and the bakery, then Lumi's observatory on the
// hill - with lamps, trees, flower beds, fences, crates, a lookout and a back
// row of houses and trees on a parallax layer.

#include "maps.hpp"

namespace wayfarer::game {

namespace {

// helpers

constexpr int kColumns = 128;
constexpr int kRows = 26;
constexpr int kGroundRow = 21;

// A house variant: the style, a color palette (0: the style's own colors)
// and a sign icon (0: the style's default sign).
constexpr int house(const int style, const int palette = 0, const int signIcon = 0)
{
    return style | palette << 4 | signIcon << 8;
}

} // namespace

// House styles (Deco::variant & 15 for DecoKind::House): 0 inn, 1 shop,
// 2 cottage, 3 tree hut, 4 forge, 5 bakery, 6 observatory, 7 townhouse.

void buildTown(MapDef& def)
{
    def.name = "Hearthvale";
    def.region = Region::Town;
    MapBuilder b{kColumns, kRows};
    b.ground(0, kColumns - 1, kGroundRow);
    // Lumi's hill at the east end.
    b.rampUp(108, kGroundRow, 2);
    b.ground(110, kColumns - 1, kGroundRow - 2);
    // A lookout between the elder's house and the inn.
    b.foothold(37, 42, 17);
    b.ladder(36, 17, kGroundRow - 1);
    def.text = b.take();

    const float g = static_cast<float>(kGroundRow) * kTile;
    const float hill = static_cast<float>(kGroundRow - 2) * kTile;
    // The waystone plaza by the west gate.
    for (int r = 1; r <= 5; ++r) {
        const auto region = static_cast<Region>(r);
        def.portals.push_back({.kind = PortalKind::Waystone, .at = feetOn(8.5f + static_cast<float>(r - 1) * 3.0f, kGroundRow),
                               .to = regionEntry(region), .toPortal = 0, .chapter = r, .label = regionName(region)});
    }

    using K = DecoKind;
    def.decos = {
        // The gate and the plaza.
        {K::Arch, {tx(2.5f), g}, 0, 1.0f, false},
        {K::Banner, {tx(6.0f), g}, 0, 1.0f, false},
        {K::Banner, {tx(23.0f), g}, 1, 1.0f, false},
        {K::Lamp, {tx(24.8f), g}, 0, 1.0f, false},
        {K::Flowers, {tx(10.0f), g}, 0, 0.8f, true},
        {K::Flowers, {tx(16.0f), g}, 1, 0.8f, true},
        {K::Flowers, {tx(22.0f), g}, 0, 0.8f, true},
        {K::Grass, {tx(13.0f), g}, 0, 1.0f, true},
        // The elder's townhouse with a leaf sign.
        {K::House, {tx(30.5f), g}, house(7, 3, 6), 1.0f, false},
        {K::FlowerPot, {tx(26.6f), g}, 1, 1.0f, false},
        {K::Bush, {tx(35.0f), g}, 1, 0.9f, true},
        // The lookout with its tree.
        {K::Tree, {tx(40.0f), 17.0f * kTile + 2.0f}, 1, 0.8f, false},
        {K::Lamp, {tx(43.0f), g}, 0, 1.0f, false},
        // The inn.
        {K::House, {tx(48.5f), g}, house(0), 1.0f, false},
        {K::Barrel, {tx(53.6f), g}, 0, 1.0f, false},
        {K::Barrel, {tx(54.5f), g}, 1, 0.85f, false},
        // The market square: bunting from the inn to the shop, a fountain,
        // two stalls and a bench.
        {K::Bunting, {tx(60.5f), g - 200.0f}, 0, 1.75f, false},
        {K::Stall, {tx(57.0f), g}, 0, 1.0f, false},
        {K::Fountain, {tx(62.0f), g}, 0, 1.0f, false},
        {K::Stall, {tx(66.5f), g}, 1, 1.0f, false},
        {K::Flowers, {tx(59.4f), g}, 1, 0.8f, true},
        {K::Flowers, {tx(64.6f), g}, 0, 0.8f, true},
        // Pippa's shop.
        {K::House, {tx(73.0f), g}, house(1), 1.0f, false},
        {K::Lamp, {tx(78.6f), g}, 0, 1.0f, false},
        // A yellow-and-green cottage behind a white fence.
        {K::House, {tx(83.0f), g}, house(2, 2), 0.9f, false},
        {K::Fence, {tx(83.0f), g}, 0, 1.4f, true},
        {K::Bunting, {tx(87.8f), g - 120.0f}, 1, 0.8f, false},
        // Brann's forge.
        {K::House, {tx(93.5f), g}, house(4), 1.0f, false},
        {K::Anvil, {tx(89.0f), g}, 0, 1.0f, false},
        {K::Crate, {tx(98.4f), g}, 0, 1.0f, false},
        {K::Crate, {tx(98.9f), g - 75.0f}, 0, 0.8f, false},
        {K::Barrel, {tx(99.6f), g}, 0, 1.0f, false},
        // The tree-top hut and the bakery.
        {K::House, {tx(103.5f), g}, house(3), 0.95f, false},
        {K::Lamp, {tx(108.2f), g}, 0, 1.0f, false},
        {K::Tree, {tx(108.8f), g}, 0, 1.0f, false},
        // Lumi's hill: a townhouse, the observatory, a fence.
        {K::House, {tx(114.0f), hill}, house(5), 0.95f, false},
        {K::Bush, {tx(110.5f), hill}, 1, 1.0f, true},
        {K::House, {tx(121.5f), hill}, house(6), 1.0f, false},
        {K::Flowers, {tx(117.6f), hill}, 1, 0.8f, true},
        {K::Lamp, {tx(111.2f), hill}, 0, 1.0f, false},
        {K::Bench, {tx(118.0f), hill}, 0, 1.0f, false},
        {K::FlowerPot, {tx(125.0f), hill}, 1, 1.0f, false},
        {K::Fence, {tx(126.0f), hill}, 0, 1.0f, false},
        {K::Grass, {tx(34.0f), g}, 0, 1.0f, true},
        {K::Grass, {tx(70.0f), g}, 0, 1.0f, true},
        {K::Grass, {tx(101.0f), g}, 0, 1.0f, true},
        {K::Grass, {tx(119.0f), hill}, 0, 1.0f, true},
    };

    // The back row: houses and trees further up the valley.
    const float back = g - 70.0f;
    def.skyline = {
        {K::Tree, {tx(1.0f), back}, 0, 0.75f, false},
        {K::House, {tx(7.0f), back}, house(7, 5), 0.6f, false},
        {K::House, {tx(15.0f), back}, house(2, 4), 0.6f, false},
        {K::Pine, {tx(21.0f), back}, 0, 0.7f, false},
        {K::House, {tx(27.0f), back}, house(7, 2), 0.62f, false},
        {K::House, {tx(35.0f), back}, house(5, 6), 0.58f, false},
        {K::Tree, {tx(42.0f), back}, 1, 0.75f, false},
        {K::House, {tx(48.0f), back}, house(7, 7), 0.64f, false},
        {K::House, {tx(56.0f), back}, house(2, 1), 0.6f, false},
        {K::House, {tx(64.0f), back}, house(7, 4), 0.62f, false},
        {K::Tree, {tx(70.0f), back}, 0, 0.75f, false},
        {K::House, {tx(76.0f), back}, house(0, 8), 0.58f, false},
        {K::House, {tx(85.0f), back}, house(7, 6), 0.6f, false},
        {K::Pine, {tx(91.0f), back}, 0, 0.7f, false},
        {K::House, {tx(97.0f), back}, house(2, 3), 0.6f, false},
        {K::House, {tx(105.0f), back}, house(7, 1), 0.62f, false},
        {K::Tree, {tx(112.0f), back}, 1, 0.75f, false},
        {K::House, {tx(118.0f), back}, house(7, 8), 0.6f, false},
        {K::Tree, {tx(126.0f), back}, 0, 0.75f, false},
    };

    def.npcs = {
        {NpcId::Dorran, feetOn(26.0f, kGroundRow), feetOn(14.5f, kGroundRow)},
        {NpcId::Elder, feetOn(34.0f, kGroundRow), feetOn(46.0f, kGroundRow)},
        {NpcId::Pippa, feetOn(68.8f, kGroundRow), feetOn(68.8f, kGroundRow)},
        {NpcId::Brann, feetOn(90.4f, kGroundRow), feetOn(90.4f, kGroundRow)},
        {NpcId::Nell, feetOn(115.5f, kGroundRow - 2), {-1.0f, 0.0f}},
        {NpcId::Lumi, {-1.0f, 0.0f}, feetOn(118.5f, kGroundRow - 2)},
    };
}

} // namespace wayfarer::game
