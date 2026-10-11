#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/tile.glsl"
#include "gamelib/noise.glsl"
#include "gamelib/anime_building.glsl"

// Wayfarer's ground: storybook terrain in the MapleStory-style look - solid
// ground tiles with a soft rounded outline along the open sides, a cap of
// grass (or moss, sand, snow, cobbles, a rock lip) on top with a scalloped
// lower edge and a light top line, the body in soil, rock, bricks or peat
// with world-space blotches and pebbles that never swim; 45 degree ramps;
// floating footholds drawn as one rounded platform per run with a soft
// underside; ropes and ladders.
//
//   color   the cap color (grass, moss, snow...)   color2  the body color
//   params  x kind (0 ground tile, 1 foothold run, 2 rope, 3 ladder,
//           4 ramp rising right, 5 ramp rising left), y style (0 grass,
//           1 mine rock, 2 marsh moss, 3 ruin sandstone, 4 snow, 5 town
//           cobbles), z seed, w unused
//   bits    ground tiles and ramps: drawTiles' mask / variant / cell

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const float kTileSize = 48.0;

vec3 shadeOf(vec3 c, float amount)
{
    return animeShadowTone(c, amount);
}

// The body's material at a world point: blotches, pebbles, strata or bricks.
vec3 bodyColor(vec2 w, vec3 base, int style)
{
    float n = fbm(w * 0.018, 3);
    vec3 c = base * (0.92 + 0.16 * n);
    if (style == 3) {
        // Sandstone bricks in courses.
        float row = floor(w.y / 24.0);
        float bx = w.x / 40.0 + row * 0.5;
        vec2 inBrick = vec2(fract(bx) * 40.0, fract(w.y / 24.0) * 24.0);
        float mortar = min(min(inBrick.x, 40.0 - inBrick.x), min(inBrick.y, 24.0 - inBrick.y));
        c = mix(c * (0.94 + 0.1 * hashCell(vec2(floor(bx), row))), shadeOf(base, 0.55), 1.0 - smoothstep(1.2, 2.6, mortar));
        return c;
    }
    vec3 v = voronoi(w * 0.045);
    float pebble = 1.0 - smoothstep(0.16, 0.22, v.x);
    float keep = step(0.62, fract(v.z * 7.13));
    c = mix(c, mix(base, vec3(1.0), 0.18), pebble * keep * 0.8);
    c = mix(c, shadeOf(base, 0.45), (1.0 - smoothstep(0.0, 0.05, abs(v.x - 0.2))) * keep * 0.6);
    if (style == 1) {
        // Mine rock: glimmer flecks.
        float fleck = 1.0 - smoothstep(0.03, 0.06, voronoi(w * 0.09 + 3.1).x);
        c = mix(c, vec3(0.75, 0.9, 1.25), fleck * step(0.8, hashCell(floor(w / 37.0))));
    } else {
        // Soft strata lines.
        float strata = abs(fract(w.y / 46.0 + valueNoise(vec2(w.x * 0.01, 3.0)) * 0.6) - 0.5);
        c = mix(c, shadeOf(base, 0.3), (1.0 - smoothstep(0.02, 0.05, strata)) * 0.35);
    }
    return c;
}

// The cap's look: highlights, blades, snow sparkle, cobbles.
vec3 capColor(vec2 w, float depth, vec3 cap, int style, float capDepth)
{
    vec3 c = cap * (0.95 + 0.1 * valueNoise(w * 0.05));
    // A light top band and a darker band towards the scalloped edge.
    c = mix(c, mix(cap, vec3(1.0), 0.38), 1.0 - smoothstep(2.0, 5.0, depth));
    c = mix(c, shadeOf(cap, 0.4), smoothstep(capDepth - 7.0, capDepth - 1.0, depth) * 0.75);
    if (style == 5) {
        // Cobbles.
        vec3 v = voronoi(w * vec2(0.05, 0.08));
        c = mix(cap * (0.9 + 0.2 * fract(v.z * 5.3)), shadeOf(cap, 0.6), 1.0 - smoothstep(0.03, 0.1, v.y - v.x));
        c = mix(c, mix(cap, vec3(1.0), 0.35), 1.0 - smoothstep(1.5, 4.0, depth));
    } else if (style == 0 || style == 2) {
        // Blade strokes in the grass.
        float blade = abs(fract(w.x / 7.0 + valueNoise(vec2(w.x * 0.1, 1.0))) - 0.5);
        c = mix(c, shadeOf(cap, 0.22), (1.0 - smoothstep(0.05, 0.12, blade)) * smoothstep(4.0, 9.0, depth) * 0.5);
    } else if (style == 4) {
        float sparkle = step(0.985, hashCell(floor(w / 3.0))) * (0.5 + 0.5 * sin(sprite.time * 3.0 + hashCell(floor(w / 3.0)) * 30.0));
        c = mix(c, vec3(1.3), sparkle);
    }
    return c;
}

// The cap's thickness under a world x: scalloped (drips for moss and snow).
float capDepthAt(float x, int style, float seed)
{
    float base = style == 5 ? 13.0 : (style == 1 ? 9.0 : 15.0);
    float scallop = abs(sin(x * 0.11 + seed));
    if (style == 2 || style == 4) {
        float drip = pow(max(sin(x * 0.07 + seed * 2.0) * 0.5 + 0.5, 0.0), 6.0) * 10.0;
        return base + scallop * 3.0 + drip;
    }
    return base + scallop * 4.0;
}

void groundTile(SpriteData s, vec2 p)
{
    uint mask = tileMask(s);
    vec2 h = s.size * 0.5;
    vec2 w = (vec2(tileCell(s)) + 0.5) * kTileSize + p;
    int style = int(s.params.y + 0.5);
    float seed = s.params.z;
    float px = max(length(dFdx(p)), 1e-4);
    float open = tileOpenDistance(p, mask, h, 13.0);
    vec2 g = vec2(dFdxFine(open), dFdyFine(open));
    float alpha = clamp(0.5 - open / px, 0.0, 1.0);
    if (alpha <= 0.0) {
        outColor = vec4(0.0);
        return;
    }
    vec3 body = s.color2.rgb;
    vec3 cap = s.color.rgb;
    vec3 c = bodyColor(w, body, style);
    float inside = -open; // distance in from the open outline
    // Cel shade: a shadow band along the open sides away from the light (right and below).
    float away = -dot(normalize(g + vec2(1e-5)), normalize(kAnimeToLight));
    c = mix(c, shadeOf(body, 0.35), (1.0 - smoothstep(10.0, 12.0, inside)) * smoothstep(-0.1, 0.4, away) * 0.8);
    // The cap on top (only where the tile above is open).
    if (tileOpen(mask, kTileN)) {
        float depth = p.y + h.y;
        float capDepth = capDepthAt(w.x, style, seed);
        if (depth < capDepth) {
            c = capColor(w, depth, cap, style, capDepth);
        }
        // The cap's ink edge.
        float edge = abs(depth - capDepth);
        c = mix(c, animeInkOf(cap), (1.0 - smoothstep(0.6, 1.6, edge)) * 0.9);
    }
    // The soft outline along the open sides.
    vec3 ink = animeInkOf(tileOpen(mask, kTileN) && p.y + h.y < 20.0 ? cap : body);
    c = mix(c, ink, 1.0 - smoothstep(2.4, 3.6, inside));
    outColor = vec4(c, alpha * s.color.a);
}

void rampTile(SpriteData s, vec2 p, bool risingRight)
{
    vec2 h = s.size * 0.5;
    vec2 w = (vec2(tileCell(s)) + 0.5) * kTileSize + p;
    int style = int(s.params.y + 0.5);
    float seed = s.params.z;
    // The surface: from the bottom corner to the top corner.
    float x = risingRight ? p.x : -p.x;
    float surface = -x; // the surface's y at this x (y down)
    float depth = (p.y - surface) * 0.7071; // perpendicular distance below the slope
    float px = max(length(dFdx(p)), 1e-4);
    float alpha = clamp(0.5 + depth / px, 0.0, 1.0);
    if (alpha <= 0.0) {
        outColor = vec4(0.0);
        return;
    }
    vec3 c = bodyColor(w, s.color2.rgb, style);
    float capDepth = capDepthAt(w.x, style, seed) * 0.9;
    if (depth < capDepth) {
        c = capColor(w, depth, s.color.rgb, style, capDepth);
    }
    c = mix(c, animeInkOf(s.color.rgb), (1.0 - smoothstep(0.6, 1.6, abs(depth - capDepth))) * 0.9);
    c = mix(c, animeInkOf(s.color.rgb), 1.0 - smoothstep(2.4, 3.6, depth));
    outColor = vec4(c, alpha * s.color.a);
}

void footholdRun(SpriteData s, vec2 p)
{
    // The quad: 8 units of margin left, right and above; the walkable top at
    // y = -half + 8; the platform 20 thick with a rounded underside below.
    vec2 h = s.size * 0.5;
    int style = int(s.params.y + 0.5);
    float seed = s.params.z;
    float top = -h.y + 8.0;
    float halfW = h.x - 8.0;
    vec2 w = vec2(s.params.w, 0.0) + inUv * s.size; // world units (params.w: the run's left edge), so nothing swims
    AnimeCanvas C = animeBegin(p, 3.0);
    C.shadeDepth = 6.0;
    vec3 bodyBase = s.color2.rgb;
    AnimeMaterial body = animeMatte(bodyBase);
    float slab = sdRoundBox(p - vec2(0.0, top + 14.0), vec2(halfW, 14.0), 12.0);
    float bulge = sdEllipse(p - vec2(0.0, top + 26.0), vec2(halfW * 0.82, 16.0));
    float under = animeSmoothUnion(slab, bulge, 10.0);
    animePart(C, under, body);
    // Pebbles and a soft underside shade.
    vec3 v = voronoi(w * 0.06);
    float pebble = max((v.x - 0.17) * 24.0 + step(fract(v.z * 7.0), 0.6) * 100.0, under + 3.0);
    if (style != 3) {
        animePartNoLine(C, pebble, animeMatte(mix(bodyBase, vec3(1.0), 0.2)));
    } else {
        animeStones(C, p, under, body, vec2(18.0, 9.0), seed);
    }
    C.color = over(vec4(body.shade, animeCover(C, max(under + C.line, -(p.y - top - 30.0))) * 0.6), C.color);
    // The top band.
    vec3 capBase = s.color.rgb;
    float capThick = style == 5 ? 12.0 : 14.0;
    float band = sdRoundBox(p - vec2(0.0, top + capThick * 0.5), vec2(halfW + 2.0, capThick * 0.5), capThick * 0.5);
    float scallopLine = p.y - top - capThick - (style == 2 || style == 4 ? pow(max(sin(p.x * 0.09 + seed) * 0.5 + 0.5, 0.0), 6.0) * 8.0 : abs(sin(p.x * 0.12 + seed)) * 3.5);
    float capD = min(band, max(under, scallopLine));
    AnimeMaterial capMat = style == 4 ? animeMatte(capBase) : animeMaterial(capBase);
    animePart(C, capD, capMat);
    if (style == 1 || style == 3) {
        // Wooden planks (mines) or cut stone (ruins) on top.
        animePlanks(C, p, capD, capMat, style == 1 ? 26.0 : 34.0, true, seed);
    }
    animeFlat(C, max(capD + 1.5, abs(p.y - top - 2.5) - 1.2), mix(capBase, vec3(1.0), 0.45));
    if (style == 0 || style == 2) {
        // Grass tufts along the top.
        for (int i = 0; i < 4; ++i) {
            float tx = (hashCell(vec2(float(i), seed + floor(halfW))) - 0.5) * halfW * 1.7;
            vec2 base = vec2(tx, top + 3.0);
            animePart(C, sdEar(p, base, base + vec2(-3.0, -7.0), 2.0, 0.2), capMat);
            animePart(C, sdEar(p, base, base + vec2(3.5, -6.0), 1.8, -0.2), capMat);
        }
    }
    if (style == 4) {
        // Icicles under the snow.
        float ix = mod(p.x + seed * 13.0, 26.0) - 13.0;
        float icicle = max(sdTriangle(vec2(ix, p.y), vec2(-4.0, top + 22.0), vec2(4.0, top + 22.0), vec2(0.0, top + 34.0)),
                           abs(p.x) - halfW + 10.0);
        animePart(C, icicle, animeMaterial(animeHex(0xCDEBFFu)));
    }
    outColor = animeFinish(C);
    outColor.a *= s.color.a;
}

void main()
{
    SpriteData s = sprite;
    vec2 p = spriteLocal(inUv);
    int kind = int(s.params.x + 0.5);
    if (kind == 0) {
        groundTile(s, p);
    } else if (kind == 4 || kind == 5) {
        rampTile(s, p, kind == 4);
    } else if (kind == 1) {
        footholdRun(s, p);
    } else {
        vec2 h = s.size * 0.5;
        AnimeCanvas C = animeBegin(p, 2.6);
        if (kind == 2) {
            animeRope(C, p, vec2(0.0, -h.y + 2.0), h.y * 2.0 - 4.0, s.time);
        } else {
            animeLadder(C, p, vec2(0.0, -h.y + 2.0), h.y * 2.0 - 4.0, min(13.0, h.x - 3.0));
        }
        outColor = animeFinish(C);
        outColor.a *= s.color.a;
    }
}
