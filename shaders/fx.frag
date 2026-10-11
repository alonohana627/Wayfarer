#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_monster.glsl"
#include "gamelib/noise.glsl"

// Wayfarer's effects: big, readable and soft - sword arcs, the crescent
// sweep, the starbolt, hit sparks, bloom petals, the tailwind's swirl,
// portals, shockwaves, the guardians' rain warnings and shots, the level-up
// pillar, coins, loot glows, dust, the hyper jump's streak and heal sparkles.
// Colors may go above 1 for the bloom pass.
//
// Design space: the quad is square, 128 units across.
//   color   core color        color2  edge / glow color
//   params  x kind, y progress 0..1 (or a phase), z variant, w facing or seed

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265;

vec4 add(vec4 acc, vec3 rgb, float a)
{
    return over(vec4(rgb, clamp(a, 0.0, 1.0)), acc);
}

float star5(vec2 p, float r, float inner)
{
    float a = atan(p.x, -p.y);
    float seg = 2.0 * PI / 5.0;
    float m = mod(a + seg * 0.5, seg) - seg * 0.5;
    float rr = mix(inner, r, pow(abs(cos(m * 2.5)), 3.0));
    return length(p) - rr;
}

// A crescent arc: the band between two circles, cut to an angle range, fading along it.
vec4 arc(vec2 p, float progress, float startA, float endA, float radius, float thick, vec3 core, vec3 edge)
{
    float head = mix(startA, endA, smoothstep(0.0, 0.45, progress));
    float tail = mix(startA, endA, smoothstep(0.15, 1.0, progress));
    float a = atan(p.y, p.x);
    float dir = sign(endA - startA);
    // Unwrap a towards the arc's range.
    float mid = (startA + endA) * 0.5;
    a = mid + mod(a - mid + PI, 2.0 * PI) - PI;
    float lo = min(head, tail);
    float hi = max(head, tail);
    float inside = step(lo, a) * step(a, hi);
    float t = clamp((a - tail) / max(head - tail, 1e-3) * dir, 0.0, 1.0);
    t = dir > 0.0 ? clamp((a - tail) / max(head - tail, 1e-3), 0.0, 1.0) : clamp((tail - a) / max(tail - head, 1e-3), 0.0, 1.0);
    float r = length(p);
    float w = thick * (0.25 + 0.75 * t); // thickest at the head
    float band = 1.0 - smoothstep(w * 0.5 - 1.0, w * 0.5 + 1.0, abs(r - radius));
    float fade = (1.0 - smoothstep(0.6, 1.0, progress));
    vec4 c = vec4(0.0);
    c = add(c, edge, band * inside * fade * 0.9 * (0.3 + 0.7 * t));
    float coreBand = 1.0 - smoothstep(w * 0.18 - 0.8, w * 0.18 + 0.8, abs(r - radius - w * 0.1));
    c = add(c, core, coreBand * inside * fade * t);
    return c;
}

void main()
{
    SpriteData s = sprite;
    vec2 p = spriteLocal(inUv) / (s.size.y / 128.0);
    int kind = int(s.params.x + 0.5);
    float t = clamp(s.params.y, 0.0, 1.0);
    float variant = s.params.z;
    float w = s.params.w;
    vec3 core = s.color.rgb;
    vec3 edge = s.color2.rgb;
    vec4 c = vec4(0.0);
    float time = s.time;

    if (kind == 0) {
        // Sword arcs: 0 a downward slash, 1 a rising slash, 2 a wide spin.
        vec2 q = vec2(p.x * (w < 0.0 ? -1.0 : 1.0), p.y);
        if (variant < 0.5) {
            c = arc(q, t, -2.2, 0.9, 44.0, 22.0, core, edge);
        } else if (variant < 1.5) {
            c = arc(q, t, 1.0, -1.9, 42.0, 22.0, core, edge);
        } else {
            c = arc(q, t, -2.9, 1.4, 50.0, 28.0, core, edge);
        }
    } else if (kind == 1) {
        // The crescent sweep: two full rings of light around the hero.
        float spin = t * 2.0;
        vec4 a1 = arc(p, fract(spin), -PI, PI, 52.0, 26.0, core, edge);
        vec4 a2 = arc(vec2(-p.x, p.y), fract(spin + 0.5), -PI, PI, 40.0, 18.0, core, edge);
        c = over(a2, a1);
        float ring = 1.0 - smoothstep(1.5, 3.0, abs(length(p) - 58.0 * smoothstep(0.0, 1.0, t)));
        c = add(c, edge, ring * (1.0 - t) * 0.6);
    } else if (kind == 2) {
        // The starbolt: a spinning five-point star in a glow with a white heart.
        vec2 q = rotate2d(p, time * 6.0 + w);
        float d = star5(q, 30.0, 15.0) - 3.0;
        c = add(c, edge, glowFalloff(d, 30.0) * 0.8);
        c = add(c, core, 1.0 - smoothstep(-1.0, 1.0, d));
        c = add(c, vec3(1.3), 1.0 - smoothstep(-1.0, 1.0, star5(q, 12.0, 6.0)));
    } else if (kind == 3) {
        // Hit spark: a burst of rays, a flash and a ring.
        float a = atan(p.y, p.x);
        float rays = pow(abs(cos(a * 4.0 + w)), 18.0);
        float r = length(p);
        float reach = 20.0 + 44.0 * smoothstep(0.0, 0.5, t);
        float ray = rays * (1.0 - smoothstep(reach * 0.6, reach, r)) * step(reach * 0.15 * t, r);
        c = add(c, edge, ray * (1.0 - t));
        c = add(c, core, (1.0 - smoothstep(4.0, 22.0 * (1.0 - t * 0.8), r)) * (1.0 - t));
        c = add(c, edge, (1.0 - smoothstep(1.0, 3.0, abs(r - reach * 0.9))) * (1.0 - t) * 0.7);
    } else if (kind == 4) {
        // A bloom petal.
        vec2 q = rotate2d(p, w + time * 2.0);
        float d = sdEllipse(q, vec2(18.0, 36.0));
        d = max(d, -sdCircle(q - vec2(0.0, -40.0), 9.0));
        c = add(c, edge, glowFalloff(d, 10.0) * 0.4 * (1.0 - t));
        c = add(c, mix(core, vec3(1.0), smoothstep(10.0, -20.0, q.y) * 0.5), (1.0 - smoothstep(-1.0, 1.0, d)) * (1.0 - smoothstep(0.7, 1.0, t)));
    } else if (kind == 5) {
        // The tailwind: streaks swirling round the hero.
        float a = atan(p.y * 1.6, p.x);
        float r = length(vec2(p.x, p.y * 1.6));
        float swirl = fract(a / (2.0 * PI) * 3.0 - time * 1.6);
        float streak = smoothstep(0.0, 0.6, swirl) * (1.0 - smoothstep(0.6, 0.75, swirl));
        float band = 1.0 - smoothstep(4.0, 9.0, abs(r - 52.0 - sin(a * 3.0 + time) * 4.0));
        c = add(c, core, streak * band * 0.85 * s.color.a);
        c = add(c, edge, band * 0.18 * s.color.a);
    } else if (kind == 6) {
        // A portal: a soft oval of light with rising sparkles; variant 1 = closed.
        vec2 q = vec2(p.x * 1.8, p.y);
        float r = length(q);
        float open = clamp(t, 0.0, 1.0);
        float ring = abs(r - 46.0 - sin(atan(q.y, q.x) * 6.0 + time * 3.0) * 2.0);
        float swirl = fract(atan(q.y, q.x) / (2.0 * PI) * 2.0 + time * 0.6 - r * 0.01);
        c = add(c, edge, glowFalloff(ring - 4.0, 18.0) * 0.6 * open);
        c = add(c, core, (1.0 - smoothstep(2.0, 4.5, ring)) * open);
        c = add(c, mix(edge, vec3(1.0), 0.3), (1.0 - smoothstep(0.0, 46.0, r)) * (0.25 + 0.2 * smoothstep(0.3, 0.7, swirl)) * open);
        for (int i = 0; i < 6; ++i) {
            float fi = float(i);
            float life = fract(time * 0.5 + fi / 6.0);
            vec2 sp = vec2((hashCell(vec2(fi, floor(time * 0.5 + fi / 6.0))) - 0.5) * 50.0, 40.0 - life * 100.0);
            c = add(c, vec3(1.2), (1.0 - smoothstep(1.0, 3.0, length(p - sp))) * (1.0 - life) * open);
        }
        c.a *= s.color.a;
    } else if (kind == 7) {
        // A shockwave ring.
        float r = length(vec2(p.x, p.y * 2.2));
        float radius = 10.0 + 56.0 * smoothstep(0.0, 1.0, t);
        c = add(c, edge, (1.0 - smoothstep(2.0, 6.0 * (1.0 - t) + 2.0, abs(r - radius))) * (1.0 - t));
    } else if (kind == 8) {
        // A rain warning: a soft column and a target ring, pulsing faster as it nears.
        float pulse = 0.5 + 0.5 * sin(time * mix(6.0, 22.0, t));
        float col = (1.0 - smoothstep(10.0, 22.0, abs(p.x))) * smoothstep(-64.0, 40.0, p.y);
        c = add(c, edge, col * 0.25 * (0.5 + 0.5 * t));
        vec2 q = vec2(p.x, (p.y - 52.0) * 3.0);
        float ring = abs(length(q) - 26.0 * (1.2 - t * 0.4));
        c = add(c, core, (1.0 - smoothstep(1.5, 4.0, ring)) * (0.4 + 0.6 * pulse));
    } else if (kind == 9) {
        // The guardians' shots by look.
        int look = int(variant + 0.5);
        vec2 q = rotate2d(p, w + time * (look == 1 ? 4.0 : 2.0));
        AnimeCanvas C = animeBegin(p, 4.0);
        if (look == 0) {
            // A spore: a fuzzy round puff with dots.
            float d = sdCircle(p, 30.0 + sin(time * 8.0) * 2.0);
            C.color = add(C.color, edge, glowFalloff(d, 16.0) * 0.5);
            animePart(C, d, animeMaterial(core));
            animePartNoLine(C, max(animeSpots(q, 14.0, 4.0, 2.0), d + 3.0), animeMaterial(edge));
        } else if (look == 1) {
            float d = sdCircle(q, 34.0) + valueNoise(q * 0.08) * 8.0;
            animePart(C, d, animeMatte(core));
            animeFlat(C, max(d + 3.0, voronoi(q * 0.05).x * 20.0 - 2.0 + 100.0 * step(0.0, -q.y)), edge * 1.6);
        } else if (look == 2) {
            float d = sdCircle(p, 30.0);
            C.color = add(C.color, core, (1.0 - smoothstep(-2.0, 0.0, d)) * 0.35);
            animeStroke(C, abs(d), 2.5, mix(core, vec3(1.0), 0.5));
            animeFlat(C, sdEllipse(p - vec2(-12.0, -12.0), vec2(7.0, 5.0)), vec3(1.0));
        } else if (look == 3) {
            vec2 f = vec2(p.x, p.y);
            float d = sdLock(f, vec2(0.0, 14.0), vec2(0.0, -4.0), vec2(sin(time * 9.0) * 6.0, -40.0), 24.0, 2.0);
            C.color = add(C.color, edge, glowFalloff(d, 22.0) * 0.6);
            animeFlat(C, d, core * 1.4);
            animeFlat(C, d + 10.0, vec3(1.3, 1.2, 0.9));
        } else if (look == 4) {
            float d = star5(q, 34.0, 12.0);
            C.color = add(C.color, edge, glowFalloff(d, 18.0) * 0.6);
            animePart(C, d, animeGlow(core));
        } else if (look == 5) {
            // A ground wave: a rolling crest.
            float d = max(sdEllipse(p - vec2(0.0, 30.0), vec2(56.0, 50.0)), p.y - 60.0);
            C.color = add(C.color, edge, glowFalloff(d, 14.0) * 0.6);
            animeFlat(C, d, core * 1.2);
            animeFlat(C, max(d + 8.0, -(p.y - 20.0)), mix(core, vec3(1.0), 0.5));
        } else {
            float d = star5(q, 30.0, 14.0);
            animePart(C, d, animeGlow(core));
        }
        c = animeFinish(C);
    } else if (kind == 10) {
        // Level up: a pillar of light, rising rings and sparkles.
        float fade = 1.0 - smoothstep(0.7, 1.0, t);
        float pillar = (1.0 - smoothstep(14.0, 34.0 * (1.0 - t * 0.5), abs(p.x))) * smoothstep(64.0, 20.0, p.y);
        c = add(c, edge, pillar * 0.55 * fade);
        for (int i = 0; i < 3; ++i) {
            float rt = fract(t * 1.5 + float(i) / 3.0);
            float ry = 50.0 - rt * 110.0;
            float ring = abs(length(vec2(p.x, (p.y - ry) * 3.5)) - 30.0);
            c = add(c, core, (1.0 - smoothstep(1.0, 3.0, ring)) * (1.0 - rt) * fade);
        }
        for (int i = 0; i < 10; ++i) {
            float fi = float(i);
            float life = fract(t * 2.0 + fi * 0.1);
            vec2 sp = vec2((hashCell(vec2(fi, 9.0)) - 0.5) * 80.0, 50.0 - life * 120.0);
            c = add(c, vec3(1.4, 1.3, 0.9), (1.0 - smoothstep(0.0, 2.5, length(p - sp))) * (1.0 - life) * fade);
        }
    } else if (kind == 11) {
        // A coin, spinning.
        float spin = cos(t * 2.0 * PI);
        vec2 q = vec2(p.x / max(abs(spin), 0.15), p.y);
        AnimeCanvas C = animeBegin(p, 5.0);
        float d = sdCircle(q, 44.0) * max(abs(spin), 0.15);
        animePart(C, d, animeMetal(core));
        animeLine(C, max(abs(sdCircle(q, 30.0)) * max(abs(spin), 0.15), d + 2.0), animeInkOf(core));
        animeFlat(C, max(sdEllipse(q - vec2(-12.0, -14.0), vec2(8.0, 12.0)) * max(abs(spin), 0.15), d + 4.0), vec3(1.2, 1.15, 0.9));
        c = animeFinish(C);
    } else if (kind == 12) {
        // Loot glow: a soft halo and a thin rising beam in the rarity color.
        float r = length(p);
        float pulse = 0.75 + 0.25 * sin(time * 3.0 + w);
        c = add(c, core, (1.0 - smoothstep(0.0, 56.0, r)) * 0.5 * pulse);
        float beam = (1.0 - smoothstep(2.0, 8.0, abs(p.x))) * smoothstep(64.0, -20.0, -p.y + 40.0) * variant;
        c = add(c, edge, beam * 0.5 * pulse);
    } else if (kind == 13) {
        // A dust puff.
        float d = sdCircle(p, 40.0 * (0.6 + 0.4 * t));
        c = add(c, core, (1.0 - smoothstep(-20.0, 0.0, d)) * (1.0 - t) * 0.8);
    } else if (kind == 14) {
        // A streak (the hyper jump's trail): a soft capsule along +x.
        float d = sdSegment(p, vec2(-56.0, 0.0), vec2(56.0, 0.0), 16.0 * (0.3 + 0.7 * smoothstep(-64.0, 64.0, p.x)));
        c = add(c, edge, glowFalloff(d, 14.0) * 0.6 * (1.0 - t));
        c = add(c, core, (1.0 - smoothstep(-2.0, 2.0, d)) * (1.0 - t) * smoothstep(-64.0, 40.0, p.x));
    } else {
        // A heal sparkle: a soft plus.
        vec2 q = abs(p);
        float d = min(sdRoundBox(q, vec2(10.0, 34.0), 8.0), sdRoundBox(q, vec2(34.0, 10.0), 8.0));
        c = add(c, edge, glowFalloff(d, 16.0) * 0.6 * (1.0 - t));
        c = add(c, core, (1.0 - smoothstep(-1.0, 1.0, d)) * (1.0 - t));
    }
    c.a *= kind == 5 || kind == 6 ? 1.0 : s.color.a;
    outColor = c;
}
