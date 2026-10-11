#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_monster.glsl"

// Wayfarer's interface in the storybook style: rounded cream windows with a
// soft title bar and shadow, pill buttons, item slots that glow in the
// rarity color, glossy gauges, and the icons - items, skills, the four core
// stats, quest markers, coins, key caps - plus the chunky outlined digits of
// the damage numbers. Icons share the characters' ink and cel shading.
//
//   color   main color   color2  accent
//   params  x kind (0 window, 1 button, 2 slot, 3 gauge, 4 item icon, 5 skill
//           icon, 6 stat icon, 7 quest marker, 8 digit, 9 coin, 10 key cap,
//           11 round frame, 12 arrow, 13 soft panel, 14 soft shadow),
//           y/z/w per kind (see each branch)
// Icons, markers and digits: 128 design units over the quad height.

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

AnimeCanvas C;
vec2 P;

vec4 add(vec4 acc, vec3 rgb, float a)
{
    return over(vec4(rgb, clamp(a, 0.0, 1.0)), acc);
}

// ---------------------------------------------------------------------------
// Windows and widgets (sprite units = canvas units)

vec4 window(vec2 p, vec2 h, float titleH, int style, vec3 fill, vec3 accent)
{
    float px = max(length(dFdx(p)), 1e-4);
    float r = style == 1 ? 12.0 : 18.0;
    vec4 c = vec4(0.0);
    // Shadow below.
    float sh = sdRoundBox(p - vec2(0.0, 8.0), h - vec2(10.0), r);
    c = add(c, vec3(0.08, 0.06, 0.15), (1.0 - smoothstep(-10.0, 12.0, sh)) * 0.35);
    float d = sdRoundBox(p, h - vec2(10.0), r);
    float inside = 1.0 - smoothstep(-px, px, d);
    vec3 ink = animeInkOf(accent);
    if (style == 1) {
        // A dark tooltip.
        c = add(c, fill, inside * 0.95);
        c = add(c, accent, (1.0 - smoothstep(0.5, 2.0, abs(d + 2.0))) * 0.9);
        return c;
    }
    if (style == 2) {
        // An inset well inside a window.
        c = add(c, fill * 0.94, inside);
        c = add(c, ink, (1.0 - smoothstep(0.6, 1.8, abs(d + 1.0))) * 0.35);
        c = add(c, vec3(0.0), (1.0 - smoothstep(0.0, 8.0, -d)) * inside * 0.06 * step(0.0, -(p.y + h.y - 18.0)));
        return c;
    }
    // A soft vertical gradient fill.
    float gy = (p.y + h.y) / (2.0 * h.y);
    vec3 body = mix(mix(fill, vec3(1.0), 0.25), fill * 0.94, gy);
    c = add(c, body, inside);
    // The title bar.
    if (titleH > 0.0) {
        float bar = max(d, p.y - (-h.y + 10.0 + titleH));
        float barIn = 1.0 - smoothstep(-px, px, bar);
        vec3 barColor = mix(mix(accent, vec3(1.0), 0.18), accent, smoothstep(-h.y, -h.y + titleH, p.y));
        c = add(c, barColor, barIn);
        c = add(c, vec3(1.0), (1.0 - smoothstep(0.0, 2.0, abs(p.y - (-h.y + 14.0)))) * barIn * 0.35);
        c = add(c, ink, (1.0 - smoothstep(0.6, 1.8, abs(p.y - (-h.y + 10.0 + titleH)))) * inside * 0.6);
    }
    // Inner highlight and the outline.
    c = add(c, vec3(1.0), (1.0 - smoothstep(0.5, 2.0, abs(d + 4.5))) * inside * 0.5);
    c = add(c, ink, 1.0 - smoothstep(1.2, 2.6, abs(d)));
    return c;
}

vec4 button(vec2 p, vec2 h, int state, vec3 fill, vec3 accent)
{
    float px = max(length(dFdx(p)), 1e-4);
    float r = min(h.x, h.y) - 2.0;
    vec4 c = vec4(0.0);
    float press = state == 2 ? 2.0 : 0.0;
    float sh = sdRoundBox(p - vec2(0.0, 4.0), h - vec2(2.0), r);
    c = add(c, animeInkOf(fill), (1.0 - smoothstep(-px, px, sh)) * (state == 3 ? 0.3 : 0.9));
    float d = sdRoundBox(p - vec2(0.0, press), h - vec2(2.0, 4.0), r);
    vec3 base = state == 1 ? mix(fill, accent, 0.45) : (state == 3 ? mix(fill, vec3(0.8), 0.6) : fill);
    float gy = (p.y + h.y) / (2.0 * h.y);
    c = add(c, mix(mix(base, vec3(1.0), 0.35), base, smoothstep(0.1, 0.6, gy)), 1.0 - smoothstep(-px, px, d));
    c = add(c, vec3(1.0), (1.0 - smoothstep(0.0, 2.5, abs(p.y + h.y * 0.45 - press))) * (1.0 - smoothstep(-px, px, d + 6.0)) * 0.5);
    c = add(c, animeInkOf(base), 1.0 - smoothstep(1.0, 2.2, abs(d)));
    if (state == 1) {
        c = add(c, accent, glowFalloff(d, 8.0) * 0.6);
    }
    return c;
}

vec4 slot(vec2 p, vec2 h, float rarity, float selected, vec3 fill, vec3 glow)
{
    float px = max(length(dFdx(p)), 1e-4);
    vec4 c = vec4(0.0);
    float d = sdRoundBox(p, h - vec2(4.0), 10.0);
    c = add(c, glow, glowFalloff(d, 8.0) * rarity * 0.8);
    c = add(c, fill, 1.0 - smoothstep(-px, px, d));
    // An inner bevel: darker at the top, light at the bottom.
    c = add(c, vec3(0.0), (1.0 - smoothstep(-px, px, d)) * (1.0 - smoothstep(-h.y, -h.y + 14.0, p.y)) * 0.07);
    c = add(c, mix(animeInkOf(fill), glow, rarity), 1.0 - smoothstep(0.8, 2.0 + rarity, abs(d)));
    if (selected > 0.0) {
        float ring = abs(sdRoundBox(p, h - vec2(1.5), 12.0)) - 2.0;
        c = add(c, vec3(1.0, 0.86, 0.35), (1.0 - smoothstep(-px, px, ring)) * selected);
        c = add(c, vec3(1.0, 0.8, 0.3), glowFalloff(ring, 6.0) * 0.6 * selected);
    }
    return c;
}

vec4 gauge(vec2 p, vec2 h, float fill, float trail, vec3 color, vec3 trailColor)
{
    float px = max(length(dFdx(p)), 1e-4);
    float r = h.y;
    vec4 c = vec4(0.0);
    float d = sdRoundBox(p, h, r);
    c = add(c, vec3(0.16, 0.13, 0.24), (1.0 - smoothstep(-px, px, d)) * 0.85);
    float inner = sdRoundBox(p, h - vec2(3.0), r - 3.0);
    float x = (p.x + h.x - 3.0) / (2.0 * (h.x - 3.0));
    float inIn = 1.0 - smoothstep(-px, px, inner);
    c = add(c, trailColor, inIn * step(x, trail));
    float gy = (p.y + h.y) / (2.0 * h.y);
    vec3 fillColor = mix(mix(color, vec3(1.0), 0.45), color, smoothstep(0.15, 0.55, gy));
    fillColor = mix(fillColor, color * 0.8, smoothstep(0.75, 1.0, gy));
    c = add(c, fillColor, inIn * (1.0 - smoothstep(fill - px / (2.0 * h.x), fill, x)));
    c = add(c, vec3(1.0), inIn * (1.0 - smoothstep(0.0, 2.0, abs(p.y + h.y * 0.42))) * 0.45 * step(x, fill));
    c = add(c, vec3(0.1, 0.08, 0.16), 1.0 - smoothstep(0.8, 2.0, abs(d)));
    return c;
}

// ---------------------------------------------------------------------------
// Icons (design units, 128 over the quad)

void potion(vec3 liquid)
{
    AnimeMaterial glass = animeMaterial(mix(liquid, vec3(1.0), 0.65));
    float bottle = sdCircle(P - vec2(0.0, 14.0), 34.0);
    bottle = animeSmoothUnion(bottle, sdRoundBox(P - vec2(0.0, -26.0), vec2(12.0, 14.0), 4.0), 8.0);
    animePart(C, bottle, glass);
    float fluid = max(sdCircle(P - vec2(0.0, 14.0), 29.0), -(P.y - 2.0 + sin(P.x * 0.15) * 2.0));
    animePartNoLine(C, fluid, animeGlow(liquid));
    animeFlat(C, sdEllipse(P - vec2(-12.0, 2.0), vec2(6.0, 9.0)), vec3(1.0));
    animePart(C, sdRoundBox(P - vec2(0.0, -42.0), vec2(14.0, 7.0), 3.0), animeMaterial(animeHex(0xB07A45u)));
}

void scroll(vec3 color)
{
    AnimeMaterial paper = animeMaterial(animeHex(0xF6EBCFu));
    float d = sdRoundBox(P, vec2(32.0, 40.0), 6.0);
    animePart(C, d, paper);
    animePart(C, sdRoundBox(P - vec2(0.0, -40.0), vec2(38.0, 7.0), 7.0), animeMaterial(animeHex(0xE0C58Eu)));
    animePart(C, sdRoundBox(P - vec2(0.0, 40.0), vec2(38.0, 7.0), 7.0), animeMaterial(animeHex(0xE0C58Eu)));
    for (int i = 0; i < 3; ++i) {
        animeLine(C, max(abs(P.y + 14.0 - float(i) * 12.0), abs(P.x) - 20.0), paper.ink);
    }
    animePart(C, sdCircle(P - vec2(16.0, 26.0), 9.0), animeMaterial(color));
}

void material(int m, vec3 a, vec3 b)
{
    if (m == 0 || m == 9) {
        // A soft cap / marsh cap.
        float cap = sdBellCap(P, vec2(0.0, 18.0), 40.0, 44.0, 6.0);
        animePart(C, sdRoundBox(P - vec2(0.0, 30.0), vec2(12.0, 14.0), 6.0), animeMaterial(animeHex(0xF6E8D2u)));
        animePart(C, cap, animeMaterial(a));
        animePartNoLine(C, max(animeDomeStripes(P, vec2(0.0, 12.0), 40.0, 14.0, 4.5), cap + 2.5), animeMaterial(b));
    } else if (m == 1 || m == 6 || m == 8) {
        // A jelly drop.
        float d = sdLock(P, vec2(0.0, 22.0), vec2(0.0, -8.0), vec2(0.0, -40.0), 28.0, 2.0);
        animePart(C, d, animeGlow(a));
        animeFlat(C, sdEllipse(P - vec2(-10.0, 12.0), vec2(6.0, 9.0)), vec3(1.0));
    } else if (m == 2) {
        // An acorn.
        animePart(C, sdEllipse(P - vec2(0.0, 14.0), vec2(26.0, 32.0)), animeMaterial(animeHex(0xC8823Fu)));
        float hat = max(sdCircle(P - vec2(0.0, -6.0), 32.0), P.y + 4.0);
        animePart(C, hat, animeMaterial(animeHex(0x8A5A3Cu)));
        animePart(C, sdRoundBox(P - vec2(0.0, -42.0), vec2(3.0, 8.0), 2.0), animeMaterial(animeHex(0x6E4428u)));
    } else if (m == 3 || m == 12) {
        // A twig / old bark.
        animePart(C, sdLock(P, vec2(-36.0, 30.0), vec2(0.0, 0.0), vec2(36.0, -30.0), 9.0, 6.0), animeMaterial(a));
        animePart(C, sdEar(P, vec2(0.0, 0.0), vec2(22.0, 14.0), 8.0, 0.3), animeMaterial(b));
    } else if (m == 4 || m == 11) {
        // Ore / a rune pebble.
        float d = sdCircle(P, 36.0) + valueNoise(P * 0.08) * 8.0;
        animePart(C, d, animeMatte(a));
        vec3 v = voronoi(P * 0.06);
        animePartNoLine(C, max(d + 3.0, v.x * 18.0 - 3.0 + step(fract(v.z * 3.0), 0.5) * 100.0), animeGlow(b));
    } else if (m == 5 || m == 13 || m == 14) {
        // Fluff, fur, a feather.
        float d = 1e5;
        for (int i = 0; i < 5; ++i) {
            float ang = float(i) * 1.2566;
            d = min(d, sdCircle(P - vec2(cos(ang), sin(ang)) * 18.0, 20.0));
        }
        if (m == 14) {
            d = sdLock(P, vec2(-30.0, 38.0), vec2(0.0, -10.0), vec2(26.0, -40.0), 4.0, 2.0);
            d = min(d, sdLockBelly(P, vec2(-20.0, 26.0), vec2(4.0, -8.0), vec2(26.0, -40.0), 4.0, 14.0));
        }
        animePart(C, d, animeMatte(a));
    } else if (m == 7) {
        // A swirl shell.
        float d = sdCircle(P, 38.0);
        animePart(C, d, animeMaterial(a));
        animeStroke(C, max(animeSpiral(P, vec2(0.0), 30.0, 2.2), d + 2.0), 2.5, animeInkOf(a));
        animeGloss(C, P, d, vec2(-14.0, -16.0), vec2(9.0, 6.0), 0.5);
    } else if (m == 10) {
        // A wisp light.
        float d = sdCircle(P, 26.0);
        C.color = add(C.color, a * 1.5, glowFalloff(d, 30.0) * 0.8);
        animePart(C, d, animeGlow(a));
    } else {
        // An ice shard.
        vec2 v[kMaxPolygonVertices];
        v[0] = vec2(0.0, -46.0);
        v[1] = vec2(22.0, -6.0);
        v[2] = vec2(10.0, 44.0);
        v[3] = vec2(-12.0, 44.0);
        v[4] = vec2(-22.0, -4.0);
        float d = sdPolygon(P, v, 5);
        animePart(C, d, animeGlow(a));
        animeFlat(C, max(d + 3.0, abs(P.x + P.y * 0.2 + 4.0) - 3.0), vec3(1.0));
    }
}

void weapon(int style, vec3 blade, vec3 trim)
{
    AnimeMaterial steel = animeMetal(blade);
    AnimeMaterial wood = animeMaterial(animeHex(0xA0683Au));
    vec2 q = rotate2d(P, -0.78);
    if (style == 2) {
        animePart(C, sdSegment(q, vec2(0.0, 50.0), vec2(0.0, -26.0), 4.0), wood);
        float orb = sdCircle(q - vec2(0.0, -38.0), 14.0);
        C.color = add(C.color, trim * 1.4, glowFalloff(orb, 14.0) * 0.6);
        animePart(C, orb, animeGlow(trim));
    } else if (style == 5) {
        animePart(C, sdSegment(q, vec2(0.0, 50.0), vec2(0.0, -40.0), 4.5), wood);
        float head = sdLockBelly(q, vec2(0.0, -34.0), vec2(22.0, -30.0), vec2(30.0, -6.0), 6.0, 16.0);
        animePartShine(C, head, steel, animeCover(C, abs(head + 3.0) - 1.0) * 0.8, vec3(1.0));
    } else {
        float len = style == 4 ? 34.0 : 54.0;
        float b = sdLock(q, vec2(0.0, 10.0), vec2(0.0, 10.0 - len * 0.5), vec2(0.0, 10.0 - len), style == 4 ? 7.0 : 9.0, 1.5);
        animePartShine(C, b, steel, animeCover(C, abs(q.x + 2.0) - 1.2) * 0.9, vec3(1.0));
        animePart(C, sdRoundBox(q - vec2(0.0, 14.0), vec2(18.0, 4.0), 3.0), animeMetal(trim));
        animePart(C, sdRoundBox(q - vec2(0.0, 32.0), vec2(4.5, 14.0), 3.0), animeMaterial(animeHex(0x7A4A2Eu)));
        animePart(C, sdCircle(q - vec2(0.0, 48.0), 6.0), animeMetal(trim));
    }
}

void hat(int style, vec3 color, vec3 trim)
{
    AnimeMaterial m = style == 4 ? animeMetal(color) : animeCloth(color);
    if (style == 2) {
        animePart(C, sdEllipse(P - vec2(0.0, 26.0), vec2(52.0, 12.0)), m);
        float crown = animeSmoothUnion(sdTrapezoid(P - vec2(0.0, 10.0), 26.0, 18.0, 12.0), sdLock(P, vec2(0.0, 0.0), vec2(4.0, -24.0), vec2(-26.0, -46.0), 18.0, 3.0), 4.0);
        animePart(C, crown, m);
        animePartNoLine(C, max(crown + 0.5, abs(P.y - 14.0) - 5.0), animeMaterial(trim));
    } else if (style == 3) {
        float b = max(sdCircle(P - vec2(0.0, 18.0), 44.0), P.y - 26.0);
        animePart(C, b, m);
        animePart(C, max(sdCircle(P - vec2(0.0, 18.0), 46.0), abs(P.y - 22.0) - 8.0), animeTinted(m, vec3(0.88)));
        animePart(C, sdCircle(P - vec2(0.0, -30.0), 10.0), animeCloth(animeHex(0xF7F1E3u)));
    } else if (style == 4) {
        float hm = max(sdCircle(P - vec2(0.0, 18.0), 42.0), P.y - 28.0);
        animePartShine(C, hm, m, animeCover(C, abs(sdCircle(P - vec2(-6.0, 8.0), 24.0)) - 2.0) * 0.7, vec3(1.0));
        animePart(C, sdLockBelly(P, vec2(0.0, -20.0), vec2(-22.0, -36.0), vec2(-44.0, -20.0), 6.0, 2.0), animeCloth(trim));
    } else if (style == 5) {
        AnimeMaterial r = animeCloth(color);
        animePart(C, sdLockBelly(P, vec2(0.0, 0.0), vec2(-14.0, -16.0), vec2(-34.0, -6.0), 4.0, 12.0), r);
        animePart(C, sdLockBelly(P, vec2(0.0, 0.0), vec2(14.0, -16.0), vec2(34.0, -6.0), 4.0, 12.0), r);
        animePart(C, sdLock(P, vec2(0.0, 0.0), vec2(-8.0, 20.0), vec2(-14.0, 36.0), 5.0, 3.0), r);
        animePart(C, sdLock(P, vec2(0.0, 0.0), vec2(8.0, 20.0), vec2(14.0, 36.0), 5.0, 3.0), r);
        animePart(C, sdCircle(P, 9.0), r);
    } else {
        float band = sdRoundBox(P - vec2(0.0, 6.0), vec2(44.0, 11.0), 10.0);
        animePart(C, band, m);
        animePart(C, sdLock(P, vec2(-40.0, 6.0), vec2(-46.0, 22.0), vec2(-36.0, 40.0), 6.0, 2.0), m);
        animePart(C, sdCircle(P - vec2(-40.0, 6.0), 8.0), animeTinted(m, vec3(0.9)));
    }
}

void top(int style, vec3 color, vec3 trim)
{
    AnimeMaterial m = style == 3 ? animeMetal(color) : animeCloth(color);
    float body = sdTrapezoid(P - vec2(0.0, 10.0), 30.0, 36.0, 34.0) - 4.0;
    if (style == 1) {
        body = sdTrapezoid(P - vec2(0.0, 14.0), 40.0, 30.0, 40.0) - 4.0;
    }
    float sleeves = min(sdSegment(P, vec2(-36.0, -18.0), vec2(-50.0, 18.0), 11.0), sdSegment(P, vec2(36.0, -18.0), vec2(50.0, 18.0), 11.0));
    animePart(C, sleeves, animeTinted(m, vec3(0.88)));
    animePart(C, body, m);
    animeLine(C, max(sdBezier(P, vec2(-12.0, -24.0), vec2(0.0, -8.0), vec2(12.0, -24.0)), body + 1.0), m.ink);
    if (style != 1) {
        animePartNoLine(C, max(body + 0.5, abs(P.y - 30.0) - 5.0), animeMaterial(animeHex(0x7A4A2Eu)));
        animePart(C, sdRoundBox(P - vec2(0.0, 30.0), vec2(5.0, 5.0), 1.5), animeMetal(trim));
    } else {
        animePartNoLine(C, max(body + 0.5, abs(P.x) - 4.0), animeMaterial(trim));
    }
    if (style == 3) {
        animeGloss(C, P, body, vec2(-12.0, -8.0), vec2(8.0, 6.0), 0.6);
    }
}

void shoes(int style, vec3 color)
{
    AnimeMaterial m = style == 2 ? animeMetal(color) : animeMaterial(color);
    for (int i = 0; i < 2; ++i) {
        vec2 o = vec2(i == 0 ? -20.0 : 18.0, i == 0 ? 6.0 : 16.0);
        AnimeMaterial mm = i == 0 ? animeTinted(m, vec3(0.85)) : m;
        if (style != 1) {
            animePart(C, sdRoundBox(P - o - vec2(-4.0, -22.0), vec2(12.0, 18.0), 5.0), mm);
        }
        animePart(C, sdRoundBox(P - o - vec2(4.0, 4.0), vec2(22.0, 11.0), 10.0), mm);
    }
}

void ring(vec3 gem)
{
    float band = abs(sdCircle(P - vec2(0.0, 14.0), 28.0)) - 6.0;
    animePart(C, band, animeMetal(animeHex(0xF2C14Eu)));
    float g = sdCircle(P - vec2(0.0, -20.0), 15.0);
    C.color = add(C.color, gem * 1.3, glowFalloff(g, 10.0) * 0.5);
    animePart(C, g, animeGlow(gem));
    animeFlat(C, sdCircle(P - vec2(-5.0, -25.0), 4.0), vec3(1.0));
}

void letter()
{
    AnimeMaterial paper = animeMaterial(animeHex(0xF6EBCFu));
    float d = sdRoundBox(P, vec2(44.0, 30.0), 4.0);
    animePart(C, d, paper);
    animeLine(C, max(min(abs(P.y + 30.0 - abs(P.x) * 0.68) - 0.0, 1e5), d + 1.0), paper.ink);
    animePart(C, sdCircle(P - vec2(0.0, 0.0), 10.0), animeMaterial(animeHex(0x5BB86Au)));
}

float star5(vec2 p, float r, float inner)
{
    float a = atan(p.x, -p.y);
    float seg = 6.2831853 / 5.0;
    float m = mod(a + seg * 0.5, seg) - seg * 0.5;
    return length(p) - mix(inner, r, pow(abs(cos(m * 2.5)), 3.0));
}

void charm(vec3 color)
{
    float d = star5(P, 46.0, 20.0) - 3.0;
    C.color = add(C.color, color * 1.4, glowFalloff(d, 18.0) * 0.6);
    animePart(C, d, animeGlow(color));
    animeFlat(C, star5(P - vec2(-6.0, -8.0), 12.0, 5.0), vec3(1.0));
}

void coin(vec3 gold)
{
    for (int i = 0; i < 3; ++i) {
        vec2 o = vec2((float(i) - 1.0) * 18.0, i == 1 ? -10.0 : 10.0);
        float d = sdEllipse(P - o, vec2(30.0, 28.0));
        animePart(C, d, animeMetal(gold));
        animeLine(C, max(abs(sdEllipse(P - o, vec2(19.0, 17.0))), d + 2.0), animeInkOf(gold));
    }
}

void skillIcon(int id, vec3 a, vec3 b)
{
    if (id == 0) {
        // Blade dance: two crossed blades.
        for (int i = 0; i < 2; ++i) {
            vec2 q = rotate2d(P, i == 0 ? -0.78 : 0.78);
            float bl = sdLock(q, vec2(0.0, 30.0), vec2(0.0, -6.0), vec2(0.0, -46.0), 8.0, 1.5);
            animePartShine(C, bl, animeMetal(animeHex(0xDCE6F5u)), animeCover(C, abs(q.x) - 1.2) * 0.8, vec3(1.0));
            animePart(C, sdRoundBox(q - vec2(0.0, 34.0), vec2(14.0, 3.5), 3.0), animeMetal(a));
        }
    } else if (id == 1) {
        // Hyper jump: a winged boot on an upward burst.
        float burst = sdTriangle(P, vec2(-30.0, 44.0), vec2(30.0, 44.0), vec2(0.0, -10.0));
        C.color = add(C.color, b, (1.0 - smoothstep(-1.0, 1.0, burst)) * 0.5);
        AnimeMaterial m = animeMaterial(a);
        animePart(C, sdRoundBox(P - vec2(-4.0, -4.0), vec2(12.0, 18.0), 5.0), m);
        animePart(C, sdRoundBox(P - vec2(4.0, 16.0), vec2(22.0, 10.0), 9.0), m);
        animePart(C, sdLockBelly(P, vec2(-14.0, -10.0), vec2(-30.0, -26.0), vec2(-44.0, -20.0), 3.0, 10.0), animeMaterial(animeHex(0xF7F1E3u)));
    } else if (id == 2) {
        // Dodge roll: a curled arrow.
        float r = abs(length(P) - 30.0) - 8.0;
        r = max(r, -max(P.x, -P.y - 6.0));
        animePart(C, r, animeMaterial(a));
        animePart(C, sdTriangle(P, vec2(-4.0, -50.0), vec2(-4.0, -12.0), vec2(26.0, -32.0)), animeMaterial(a));
    } else if (id == 3) {
        // Crescent sweep.
        float moon = max(sdCircle(P, 44.0), -sdCircle(P - vec2(16.0, -10.0), 38.0));
        C.color = add(C.color, b * 1.3, glowFalloff(moon, 14.0) * 0.6);
        animePart(C, moon, animeGlow(a));
    } else if (id == 4) {
        float d = star5(P, 46.0, 20.0);
        C.color = add(C.color, b * 1.4, glowFalloff(d, 16.0) * 0.7);
        animePart(C, d, animeGlow(a));
        animeFlat(C, star5(P, 16.0, 7.0), vec3(1.0));
    } else if (id == 5) {
        // Tailwind: three swirls.
        for (int i = 0; i < 3; ++i) {
            float y = -26.0 + float(i) * 24.0;
            float d = sdBezier(P, vec2(-42.0, y), vec2(10.0, y - 14.0), vec2(30.0, y + 2.0));
            animeStroke(C, d, 5.0, i == 1 ? a : b);
            animeStroke(C, max(abs(length(P - vec2(32.0, y - 8.0)) - 9.0), -(P.y - y + 8.0)), 4.0, i == 1 ? a : b);
        }
    } else {
        // Bloom: a five-petal flower.
        for (int k = 0; k < 5; ++k) {
            float ang = float(k) * 1.2566 - 1.57;
            animePart(C, sdCircle(P - vec2(cos(ang), sin(ang)) * 24.0, 20.0), animeMaterial(a));
        }
        animePart(C, sdCircle(P, 14.0), animeMaterial(b));
    }
}

void statIcon(int id)
{
    if (id == 0) {
        // Strength: a flexed arm... as a fist.
        AnimeMaterial m = animeSkin(animeHex(0xF8D2B6u));
        float fist = sdRoundBox(P - vec2(0.0, 4.0), vec2(30.0, 26.0), 12.0);
        animePart(C, fist, m);
        for (int i = 0; i < 3; ++i) {
            animeLine(C, max(abs(P.x - (-10.0 + float(i) * 10.0)), max(fist + 1.0, P.y + 4.0)), m.ink);
        }
        animePart(C, sdRoundBox(P - vec2(0.0, 40.0), vec2(20.0, 10.0), 5.0), animeCloth(animeHex(0xEC5662u)));
    } else if (id == 1) {
        // Agility: a feather.
        float d = sdLockBelly(P, vec2(-26.0, 40.0), vec2(0.0, 0.0), vec2(30.0, -44.0), 4.0, 16.0);
        animePart(C, d, animeMaterial(animeHex(0x5CC8B0u)));
        animeLine(C, max(sdBezier(P, vec2(-28.0, 44.0), vec2(2.0, 4.0), vec2(28.0, -40.0)), d + 0.5), animeHex(0x2C6B5Eu));
    } else if (id == 2) {
        // Vitality: a heart.
        vec2 q = vec2(abs(P.x), -P.y + 6.0);
        float d = sdCircle(q - vec2(18.0, 14.0), 22.0);
        d = min(d, sdTriangle(vec2(P.x, P.y - 6.0), vec2(-38.0, -4.0), vec2(38.0, -4.0), vec2(0.0, 40.0)) - 2.0);
        animePart(C, d, animeMaterial(animeHex(0xEC5662u)));
        animeGloss(C, P, d, vec2(-18.0, -16.0), vec2(7.0, 5.0), 0.6);
    } else {
        // Magic: a sparkle.
        vec2 q = abs(P);
        float d = min(sdTriangle(q, vec2(0.0, 0.0), vec2(12.0, 0.0), vec2(0.0, 48.0)), sdTriangle(q, vec2(0.0, 0.0), vec2(48.0, 0.0), vec2(0.0, 12.0))) - 3.0;
        C.color = add(C.color, animeHex(0xB79CFFu) * 1.3, glowFalloff(d, 12.0) * 0.5);
        animePart(C, d, animeGlow(animeHex(0xB79CFFu)));
    }
}

void marker(int kind, vec3 color)
{
    // A rounded bubble with "!" (available), "?" (ready) or "..." (in progress).
    float bubble = sdCircle(P - vec2(0.0, -8.0), 40.0);
    bubble = min(bubble, sdTriangle(P, vec2(-10.0, 24.0), vec2(10.0, 24.0), vec2(0.0, 46.0)));
    animePart(C, bubble, animeMaterial(color));
    vec3 ink = animeInkOf(color);
    if (kind == 0) {
        animeFlat(C, sdRoundBox(P - vec2(0.0, -18.0), vec2(6.0, 16.0), 6.0), vec3(1.0));
        animeFlat(C, sdCircle(P - vec2(0.0, 12.0), 6.5), vec3(1.0));
    } else if (kind == 2) {
        float q = abs(length(P - vec2(0.0, -22.0)) - 14.0) - 5.5;
        q = (P.x < 0.0 && P.y > -22.0) ? 1e5 : q;
        animeFlat(C, q, vec3(1.0));
        animeFlat(C, sdRoundBox(P - vec2(4.0, -2.0), vec2(5.5, 7.0), 3.0), vec3(1.0));
        animeFlat(C, sdCircle(P - vec2(4.0, 14.0), 6.0), vec3(1.0));
    } else {
        for (int i = -1; i <= 1; ++i) {
            animeFlat(C, sdCircle(P - vec2(float(i) * 16.0, -8.0), 6.0), vec3(1.0));
        }
    }
}

// A chunky digit from round strokes on a 2 x 3 grid of points.
float digitShape(int d, vec2 p)
{
    // Corners: tl tr ml mr bl br.
    vec2 tl = vec2(-22.0, -40.0), tr = vec2(22.0, -40.0), ml = vec2(-22.0, 0.0), mr = vec2(22.0, 0.0), bl = vec2(-22.0, 40.0),
         br = vec2(22.0, 40.0);
    float r = 9.0;
    float x = 1e5;
    if (d == 0) {
        x = sdRoundBox(p, vec2(22.0, 40.0), 22.0);
        return abs(x) - r;
    }
    if (d == 1) {
        x = min(sdSegment(p, vec2(4.0, -40.0), vec2(4.0, 40.0), r), sdSegment(p, vec2(4.0, -40.0), vec2(-12.0, -26.0), r));
    } else if (d == 2) {
        float ring = abs(length(p - vec2(0.0, -18.0)) - 20.0) - r;
        ring = (p.x < -2.0 && p.y > -18.0) ? 1e5 : ring;
        x = min(min(ring, sdSegment(p, vec2(15.0, -6.0), bl, r)), sdSegment(p, bl, br, r));
    } else if (d == 3) {
        float up = abs(length(p - vec2(0.0, -20.0)) - 18.0) - r;
        float low = abs(length(p - vec2(0.0, 20.0)) - 20.0) - r;
        x = min(up, low);
        x = (p.x < -8.0 && abs(p.y) < 28.0) ? 1e5 : x;
    } else if (d == 4) {
        x = min(min(sdSegment(p, vec2(12.0, -40.0), vec2(12.0, 40.0), r), sdSegment(p, vec2(12.0, -40.0), vec2(-22.0, 14.0), r)),
                sdSegment(p, vec2(-22.0, 14.0), vec2(24.0, 14.0), r));
    } else if (d == 5) {
        float bowl = abs(length(p - vec2(0.0, 16.0)) - 22.0) - r;
        bowl = (p.x < -4.0 && p.y < 16.0) ? 1e5 : bowl;
        x = min(min(sdSegment(p, tr, tl, r), sdSegment(p, tl, vec2(-22.0, -4.0), r)), min(bowl, sdSegment(p, vec2(-22.0, -4.0), vec2(0.0, -6.0), r)));
    } else if (d == 6) {
        x = min(abs(sdCircle(p - vec2(0.0, 16.0), 22.0)) - r, sdBezier(p, vec2(16.0, -38.0), vec2(-26.0, -30.0), vec2(-22.0, 14.0)) - r);
    } else if (d == 7) {
        x = min(sdSegment(p, tl, tr, r), sdSegment(p, tr, vec2(-6.0, 40.0), r));
    } else if (d == 8) {
        x = min(abs(sdCircle(p - vec2(0.0, -20.0), 18.0)) - r, abs(sdCircle(p - vec2(0.0, 19.0), 21.0)) - r);
    } else if (d == 9) {
        x = min(abs(sdCircle(p - vec2(0.0, -16.0), 22.0)) - r, sdBezier(p, vec2(-16.0, 38.0), vec2(26.0, 30.0), vec2(22.0, -14.0)) - r);
    } else if (d == 10) {
        // "+"
        x = min(sdSegment(p, vec2(-24.0, 0.0), vec2(24.0, 0.0), r), sdSegment(p, vec2(0.0, -24.0), vec2(0.0, 24.0), r));
    }
    return x;
}

void main()
{
    SpriteData s = sprite;
    vec2 local = spriteLocal(inUv);
    vec2 h = s.size * 0.5;
    int kind = int(s.params.x + 0.5);
    vec3 a = s.color.rgb;
    vec3 b = s.color2.rgb;
    vec4 c = vec4(0.0);
    if (kind == 0 || kind == 13) {
        c = window(local, h, s.params.y, kind == 13 ? 1 : int(s.params.z + 0.5), a, b);
    } else if (kind == 1) {
        c = button(local, h, int(s.params.y + 0.5), a, b);
    } else if (kind == 2) {
        c = slot(local, h, s.params.y, s.params.z, a, b);
    } else if (kind == 3) {
        c = gauge(local, h, s.params.y, s.params.z, a, b);
    } else if (kind == 14) {
        float d = length(local / h);
        c = vec4(a, (1.0 - smoothstep(0.3, 1.0, d)) * s.color.a);
        outColor = c;
        return;
    } else if (kind == 11) {
        float px = max(length(dFdx(local)), 1e-4);
        float d = length(local) - min(h.x, h.y) + 4.0;
        c = add(c, a, 1.0 - smoothstep(-px, px, d));
        c = add(c, b, 1.0 - smoothstep(1.5, 3.5, abs(d)));
    } else if (kind == 12) {
        float px = max(length(dFdx(local)), 1e-4);
        float d = sdTriangle(local, vec2(-h.x * 0.6, -h.y * 0.8), vec2(-h.x * 0.6, h.y * 0.8), vec2(h.x * 0.8, 0.0)) - 2.0;
        c = add(c, a, 1.0 - smoothstep(-px, px, d));
        c = add(c, animeInkOf(a), 1.0 - smoothstep(0.8, 2.0, abs(d)));
    } else {
        P = local / (s.size.y / 128.0);
        C = animeBegin(P, 5.0);
        C.shadeDepth = 9.0;
        int id = int(s.params.y + 0.5);
        int style = int(s.params.z + 0.5);
        if (kind == 4) {
            if (id == 0) {
                potion(a);
            } else if (id == 1) {
                scroll(a);
            } else if (id == 2) {
                material(style, a, b);
            } else if (id == 3) {
                weapon(style, a, b);
            } else if (id == 4) {
                hat(style, a, b);
            } else if (id == 5) {
                top(style, a, b);
            } else if (id == 6) {
                shoes(style, a);
            } else if (id == 7) {
                ring(a);
            } else if (id == 8) {
                letter();
            } else if (id == 9) {
                charm(a);
            } else {
                coin(a);
            }
        } else if (kind == 5) {
            skillIcon(id, a, b);
        } else if (kind == 6) {
            statIcon(id);
        } else if (kind == 7) {
            marker(id, a);
        } else if (kind == 9) {
            coin(a);
        } else if (kind == 10) {
            float d = sdRoundBox(P, vec2(s.size.x / (s.size.y / 128.0) * 0.5 - 6.0, 56.0), 18.0);
            animePart(C, d, animeMaterial(a));
            animeFlat(C, max(d + 8.0, P.y + 10.0), mix(a, vec3(1.0), 0.4));
        } else if (kind == 8) {
            // A damage digit: two-tone fill, a dark outline and a light top edge.
            float d = digitShape(id, P);
            float px = C.px;
            vec3 ink = animeInkOf(b) * 0.7;
            c = add(c, ink, 1.0 - smoothstep(-px, px, d - 7.0));
            vec3 fill = mix(a, b, smoothstep(-30.0, 30.0, P.y));
            c = add(c, fill, 1.0 - smoothstep(-px, px, d));
            c = add(c, mix(a, vec3(1.0), 0.6), (1.0 - smoothstep(-px, px, d + 3.0)) * (1.0 - smoothstep(-40.0, -20.0, P.y)) * 0.6);
            c.a *= s.color.a;
            outColor = c;
            return;
        }
        if (kind == 5 && s.params.w > 0.5) {
            // Not learned: a dimmed icon.
            vec4 f = animeFinish(C);
            float g = dot(f.rgb, vec3(0.3, 0.55, 0.15));
            outColor = vec4(mix(f.rgb, vec3(g) * 0.7, 0.85), f.a * s.color.a);
            return;
        }
        c = animeFinish(C);
    }
    c.a *= s.color.a;
    outColor = c;
}
