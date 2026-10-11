#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_building.glsl"
#include "gamelib/anime_monster.glsl"

// Wayfarer's decorations in the MapleStory-style look, with the same ink
// lines, cel shading and palette as the characters: round broadleaf trees,
// pines, giant mushrooms, bushes, flowers, grass tufts, boulders, glowing
// crystals, street lamps, fences, signposts, barrels, crates, flower pots,
// a well, a bench, a mine cart, reeds, broken columns, the town gate's arch,
// torches, the waystones, snow piles, lantern posts, banners, an anvil on a
// stump and a market stall.
//
// Design space: 128 units over the quad height, the ground on y = +58; the
// width follows the quad (fences are wide).
//   color   main tint (foliage, cap, paint)   color2  second tint
//   params  x kind (game::DecoKind: 1 tree ... 27 stall), y lit 0..1 (night
//           lamps, an open waystone), z variant, w seed

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

AnimeCanvas C;
vec2 P;
float T;

const float G = 58.0; // the ground line

float hsh(float x)
{
    return hashCell(vec2(x, 3.7));
}

// A convex quad (crystals, gems).
float sdPolygonQuad(vec2 p, vec2 a, vec2 b, vec2 c, vec2 d)
{
    vec2 v[kMaxPolygonVertices];
    v[0] = a;
    v[1] = b;
    v[2] = c;
    v[3] = d;
    return sdPolygon(p, v, 4);
}

// A flame: round at the bottom (y = 0), a tip `h` above.
float sdEgg(vec2 p, float r, float h)
{
    return sdLock(p, vec2(0.0, 0.0), vec2(0.0, -h * 0.5), vec2(0.0, -h), r, r * 0.1);
}

void tree(vec3 leaf, vec3 accent, float variant, float seed)
{
    AnimeMaterial bark = animeMaterial(animeHex(0x9A6440u));
    float trunk = sdTrapezoid(P - vec2(0.0, 30.0), 11.0, 7.0, 28.0);
    trunk = animeSmoothUnion(trunk, sdEllipse(P - vec2(0.0, G - 2.0), vec2(16.0, 4.0)), 6.0);
    animePart(C, trunk, bark);
    animeLine(C, max(sdBezier(P, vec2(-2.0, 50.0), vec2(1.0, 36.0), vec2(-1.0, 22.0)), trunk + 1.0), bark.ink);
    // The canopy: overlapping round puffs, swaying gently.
    float sway = sin(T * 1.1 + seed) * 1.2;
    float canopy = 1e5;
    for (int i = 0; i < 6; ++i) {
        float a = float(i) / 6.0 * 6.2831 + seed;
        vec2 c = vec2(cos(a) * 26.0, sin(a) * 15.0 - 22.0) + vec2(sway * (sin(a) * 0.5 + 0.5), 0.0);
        canopy = min(canopy, sdCircle(P - c, 22.0 + 4.0 * hsh(float(i) + seed)));
    }
    canopy = min(canopy, sdCircle(P - vec2(sway, -30.0), 30.0));
    AnimeMaterial leaves = animeMaterial(leaf);
    leaves.shadeDepth = 1.6;
    animePart(C, canopy, leaves);
    // Inner puff lines and highlights.
    for (int i = 0; i < 3; ++i) {
        vec2 c = vec2(-14.0 + float(i) * 15.0, -18.0 + float(i % 2) * 10.0);
        animeLine(C, max(abs(sdCircle(P - c - vec2(sway, 0.0), 13.0)), max(canopy + 2.0, P.y - c.y - 4.0)), leaves.ink);
    }
    animeGloss(C, P, canopy, vec2(-16.0 + sway, -44.0), vec2(10.0, 7.0), 0.35);
    if (variant > 0.5) {
        // Round fruits.
        for (int i = 0; i < 4; ++i) {
            vec2 f = vec2((hsh(float(i) * 3.0 + seed) - 0.5) * 60.0, -30.0 + (hsh(float(i) * 7.0 + seed) - 0.3) * 30.0);
            animePart(C, max(sdCircle(P - f, 4.0), canopy + 3.0), animeMaterial(accent));
        }
    }
}

void pine(vec3 leaf, float snowy, float seed)
{
    AnimeMaterial bark = animeMaterial(animeHex(0x8A5A3Cu));
    animePart(C, sdRoundBox(P - vec2(0.0, 46.0), vec2(6.0, 14.0), 2.0), bark);
    AnimeMaterial needles = animeMaterial(leaf);
    float d = 1e5;
    for (int i = 0; i < 4; ++i) {
        float fi = float(i);
        float y = 34.0 - fi * 20.0;
        float w = 38.0 - fi * 8.0;
        d = min(d, sdTriangle(P, vec2(-w, y), vec2(w, y), vec2(0.0, y - 34.0)) - 3.0);
    }
    animePart(C, d, needles);
    for (int i = 0; i < 3; ++i) {
        float y = 34.0 - float(i) * 20.0 - 20.0;
        animeLine(C, max(abs(P.y - y - abs(P.x) * 0.25) - 0.0, d + 2.0), needles.ink);
    }
    if (snowy > 0.5) {
        for (int i = 0; i < 4; ++i) {
            float fi = float(i);
            float y = 34.0 - fi * 20.0 - 30.0;
            float cap = max(d + 0.5, P.y - y - 4.0 - sin(P.x * 0.3 + seed) * 2.0);
            animePartNoLine(C, cap, animeMatte(animeHex(0xF4F8FFu)));
        }
    }
}

void bigShroom(vec3 cap, vec3 spots, float seed)
{
    AnimeMaterial stalk = animeMaterial(animeHex(0xF6E8D2u));
    float s = sdTrapezoid(P - vec2(0.0, 30.0), 13.0, 9.0, 28.0);
    animePart(C, s, stalk);
    animeLine(C, max(sdBezier(P, vec2(4.0, 50.0), vec2(6.0, 36.0), vec2(4.0, 12.0)), s + 1.0), stalk.ink);
    float capD = sdBellCap(P, vec2(0.0, 6.0), 50.0, 46.0, 6.0);
    AnimeMaterial capMat = animeMaterial(cap);
    animePart(C, capD, capMat);
    float sp = animeSpots(P - vec2(seed * 10.0, 0.0), 22.0, 6.0, seed);
    animePartNoLine(C, max(sp, capD + 2.5), animeMaterial(spots));
    animeGloss(C, P, capD, vec2(-22.0, -24.0), vec2(10.0, 7.0), 0.4);
}

void bush(vec3 leaf, vec3 bloom, float seed)
{
    float d = 1e5;
    for (int i = 0; i < 4; ++i) {
        vec2 c = vec2((float(i) - 1.5) * 18.0, G - 16.0 - 8.0 * hsh(float(i) + seed));
        d = min(d, sdCircle(P - c, 16.0 + 4.0 * hsh(float(i) * 2.0 + seed)));
    }
    d = max(d, P.y - G);
    AnimeMaterial m = animeMaterial(leaf);
    animePart(C, d, m);
    animeGloss(C, P, d, vec2(-14.0, G - 30.0), vec2(8.0, 5.0), 0.3);
    for (int i = 0; i < 4; ++i) {
        vec2 f = vec2((hsh(float(i) * 5.0 + seed) - 0.5) * 60.0, G - 12.0 - hsh(float(i) * 9.0 + seed) * 20.0);
        animePart(C, max(sdCircle(P - f, 3.2), d + 2.0), animeMaterial(bloom));
        animeFlat(C, sdCircle(P - f, 1.2), animeHex(0xFAD65Eu));
    }
}

void flowers(vec3 a, vec3 b, float seed)
{
    for (int i = 0; i < 5; ++i) {
        float fi = float(i);
        float x = (fi - 2.0) * 13.0 + (hsh(fi + seed) - 0.5) * 6.0;
        float h = 22.0 + hsh(fi * 3.0 + seed) * 16.0;
        float sway = sin(T * 2.0 + fi + seed) * 2.0;
        vec2 top = vec2(x + sway, G - h);
        animeStroke(C, sdBezier(P, vec2(x, G), vec2(x, G - h * 0.5), top), 1.4, animeInkOf(animeHex(0x6DBE57u)));
        animePart(C, sdEar(P, vec2(x, G - 4.0), vec2(x + 7.0, G - 12.0), 2.6, -0.2), animeMaterial(animeHex(0x86D065u)));
        vec3 col = i % 2 == 0 ? a : b;
        for (int k = 0; k < 5; ++k) {
            float ang = float(k) * 1.2566 + seed;
            animePart(C, sdCircle(P - top - vec2(cos(ang), sin(ang)) * 4.2, 3.2), animeMaterial(col));
        }
        animeFlat(C, sdCircle(P - top, 2.2), animeHex(0xFAD65Eu));
    }
}

void grass(vec3 leaf, float seed)
{
    AnimeMaterial m = animeMaterial(leaf);
    for (int i = 0; i < 7; ++i) {
        float fi = float(i);
        float x = (fi - 3.0) * 7.0;
        float h = 14.0 + hsh(fi + seed) * 16.0;
        float sway = sin(T * 2.3 + fi * 0.7 + seed) * 3.0;
        animePart(C, sdEar(P, vec2(x, G + 1.0), vec2(x + sway + (fi - 3.0) * 1.5, G - h), 2.6, 0.2 * (fi - 3.0) / 3.0), m);
    }
}

void rock(vec3 stone, vec3 moss, float variant, float seed)
{
    float d = sdEllipse(P - vec2(0.0, G - 18.0), vec2(34.0, 22.0));
    d = animeSmoothUnion(d, sdCircle(P - vec2(-14.0, G - 30.0), 16.0), 8.0);
    d = max(d, P.y - G);
    AnimeMaterial m = animeMatte(stone);
    animePart(C, d, m);
    animeLine(C, max(sdBezier(P, vec2(4.0, G - 34.0), vec2(10.0, G - 22.0), vec2(6.0, G - 8.0)), d + 1.5), m.ink);
    if (variant > 0.5) {
        float cap = max(d + 0.5, P.y - (G - 32.0) - sin(P.x * 0.2 + seed) * 3.0);
        animePartNoLine(C, cap, animeMaterial(moss));
    }
    animeGloss(C, P, d, vec2(-16.0, G - 36.0), vec2(7.0, 4.0), 0.3);
}

void crystal(vec3 col, float seed)
{
    AnimeMaterial m = animeGlow(col);
    float d = 1e5;
    for (int i = 0; i < 4; ++i) {
        float fi = float(i);
        float x = (fi - 1.5) * 12.0;
        float h = 24.0 + hsh(fi + seed) * 30.0;
        float lean = (fi - 1.5) * 0.15;
        vec2 base = vec2(x, G);
        vec2 tip = base + vec2(sin(lean) * h, -h);
        vec2 side = vec2(cos(lean), sin(lean)) * 6.0;
        d = min(d, sdPolygonQuad(P, base - side, base + side, tip + side * 0.6, tip - side * 0.6) );
    }
    float pulse = 0.7 + 0.3 * sin(T * 1.8 + seed * 5.0);
    C.color = over(vec4(col * 1.6, glowFalloff(d, 16.0) * 0.45 * pulse), C.color);
    animePart(C, d, m);
    animeFlat(C, max(d + 2.0, abs(P.x + P.y * 0.2 - 2.0) - 1.4), mix(col, vec3(1.0), 0.7));
}

void lamp(float lit)
{
    AnimeMaterial iron = animeMetal(animeHex(0x4A4B5Cu));
    animePart(C, sdRoundBox(P - vec2(-10.0, G - 3.0), vec2(9.0, 3.0), 2.0), iron);
    animePart(C, sdSegment(P, vec2(-10.0, G - 4.0), vec2(-10.0, -48.0), 2.4), iron);
    animePart(C, sdLock(P, vec2(-10.0, -44.0), vec2(4.0, -60.0), vec2(14.0, -44.0), 1.8, 1.4), iron);
    animeLantern(C, P, vec2(14.0, -44.0), 9.0, lit, T);
}

void signpost(float seed)
{
    AnimeMaterial wood = animeMaterial(animeHex(0xB07A45u));
    animePart(C, sdRoundBox(P - vec2(0.0, 10.0), vec2(3.5, 48.0), 2.5), wood);
    AnimeMaterial board = animeMaterial(animeHex(0xE9C48Au));
    for (int i = 0; i < 2; ++i) {
        float dir = (i == 0) == (seed < 0.5) ? 1.0 : -1.0;
        vec2 c0 = vec2(dir * 14.0, -22.0 + float(i) * 22.0);
        vec2 q = vec2((P.x - c0.x) * dir, P.y - c0.y);
        float arrow = min(sdRoundBox(q - vec2(-4.0, 0.0), vec2(18.0, 7.0), 2.0), sdTriangle(q, vec2(12.0, -9.0), vec2(12.0, 9.0), vec2(24.0, 0.0)));
        animePart(C, arrow, board);
        animeLine(C, max(abs(q.y), max(abs(q.x + 2.0) - 12.0, arrow + 1.5)), board.ink);
    }
}

void well(float seed)
{
    AnimeMaterial stone = animeMatte(animeHex(0xB4B0C4u));
    AnimeMaterial wood = animeMaterial(animeHex(0xA0683Au));
    AnimeMaterial roof = animeMaterial(animeHex(0xEC5662u));
    animePart(C, sdSegment(P, vec2(-26.0, G - 20.0), vec2(-26.0, -24.0), 2.6), wood);
    animePart(C, sdSegment(P, vec2(26.0, G - 20.0), vec2(26.0, -24.0), 2.6), wood);
    float r = sdRoundRoof(P, vec2(0.0, -20.0), 38.0, 24.0, 0.2, 4.0);
    animePart(C, r, roof);
    animeRoofTiles(C, P, r, roof, 6.0, 8.0, seed);
    animePart(C, sdSegment(P, vec2(-22.0, -8.0), vec2(22.0, -8.0), 1.6), wood);
    animeStroke(C, sdSegment(P, vec2(4.0, -8.0), vec2(4.0, 10.0), 0.0), 0.8, wood.ink);
    animePart(C, sdTrapezoid(P - vec2(4.0, 14.0), 6.0, 7.0, 5.0) - 1.0, wood);
    float ring = sdRoundBox(P - vec2(0.0, G - 12.0), vec2(34.0, 12.0), 4.0);
    animePart(C, ring, stone);
    animeStones(C, P, ring, stone, vec2(12.0, 6.0), seed);
}

void bench()
{
    AnimeMaterial wood = animeMaterial(animeHex(0xC8955Au));
    for (int i = -1; i <= 1; i += 2) {
        animePart(C, sdRoundBox(P - vec2(float(i) * 26.0, G - 9.0), vec2(3.0, 9.0), 1.5), animeTinted(wood, vec3(0.8)));
    }
    animePart(C, sdRoundBox(P - vec2(0.0, G - 20.0), vec2(36.0, 3.5), 2.0), wood);
    animePart(C, sdRoundBox(P - vec2(0.0, G - 38.0), vec2(34.0, 6.0), 3.0), wood);
    animePart(C, sdRoundBox(P - vec2(-24.0, G - 30.0), vec2(2.5, 9.0), 1.0), animeTinted(wood, vec3(0.85)));
    animePart(C, sdRoundBox(P - vec2(24.0, G - 30.0), vec2(2.5, 9.0), 1.0), animeTinted(wood, vec3(0.85)));
}

void cart(float seed)
{
    AnimeMaterial wood = animeMaterial(animeHex(0x9A6440u));
    AnimeMaterial iron = animeMetal(animeHex(0x6A6C7Eu));
    vec3 v = voronoi(P * 0.15 + seed);
    float pile = max(sdEllipse(P - vec2(0.0, G - 32.0), vec2(28.0, 14.0)), P.y - (G - 30.0));
    animePart(C, pile, animeMatte(animeHex(0x8E8AA6u)));
    animePartNoLine(C, max(pile + 1.0, v.x * 12.0 - 2.0 + step(fract(v.z * 3.0), 0.6) * 100.0), animeGlow(animeHex(0x7FE6FFu)));
    float box = sdTrapezoid(P - vec2(0.0, G - 20.0), 26.0, 32.0, 12.0) - 2.0;
    animePart(C, box, wood);
    animePlanks(C, P, box, wood, 8.0, false, seed);
    for (int i = -1; i <= 1; i += 2) {
        float wheel = sdCircle(P - vec2(float(i) * 18.0, G - 6.0), 7.0);
        animePart(C, wheel, iron);
        animeFlat(C, sdCircle(P - vec2(float(i) * 18.0, G - 6.0), 2.0), iron.ink);
    }
}

void reeds(vec3 leaf, float seed)
{
    for (int i = 0; i < 6; ++i) {
        float fi = float(i);
        float x = (fi - 2.5) * 9.0;
        float h = 40.0 + hsh(fi + seed) * 40.0;
        float sway = sin(T * 1.6 + fi + seed) * 3.0 * h / 80.0;
        vec2 top = vec2(x + sway, G - h);
        animeStroke(C, sdBezier(P, vec2(x, G), vec2(x, G - h * 0.5), top), 1.6, animeInkOf(leaf));
        if (i % 2 == 0) {
            animePart(C, sdRoundBox(P - top - vec2(0.0, 7.0), vec2(3.4, 8.0), 3.4), animeMaterial(animeHex(0x8A5A3Cu)));
        } else {
            animePart(C, sdEar(P, vec2(x, G), top + vec2(6.0, 10.0), 2.4, 0.3), animeMaterial(leaf));
        }
    }
}

void column(vec3 stone, float broken, float seed)
{
    AnimeMaterial m = animeMatte(stone);
    float top = broken > 0.5 ? -10.0 : -52.0;
    animePart(C, sdRoundBox(P - vec2(0.0, G - 5.0), vec2(22.0, 5.0), 2.0), m);
    float shaft = max(abs(P.x) - 15.0, max(top - P.y + (broken > 0.5 ? (sin(P.x * 0.6 + seed) * 4.0 + P.x * 0.3) : 0.0), P.y - G + 10.0));
    animePart(C, shaft, m);
    for (int i = -1; i <= 1; ++i) {
        animeLine(C, max(abs(P.x - float(i) * 7.0), shaft + 1.5), m.ink);
    }
    if (broken < 0.5) {
        animePart(C, sdRoundBox(P - vec2(0.0, top - 4.0), vec2(22.0, 5.0), 2.0), m);
    }
    float vine = sdBezier(P, vec2(-14.0, G - 10.0), vec2(10.0, G - 30.0), vec2(-8.0, top + 10.0));
    animeStroke(C, max(vine, shaft - 2.0), 1.4, animeHex(0x4F9A4Cu));
}

void arch(vec3 stone, float seed)
{
    AnimeMaterial m = animeMatte(stone);
    float outer = sdArch(P, vec2(0.0, G), 52.0, 108.0);
    float inner = sdArch(P, vec2(0.0, G + 2.0), 32.0, 82.0);
    float d = max(outer, -inner);
    animePart(C, d, m);
    animeStones(C, P, d, m, vec2(14.0, 9.0), seed);
    AnimeMaterial ivy = animeMaterial(animeHex(0x6DBE57u));
    float leaves = max(animeSpots(P, 9.0, 4.0, seed), d + 1.0);
    animePartNoLine(C, max(leaves, P.y + 30.0 - abs(P.x) * 0.3), ivy);
    animeSign(C, P, vec2(0.0, -46.0), 13.0, 5, animeMaterial(animeHex(0xE9C48Au)), T);
}

void torch(float lit, float seed)
{
    AnimeMaterial wood = animeMaterial(animeHex(0x8A5A3Cu));
    animePart(C, sdTrapezoid(P - vec2(0.0, 18.0), 3.0, 4.5, 40.0), wood);
    animePart(C, sdTrapezoid(P - vec2(0.0, -24.0), 9.0, 6.0, 5.0) - 1.0, animeMetal(animeHex(0x6A6C7Eu)));
    float flick = sin(T * 13.0 + seed * 9.0) * 0.5 + sin(T * 7.3 + seed) * 0.5;
    vec2 q = P - vec2(0.0, -34.0);
    float flame = sdEgg(q, 7.0 + flick * 0.6, 16.0 + flick * 2.0);
    C.color = over(vec4(vec3(1.0, 0.6, 0.2) * 1.6, glowFalloff(flame, 22.0) * 0.5), C.color);
    animeFlat(C, flame, vec3(1.0, 0.55, 0.15) * 1.6);
    animeFlat(C, sdEgg(q - vec2(0.0, 3.0), 3.5, 8.0), vec3(1.0, 0.95, 0.6) * 1.8);
}

void waystone(vec3 rune, float open, float seed)
{
    AnimeMaterial stone = animeMatte(animeHex(0xB8B4D0u));
    float base = sdRoundBox(P - vec2(0.0, G - 5.0), vec2(26.0, 5.0), 3.0);
    animePart(C, base, animeMatte(animeHex(0x9E9AB8u)));
    float slab = sdRoundBox(P - vec2(0.0, 6.0), vec2(16.0, 48.0), 9.0);
    slab = min(slab, sdCircle(P - vec2(0.0, -40.0), 16.0));
    animePart(C, slab, stone);
    animeLine(C, max(sdBezier(P, vec2(-8.0, 40.0), vec2(-4.0, 20.0), vec2(-9.0, 0.0)), slab + 1.5), stone.ink);
    // The rune: glowing when the way is open.
    float r = abs(sdCircle(P - vec2(0.0, -8.0), 9.0)) - 1.6;
    r = min(r, sdSegment(P, vec2(0.0, -20.0), vec2(0.0, 4.0), 1.6));
    r = min(r, sdSegment(P, vec2(-7.0, -8.0), vec2(7.0, -8.0), 1.6));
    vec3 glow = mix(stone.shade * 0.8, rune * 1.8, open);
    float pulse = 0.75 + 0.25 * sin(T * 2.0 + seed);
    C.color = over(vec4(rune * 1.5, glowFalloff(r, 18.0) * 0.6 * open * pulse), C.color);
    animeFlat(C, r, glow);
    // A floating gem above it.
    vec2 gem = vec2(0.0, -70.0 + sin(T * 1.7 + seed) * 4.0);
    float g = sdPolygonQuad(P, gem + vec2(0.0, -10.0), gem + vec2(8.0, 0.0), gem + vec2(0.0, 10.0), gem + vec2(-8.0, 0.0));
    C.color = over(vec4(rune * 1.4, glowFalloff(g, 20.0) * 0.5 * open), C.color);
    animePart(C, g, open > 0.5 ? animeGlow(rune) : animeMatte(animeHex(0x8E8AA6u)));
}

void snowpile(float seed)
{
    float d = 1e5;
    for (int i = 0; i < 3; ++i) {
        d = min(d, sdCircle(P - vec2((float(i) - 1.0) * 22.0, G + 6.0), 18.0 + 6.0 * hsh(float(i) + seed)));
    }
    d = max(d, P.y - G);
    AnimeMaterial m = animeMatte(animeHex(0xF4F8FFu));
    animePart(C, d, m);
    animeGloss(C, P, d, vec2(-16.0, G - 16.0), vec2(9.0, 4.0), 0.5);
}

void lanternPost(float lit)
{
    AnimeMaterial wood = animeMaterial(animeHex(0x8A5A3Cu));
    animePart(C, sdRoundBox(P - vec2(-8.0, 6.0), vec2(3.0, 52.0), 1.5), wood);
    animePart(C, sdRoundBox(P - vec2(2.0, -44.0), vec2(13.0, 2.5), 1.0), wood);
    animeLantern(C, P, vec2(12.0, -42.0), 8.0, lit, T);
}

void banner(vec3 cloth, float seed)
{
    AnimeMaterial pole = animeMetal(animeHex(0xC9A06Au));
    animePart(C, sdRoundBox(P - vec2(-14.0, 0.0), vec2(2.5, 58.0), 1.5), pole);
    animePart(C, sdCircle(P - vec2(-14.0, -60.0), 4.0), pole);
    float wave = sin(P.x * 0.15 - T * 3.0 + seed) * 3.0 * (P.x + 12.0) / 30.0;
    vec2 q = P - vec2(0.0, wave);
    float flag = max(max(abs(q.x - 4.0) - 17.0, abs(q.y + 30.0) - 22.0), q.y + 8.0 - abs(q.x - 4.0) * 0.9 - 6.0);
    animePart(C, flag, animeCloth(cloth));
    animePart(C, sdCircle(q - vec2(4.0, -34.0), 6.0), animeMaterial(animeHex(0xFAD65Eu)));
}

void anvil()
{
    AnimeMaterial stump = animeMaterial(animeHex(0x9A6440u));
    float s = sdRoundBox(P - vec2(0.0, G - 14.0), vec2(20.0, 14.0), 4.0);
    animePart(C, s, stump);
    animePart(C, sdEllipse(P - vec2(0.0, G - 27.0), vec2(19.0, 4.0)), animeMaterial(animeHex(0xE9C48Au)));
    AnimeMaterial iron = animeMetal(animeHex(0x5E6070u));
    float a = sdRoundBox(P - vec2(0.0, G - 36.0), vec2(16.0, 6.0), 2.0);
    a = min(a, sdTrapezoid(P - vec2(0.0, G - 30.0), 7.0, 10.0, 4.0));
    a = min(a, sdLock(P, vec2(14.0, G - 40.0), vec2(24.0, G - 39.0), vec2(30.0, G - 35.0), 3.5, 0.8));
    animePartShine(C, a, iron, animeCover(C, abs(P.y - (G - 41.0)) - 0.8) * 0.8, vec3(1.0));
    // A glowing ingot cooling on it.
    float ingot = sdRoundBox(P - vec2(-4.0, G - 44.0), vec2(7.0, 2.6), 1.4);
    C.color = over(vec4(vec3(1.0, 0.5, 0.15) * 1.6, glowFalloff(ingot, 10.0) * 0.6 * (0.8 + 0.2 * sin(T * 4.0))), C.color);
    animeFlat(C, ingot, vec3(1.0, 0.55, 0.2) * 1.5);
}

void stall(vec3 a, vec3 b, float seed)
{
    AnimeMaterial wood = animeMaterial(animeHex(0xB07A45u));
    for (int i = -1; i <= 1; i += 2) {
        animePart(C, sdRoundBox(P - vec2(float(i) * 52.0, 4.0), vec2(3.0, 54.0), 1.5), wood);
    }
    float counter = sdRoundBox(P - vec2(0.0, G - 14.0), vec2(56.0, 14.0), 3.0);
    animePart(C, counter, wood);
    animePlanks(C, P, counter, wood, 10.0, true, seed);
    // Goods: potions and apples on the counter.
    for (int i = 0; i < 5; ++i) {
        float x = -40.0 + float(i) * 20.0;
        vec3 col = i % 2 == 0 ? animeHex(0xEC5662u) : animeHex(0x5C9CEEu);
        float bottle = sdCircle(P - vec2(x, G - 34.0), 6.0);
        bottle = min(bottle, sdRoundBox(P - vec2(x, G - 42.0), vec2(2.0, 4.0), 1.0));
        animePart(C, bottle, animeGlow(col));
        animeFlat(C, sdCircle(P - vec2(x - 2.0, G - 36.0), 1.6), vec3(1.0));
    }
    animeAwning(C, P, vec2(0.0, -46.0), 62.0, 16.0, a, b, 8.0);
}

void main()
{
    SpriteData s = sprite;
    float k = s.size.y / 128.0;
    P = spriteLocal(inUv) / k;
    T = s.time;
    int kind = int(s.params.x + 0.5);
    float lit = clamp(s.params.y, 0.0, 1.0);
    float variant = s.params.z;
    float seed = s.params.w;
    vec3 a = s.color.rgb;
    vec3 b = s.color2.rgb;
    C = animeBegin(P, 3.0);
    C.shadeDepth = 5.0;
    vec2 extent = s.size / k * 0.5;
    switch (kind) {
    case 1: tree(a, b, variant, seed); break;
    case 2: pine(a, variant, seed); break;
    case 3: bigShroom(a, b, seed); break;
    case 4: bush(a, b, seed); break;
    case 5: flowers(a, b, seed); break;
    case 6: grass(a, seed); break;
    case 7: rock(a, b, variant, seed); break;
    case 8: crystal(a, seed); break;
    case 9: lamp(lit); break;
    case 10: animeFence(C, P, -extent.x + 4.0, extent.x - 4.0, G, 40.0, a); break;
    case 11: signpost(variant); break;
    case 12: animeBarrel(C, P, vec2(0.0, G), 24.0, 58.0); break;
    case 13: animeCrate(C, P, vec2(0.0, G), 50.0); break;
    case 14: animeFlowerPot(C, P, vec2(0.0, G), 30.0, a); break;
    case 15: well(seed); break;
    case 16: bench(); break;
    case 17: cart(seed); break;
    case 18: reeds(a, seed); break;
    case 19: column(a, variant, seed); break;
    case 20: arch(a, seed); break;
    case 21: torch(lit, seed); break;
    case 22: waystone(a, lit, seed); break;
    case 23: snowpile(seed); break;
    case 24: lanternPost(lit); break;
    case 25: banner(a, seed); break;
    case 26: anvil(); break;
    case 27: stall(a, b, seed); break;
    default: break;
    }
    outColor = animeFinish(C);
    outColor.a *= s.color.a;
}
