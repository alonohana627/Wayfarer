#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_building.glsl"

// Sample: four distinct storybook houses in the MapleStory-style chibi look,
// from gamelib/anime_building.glsl - an inn (two floors, timber framing,
// tiled roof, chimney smoke, shuttered windows with flower boxes, a mug
// sign, lanterns), a shop (stone ground floor, striped awning over the
// display window, a potion sign, a dormer, barrels and a crate), a cottage
// (puffy thatched roof, stone walls, round window, flower pots) and a
// tree-top hut (plank walls on a deck in the branches, a leaf roof, a rope
// railing). Lit windows and lanterns glow at night.
//
// Design space: 128 x 128 units over the quad height, the ground on
// y = +58; a square quad holds every style. anime_samples.hpp packs the fields.
//   color   wall color          color2  roof color
//   params  x style (0 inn, 1 shop, 2 cottage, 3 tree-top hut), y night 0..1
//           (windows and lanterns lit), z wind -1..1 (smoke drift),
//           w unused
//   bits.x  0-3 sign icon (kAnimeIcon*), 8-15 seed (varies tiles, stones,
//           flowers)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

AnimeCanvas C;

void main()
{
    SpriteData s = sprite;
    float k = s.size.y / 128.0;
    vec2 p = spriteLocal(inUv) / k;
    int style = int(s.params.x + 0.5);
    float night = clamp(s.params.y, 0.0, 1.0);
    float wind = s.params.z;
    int icon = int(s.bits.x & 15u);
    float seed = float((s.bits.x >> 8) & 0xffu);
    float time = s.time;
    vec3 wallColor = s.color.rgb;
    vec3 roofColor = s.color2.rgb;

    C = animeBegin(p, 2.4);
    C.shadeDepth = 5.0;
    AnimeMaterial wall = animeMatte(wallColor);
    AnimeMaterial roof = animeMaterial(roofColor);
    AnimeMaterial timber = animeMaterial(animeHex(0x8A5A3Cu));
    AnimeMaterial frame = animeMaterial(animeHex(0xF4EEE0u));
    AnimeMaterial stone = animeMatte(animeHex(0xB4B0C4u));
    AnimeMaterial shutter = animeMaterial(animeHex(0x5CA8C8u));
    AnimeMaterial doorWood = animeMaterial(animeHex(0xA0683Au));
    float ground = 58.0;

    if (style == 0) {
        // ---- The inn.
        float chimney = sdRoundBox(p - vec2(27.0, -34.0), vec2(6.0, 11.0), 2.0);
        animePart(C, chimney, stone);
        animeStones(C, p, chimney, stone, vec2(6.0, 4.0), seed);
        animePart(C, sdRoundBox(p - vec2(27.0, -45.0), vec2(7.5, 2.0), 1.5), stone);
        float wallD = sdRoundBox(p - vec2(0.0, 22.0), vec2(40.0, 35.0), 3.0);
        animePart(C, wallD, wall);
        animePlaster(C, p, wallD, wall, seed);
        animeTimber(C, p, max(wallD + 1.0, p.y - 34.0), timber, 20.0, -11.0, 34.0, 1.0, seed);
        float base = max(wallD, -(p.y - 38.0));
        animePartNoLine(C, base, stone);
        animeStones(C, p, base, stone, vec2(9.0, 5.0), seed + 1.0);
        animeLine(C, max(abs(p.y - 38.0), wallD), stone.ink);
        // The roof and its shadow under the eaves.
        float roofD = sdRoundRoof(p, vec2(0.0, -8.0), 52.0, 40.0, 0.28, 6.0);
        animeCastShadow(C, wallD, sdRoundRoof(p - vec2(2.0, 6.0), vec2(0.0, -8.0), 52.0, 40.0, 0.28, 6.0), wall);
        animePart(C, roofD, roof);
        animeRoofTiles(C, p, roofD, roof, 6.0, 8.0, seed);
        animeGloss(C, p, roofD, vec2(-20.0, -34.0), vec2(14.0, 5.0), 0.25);
        // Upper windows with shutters and flowers; the ground floor door,
        // a round window and lanterns.
        animeWindow(C, p, vec2(-20.0, 20.0), 6.5, 16.0, night, 5.0, frame, shutter, true, seed);
        animeWindow(C, p, vec2(20.0, 20.0), 6.5, 16.0, night, 5.0, frame, shutter, true, seed + 3.0);
        animeDoor(C, p, vec2(-8.0, ground - 1.5), 9.0, 22.0, doorWood, frame, night);
        animeRoundWindow(C, p, vec2(22.0, 46.0), 6.0, night, frame);
        animeLantern(C, p, vec2(-25.0, 30.0), 3.2, night, time);
        animeLantern(C, p, vec2(9.0, 30.0), 3.2, night, time + 1.3);
        // The sign on a bracket out of the right wall.
        animePart(C, sdSegment(p, vec2(39.0, 2.0), vec2(46.0, 2.0), 1.1), animeMetal(animeHex(0x5E6070u)));
        animeSign(C, p, vec2(50.0, 2.0), 8.5, icon, animeMaterial(animeHex(0xE9C48Au)), time);
        animeSmoke(C, p, vec2(27.0 + wind, -48.0), 4.0, time, wind);
    } else if (style == 1) {
        // ---- The shop.
        float wallD = sdRoundBox(p - vec2(0.0, 28.0), vec2(42.0, 29.0), 3.0);
        animePart(C, wallD, wall);
        animePlaster(C, p, wallD, wall, seed);
        float base = max(wallD, -(p.y - 30.0));
        animePartNoLine(C, base, stone);
        animeStones(C, p, base, stone, vec2(10.0, 6.0), seed);
        animeLine(C, max(abs(p.y - 30.0), wallD), stone.ink);
        float roofD = sdRoundRoof(p, vec2(0.0, 0.0), 52.0, 34.0, 0.35, 7.0);
        // A dormer with a round window in the roof.
        float dormer = sdArch(p, vec2(-14.0, -4.0), 9.0, 22.0);
        animeCastShadow(C, wallD, sdRoundRoof(p - vec2(2.0, 6.0), vec2(0.0, 0.0), 52.0, 34.0, 0.35, 7.0), wall);
        animePart(C, roofD, roof);
        animeRoofTiles(C, p, roofD, roof, 5.5, 7.5, seed);
        animePart(C, dormer, wall);
        animeRoundWindow(C, p, vec2(-14.0, -10.0), 4.5, night, frame);
        // The display window under a striped awning, goods on its shelf.
        vec2 win = vec2(16.0, 36.0);
        float glassFrame = sdRoundBox(p - win, vec2(18.0, 11.0), 3.0);
        if (night > 0.0) {
            C.color = over(vec4(kAnimeWindowLight, glowFalloff(glassFrame, 10.0) * night * 0.5), C.color);
        }
        animePart(C, glassFrame, frame);
        float glass = sdRoundBox(p - win, vec2(16.0, 9.0), 2.0);
        animeFlat(C, glass, animeGlass(p, win, 9.0, night));
        for (int i = 0; i < 4; ++i) {
            vec2 b = win + vec2(-11.0 + float(i) * 7.3, 5.0);
            vec3 potion = animePalette(i == 0 ? 2u : (i == 1 ? 8u : (i == 2 ? 5u : 10u)));
            float bottle = min(sdCircle(p - b, 2.8), sdRoundBox(p - b - vec2(0.0, -3.5), vec2(1.0, 1.5), 0.4));
            animePart(C, max(bottle, glass), animeMaterial(potion));
        }
        animeLine(C, max(abs(p.y - win.y - 8.0), glass), frame.ink);
        animeAwning(C, p, vec2(16.0, 19.0), 21.0, 10.0, animeHex(0xEC5662u), animeHex(0xF7F1E3u), 7.0);
        animeDoor(C, p, vec2(-24.0, ground - 1.5), 8.5, 21.0, doorWood, frame, night);
        animePart(C, sdSegment(p, vec2(-41.0, 14.0), vec2(-46.0, 14.0), 1.1), animeMetal(animeHex(0x5E6070u)));
        animeSign(C, p, vec2(-50.0, 14.0), 8.0, icon, animeMaterial(animeHex(0xF4EEE0u)), time);
        animeBarrel(C, p, vec2(-47.0, ground), 6.5, 15.0);
        animeCrate(C, p, vec2(45.0, ground), 13.0);
        animeLantern(C, p, vec2(-8.0, 18.0), 3.0, night, time);
    } else if (style == 2) {
        // ---- The cottage.
        float chimney = sdRoundBox(p - vec2(-22.0, -20.0), vec2(5.5, 14.0), 2.0);
        animePart(C, chimney, stone);
        animeStones(C, p, chimney, stone, vec2(5.5, 4.0), seed);
        float wallD = sdRoundBox(p - vec2(0.0, 33.0), vec2(36.0, 24.0), 5.0);
        animePart(C, wallD, wall);
        animeStones(C, p, wallD, wall, vec2(11.0, 6.5), seed);
        // A puffy thatched roof with a deep overhang.
        float roofD = sdRoundRoof(p, vec2(0.0, 14.0), 50.0, 44.0, 0.12, 13.0);
        roofD = animeSmoothUnion(roofD, sdEllipse(p - vec2(0.0, -26.0), vec2(14.0, 8.0)), 6.0);
        animeCastShadow(C, wallD, sdRoundRoof(p - vec2(2.0, 6.0), vec2(0.0, 14.0), 50.0, 44.0, 0.12, 13.0), wall);
        animePart(C, roofD, roof);
        animeThatch(C, p, roofD, roof, 7.0, seed);
        animeGloss(C, p, roofD, vec2(-16.0, -14.0), vec2(14.0, 6.0), 0.25);
        animeRoundWindow(C, p, vec2(-2.0, -6.0), 5.0, night, frame);
        animeWindow(C, p, vec2(-19.0, 44.0), 6.0, 14.0, night, 4.5, frame, animeMaterial(animeHex(0xEC8A5Au)), true, seed);
        animeDoor(C, p, vec2(14.0, ground - 1.5), 8.0, 20.0, doorWood, frame, night);
        animeFlowerPot(C, p, vec2(30.0, ground), 6.0, animePalette(11u));
        animeFlowerPot(C, p, vec2(-1.0, ground), 5.0, animePalette(4u));
        animeSmoke(C, p, vec2(-22.0 + wind, -36.0), 3.6, time, wind);
    } else if (style == 4) {
        // ---- The forge: a stone ground floor opening on a glowing furnace,
        // timber above, a slate roof and a tall brick chimney with sparks.
        float chimney = sdRoundBox(p - vec2(-30.0, -30.0), vec2(8.0, 24.0), 2.0);
        animePart(C, chimney, animeMatte(animeHex(0xB86A52u)));
        animeStones(C, p, chimney, animeMatte(animeHex(0xB86A52u)), vec2(7.0, 4.0), seed);
        animePart(C, sdRoundBox(p - vec2(-30.0, -54.0), vec2(10.0, 2.5), 1.5), stone);
        float wallD = sdRoundBox(p - vec2(4.0, 26.0), vec2(42.0, 31.0), 3.0);
        animePart(C, wallD, wall);
        animePlaster(C, p, wallD, wall, seed);
        animeTimber(C, p, max(wallD + 1.0, p.y - 20.0), timber, 21.0, -5.0, 20.0, 1.0, seed);
        float base = max(wallD, -(p.y - 22.0));
        animePartNoLine(C, base, stone);
        animeStones(C, p, base, stone, vec2(10.0, 6.0), seed + 2.0);
        animeLine(C, max(abs(p.y - 22.0), wallD), stone.ink);
        float roofD = sdRoundRoof(p, vec2(4.0, -4.0), 54.0, 30.0, 0.4, 5.0);
        animeCastShadow(C, wallD, sdRoundRoof(p - vec2(2.0, 6.0), vec2(4.0, -4.0), 54.0, 30.0, 0.4, 5.0), wall);
        animePart(C, roofD, roof);
        animeRoofTiles(C, p, roofD, roof, 5.0, 9.0, seed);
        // The forge mouth: an arch with the fire inside.
        float mouth = sdArch(p, vec2(12.0, ground - 1.0), 17.0, 30.0);
        float flick = 0.85 + 0.15 * sin(time * 9.0 + seed) * sin(time * 5.3);
        C.color = over(vec4(vec3(1.0, 0.55, 0.2), glowFalloff(mouth, 22.0) * 0.55 * flick), C.color);
        animeFlat(C, mouth, mix(vec3(0.35, 0.12, 0.1), vec3(1.0, 0.5, 0.15) * 1.6, smoothstep(ground, ground - 22.0, p.y) * flick));
        float coals = max(mouth + 1.0, -(p.y - ground + 7.0));
        animeFlat(C, coals, vec3(1.0, 0.75, 0.3) * 1.8 * flick);
        animeLine(C, abs(mouth), stone.ink);
        animeWindow(C, p, vec2(-22.0, 14.0), 6.0, 12.0, night, 0.0, frame, shutter, false, seed);
        animePart(C, sdSegment(p, vec2(-38.0, 30.0), vec2(-45.0, 30.0), 1.1), animeMetal(animeHex(0x5E6070u)));
        animeSign(C, p, vec2(-50.0, 30.0), 8.5, kAnimeIconSword, animeMaterial(animeHex(0xD9DEE8u)), time);
        animeSmoke(C, p, vec2(-30.0 + wind, -58.0), 5.0, time * 1.3, wind);
        // Sparks rising from the chimney.
        for (int i = 0; i < 5; ++i) {
            float life = fract(time * 0.8 + float(i) * 0.2);
            vec2 sp = vec2(-30.0 + sin(life * 9.0 + float(i)) * 6.0 + wind * life * 10.0, -58.0 - life * 40.0);
            C.color = over(vec4(vec3(1.0, 0.7, 0.3) * 1.6, (1.0 - smoothstep(0.4, 1.4, length(p - sp))) * (1.0 - life)), C.color);
        }
        animeLantern(C, p, vec2(36.0, 10.0), 3.0, max(night, 0.4), time);
    } else if (style == 5) {
        // ---- The bakery: a cream house with a bun-round tiled roof, a bay
        // window full of bread and pies, a pink awning and a bread sign.
        float chimney = sdRoundBox(p - vec2(24.0, -28.0), vec2(5.5, 12.0), 2.0);
        animePart(C, chimney, stone);
        animeStones(C, p, chimney, stone, vec2(5.5, 4.0), seed);
        float wallD = sdRoundBox(p - vec2(0.0, 26.0), vec2(40.0, 31.0), 6.0);
        animePart(C, wallD, wall);
        animePlaster(C, p, wallD, wall, seed);
        float roofD = sdRoundRoof(p, vec2(0.0, -2.0), 50.0, 40.0, 0.05, 18.0);
        animeCastShadow(C, wallD, sdRoundRoof(p - vec2(2.0, 6.0), vec2(0.0, -2.0), 50.0, 40.0, 0.05, 18.0), wall);
        animePart(C, roofD, roof);
        animeRoofTiles(C, p, roofD, roof, 6.0, 7.0, seed);
        animeGloss(C, p, roofD, vec2(-18.0, -26.0), vec2(14.0, 6.0), 0.3);
        animeRoundWindow(C, p, vec2(0.0, -18.0), 6.0, night, frame);
        // The bay window with goods.
        vec2 win = vec2(-16.0, 34.0);
        float bay = sdRoundBox(p - win, vec2(16.0, 12.0), 5.0);
        if (night > 0.0) {
            C.color = over(vec4(kAnimeWindowLight, glowFalloff(bay, 10.0) * night * 0.5), C.color);
        }
        animePart(C, bay, frame);
        float glass = sdRoundBox(p - win, vec2(14.0, 10.0), 4.0);
        animeFlat(C, glass, animeGlass(p, win, 10.0, night));
        for (int i = 0; i < 3; ++i) {
            vec2 bread = win + vec2(-9.0 + float(i) * 9.0, 6.0);
            float loaf = sdEllipse(p - bread, vec2(4.0, 3.0));
            animePart(C, max(loaf, glass), animeMaterial(animeHex(i == 1 ? 0xE8A05Cu : 0xD98A44u)));
        }
        animeAwning(C, p, vec2(-16.0, 19.0), 20.0, 8.0, animeHex(0xF39AB4u), animeHex(0xFFF4EAu), 6.0);
        animeDoor(C, p, vec2(20.0, ground - 1.5), 8.5, 21.0, doorWood, frame, night);
        animeWindow(C, p, vec2(20.0, 14.0), 6.0, 12.0, night, 4.0, frame, animeMaterial(animeHex(0x8BC6E8u)), true, seed);
        animePart(C, sdSegment(p, vec2(39.0, 26.0), vec2(45.0, 26.0), 1.1), animeMetal(animeHex(0x5E6070u)));
        animeSign(C, p, vec2(50.0, 26.0), 8.0, kAnimeIconBread, animeMaterial(animeHex(0xF6E2C0u)), time);
        animeFlowerPot(C, p, vec2(-40.0, ground), 6.0, animePalette(11u));
        animeSmoke(C, p, vec2(24.0 + wind, -42.0), 3.6, time, wind);
    } else if (style == 6) {
        // ---- The observatory: a round stone tower with a dome, a telescope
        // through the dome's slit, round windows and a star sign.
        float tower = sdRoundBox(p - vec2(0.0, 18.0), vec2(26.0, 40.0), 8.0);
        animePart(C, tower, stone);
        animeStones(C, p, tower, stone, vec2(10.0, 6.0), seed);
        animeLine(C, max(sdBezier(p, vec2(14.0, -12.0), vec2(18.0, 20.0), vec2(14.0, 52.0)), tower + 1.5), stone.ink);
        float dome = max(sdCircle(p - vec2(0.0, -20.0), 30.0), p.y + 18.0);
        animePart(C, dome, roof);
        animeGloss(C, p, dome, vec2(-12.0, -38.0), vec2(9.0, 6.0), 0.5);
        animePart(C, sdRoundBox(p - vec2(0.0, -18.0), vec2(31.0, 3.0), 2.0), animeMetal(animeHex(0xC9A06Au)));
        float slit = max(sdRoundBox(p - vec2(8.0, -34.0), vec2(4.0, 14.0), 3.0), dome);
        animeFlat(C, slit, mix(vec3(0.12, 0.12, 0.25), kAnimeWindowLight, night * 0.8));
        float scope = sdSegment(p, vec2(6.0, -30.0), vec2(30.0, -56.0), 4.0);
        animePart(C, scope, animeMetal(animeHex(0xD8C7A0u)));
        animePart(C, sdCircle(p - vec2(0.0, -51.0), 4.0), animeMetal(animeHex(0xC9A06Au)));
        animeRoundWindow(C, p, vec2(-10.0, 4.0), 5.0, night, frame);
        animeRoundWindow(C, p, vec2(10.0, 22.0), 4.0, night, frame);
        animeDoor(C, p, vec2(-6.0, ground - 1.5), 8.0, 20.0, doorWood, frame, night);
        animePart(C, sdSegment(p, vec2(-25.0, 26.0), vec2(-31.0, 26.0), 1.1), animeMetal(animeHex(0x5E6070u)));
        animeSign(C, p, vec2(-36.0, 26.0), 7.5, kAnimeIconStar, animeMaterial(animeHex(0x3D4C92u)), time);
        animeLantern(C, p, vec2(20.0, 36.0), 3.0, night, time);
    } else {
        // ---- The tree-top hut: canopy behind, trunk and branches, a plank
        // deck with a rope railing, a round plank hut with a leaf roof.
        AnimeMaterial leaves = animeMaterial(animeHex(0x6DBE57u));
        AnimeMaterial bark = animeMaterial(animeHex(0x8A5A3Cu));
        float canopy = 1e5;
        for (int i = 0; i < 5; ++i) {
            float a = float(i) * 1.3 + seed;
            canopy = min(canopy, sdCircle(p - vec2(-40.0 + float(i) * 20.0, -30.0 + sin(a) * 8.0), 16.0 + cos(a) * 3.0));
        }
        animePart(C, canopy, animeTinted(leaves, vec3(0.75, 0.8, 0.85)));
        float trunk = sdTrapezoid(p - vec2(0.0, 43.0), 13.0, 9.0, 15.0) - 2.0;
        trunk = min(trunk, sdLock(p, vec2(-4.0, 40.0), vec2(-24.0, 36.0), vec2(-38.0, 28.0), 4.5, 2.0));
        trunk = min(trunk, sdLock(p, vec2(4.0, 38.0), vec2(22.0, 34.0), vec2(36.0, 29.0), 4.5, 2.0));
        animePart(C, trunk, bark);
        animeLine(C, max(sdBezier(p, vec2(-3.0, 34.0), vec2(-4.0, 46.0), vec2(-2.0, 62.0)), trunk + 1.0), bark.ink);
        animeLine(C, max(sdBezier(p, vec2(4.0, 36.0), vec2(5.0, 48.0), vec2(4.0, 62.0)), trunk + 1.0), bark.ink);
        // The deck.
        float deck = sdRoundBox(p - vec2(0.0, 28.0), vec2(46.0, 3.5), 3.0);
        AnimeMaterial plank = animeMaterial(animeHex(0xC8955Au));
        // The hut.
        float hut = sdRoundBox(p - vec2(0.0, 10.0), vec2(24.0, 15.0), 9.0);
        animePart(C, hut, animeMaterial(wallColor));
        animePlanks(C, p, hut, animeMaterial(wallColor), 6.0, true, seed);
        float roofD = sdRoundRoof(p, vec2(0.0, -2.0), 33.0, 40.0, 0.04, 4.0);
        roofD = min(roofD, sdLock(p, vec2(0.0, -38.0), vec2(4.0, -46.0), vec2(12.0 + wind * 2.0, -48.0), 3.0, 0.8));
        animeCastShadow(C, hut, sdRoundRoof(p - vec2(1.5, 5.0), vec2(0.0, -2.0), 33.0, 40.0, 0.04, 4.0), animeMaterial(wallColor));
        animePart(C, roofD, roof);
        animeThatch(C, p, roofD, roof, 6.0, seed);
        animeRoundWindow(C, p, vec2(-11.0, 8.0), 4.5, night, frame);
        animeDoor(C, p, vec2(10.0, 24.5), 6.0, 15.0, doorWood, frame, night);
        animePart(C, deck, plank);
        animePlanks(C, p, deck, plank, 6.0, true, seed + 2.0);
        // A rope railing on posts at both ends of the deck.
        AnimeMaterial rope = animeMaterial(animeHex(0xC9A06Au));
        for (int i = -1; i <= 1; i += 2) {
            for (int j = 0; j < 2; ++j) {
                float x = float(i) * (44.0 - float(j) * 14.0);
                animePart(C, sdRoundBox(p - vec2(x, 18.0), vec2(1.6, 8.0), 1.2), animeMaterial(animeHex(0x9C6438u)));
            }
            float x0 = float(i) * 44.0;
            float x1 = float(i) * 30.0;
            float t = clamp((p.x - min(x0, x1)) / 14.0, 0.0, 1.0);
            float sagY = 12.0 + 3.0 * 4.0 * t * (1.0 - t);
            animePart(C, max(abs(p.y - sagY) - 0.9, max(min(x0, x1) - p.x, p.x - max(x0, x1))), rope);
        }
        animeLantern(C, p, vec2(26.0, -4.0), 3.0, night, time);
        // Leaves in front of the trunk.
        float front = min(sdCircle(p - vec2(-44.0, 30.0), 7.0), sdCircle(p - vec2(43.0, 31.0), 6.5));
        animePart(C, front, leaves);
    }

    outColor = animeFinish(C);
    outColor.a *= s.color.a;
}
