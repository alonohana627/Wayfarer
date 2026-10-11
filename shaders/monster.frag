#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_monster.glsl"
#include "gamelib/noise.glsl"

// Sample: six cute, rounded monster archetypes in the MapleStory-style chibi
// look - original designs, each with its own silhouette and one feature that
// breaks the outline - sharing gamelib/anime_monster.glsl's bouncy states.
// Elites keep the silhouette and add a crown and an aura (draw the quad 1.3x
// larger and pass a deeper palette).
//
// Design space: 128 x 128 units over the quad height, feet on y = +50,
// facing +x. anime_samples.hpp packs the fields.
//   color   body color          color2  accent (cap, shell, ears, leaves)
//   params  x kind (0 bellcap: capped fungus, 1 mochi: jelly blob,
//           2 turbanshell: shelled crawler, 3 pudgy: round hog, 4 stumpling:
//           walking stump, 5 wisp: sheet ghost), y state (0 idle, 1 move,
//           2 wind-up, 3 hit, 4 defeat), z progress 0..1 through the state
//           (one hop / step cycle while moving), w facing (-1, 1)
//   bits.x  bit 0 elite, 8-15 seed (staggers idle loops), 16-19 elite trait variant

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

AnimeCanvas C;
vec3 gIris;
vec3 gEyeInk;
int gVariant;   // the species' elite trait
float gTime;
float gSeed;

// The face kit's expressions for a monster mood, so creatures share the
// hero's eyes: an almond white, a two-tone iris, a pupil, two highlights, a
// lash line, blinking.
AnimeFace moodFace(int mood)
{
    if (mood == kAnimeMoodHappy) {
        return animeExpression(kAnimeHappy);
    }
    if (mood == kAnimeMoodAngry) {
        return animeExpression(kAnimeAngry);
    }
    if (mood == kAnimeMoodHurt) {
        return animeExpression(kAnimeHurt);
    }
    if (mood == kAnimeMoodKnocked) {
        return animeExpression(kAnimeKnocked);
    }
    if (mood == kAnimeMoodSleepy) {
        return animeExpression(kAnimeBlink);
    }
    return animeExpression(kAnimeNeutral);
}

// One eye of a creature: `size` is the old dot eye's half height.
void critterEye(vec2 q, vec2 c, float size, int mood, float side)
{
    AnimeFace f = moodFace(mood);
    if (f.eyeShape == kAnimeEyeOpen) {
        f.open *= animeBlink(gTime, gSeed);
        if (f.open < 0.12) {
            f.eyeShape = kAnimeEyeClosed;
        }
    }
    f.look = vec2(0.35, 0.1);
    float h = size * 3.0;
    animeEye(C, q, c, vec2(h * 0.8, h), side, f, gIris, gEyeInk);
    if (mood == kAnimeMoodAngry) {
        animeBrow(C, q, c + vec2(0.0, -h * 0.78), h * 0.8, side, 1.0, gEyeInk);
    }
}

const int kBellcap = 0, kMochi = 1, kTurbanshell = 2, kPudgy = 3, kStumpling = 4, kWisp = 5, kPebblet = 6, kFlitter = 7;

void main()
{
    SpriteData s = sprite;
    float k = s.size.y / 128.0;
    float facing = s.params.w < 0.0 ? -1.0 : 1.0;
    vec2 local = spriteLocal(inUv);
    vec2 p = vec2(local.x * facing, local.y) / k;
    int kind = int(s.params.x + 0.5);
    int state = int(s.params.y + 0.5);
    float progress = s.params.z;
    bool elite = (s.bits.x & 1u) != 0u;
    float seed = float((s.bits.x >> 8) & 0xffu) / 255.0;
    float time = s.time + seed * 10.0;
    vec3 bodyColor = s.color.rgb;
    vec3 accent = s.color2.rgb;
    vec2 feet = vec2(0.0, 50.0);

    C = animeBegin(p, 3.4);
    C.shadeDepth = 6.0;
    AnimeMotion m = animeMonsterMotion(state, progress, time, kind == kWisp || kind == kFlitter ? 0.0 : 1.0);
    if (state == kAnimeMove && (kind == kPudgy || kind == kStumpling || kind == kTurbanshell)) {
        // Walkers: a trot bob instead of a hop.
        float w = sin(progress * 2.0 * kAnimePi);
        m.offset.y = -abs(w) * (kind == kTurbanshell ? 0.0 : 3.0);
        m.squash = kind == kTurbanshell ? 1.0 + 0.06 * w : 1.0 + 0.04 * abs(w);
        m.tilt = kind == kStumpling ? 0.12 * w : 0.04 * w;
    }
    if (kind == kBellcap) {
        m.tilt *= 0.75; // tall: topples less, so the cap stays in the quad
    }
    if (kind == kWisp || kind == kFlitter) {
        m.offset.y += -8.0 + sin(time * 2.2) * 4.0; // floats
    }
    if (state == kAnimeMove && m.mood == kAnimeMoodCalm && kind != kWisp) {
        m.mood = kAnimeMoodHappy * int(fract(seed * 3.0) > 0.5);
    }
    vec2 q = animeApplyMotion(p, m, feet);
    vec3 ink = animeInkOf(bodyColor * 0.6 + accent * 0.4);
    gIris = animeSaturate(mix(animeShadowTone(accent, 0.45), animeHex(0x6A3FA0u), 0.25), 1.6);
    gEyeInk = animeInkOf(gIris * 0.3 + vec3(0.12, 0.05, 0.06));
    gTime = time;
    gVariant = int((s.bits.x >> 16) & 15u);
    gSeed = seed;

    if (kind != kWisp && kind != kFlitter) {
        animeGroundShadow(C, p, feet, kind == kPudgy ? 34.0 : 30.0, m);
    }

    if (kind == kBellcap) {
        // A short stalk body with the face, two stubby feet and a tall bell
        // cap with a frilled rim, cream stripe bands and a round knob.
        AnimeMaterial stalk = animeMaterial(bodyColor);
        AnimeMaterial cap = animeMaterial(accent);
        float stepPh = sin(progress * 2.0 * kAnimePi);
        animePart(C, sdEllipse(q - feet - vec2(-9.0, -3.5), vec2(7.5, 4.5)), animeTinted(stalk, vec3(0.85, 0.85, 0.92)));
        animePart(C, sdEllipse(q - feet - vec2(9.0, -3.5 - max(stepPh, 0.0) * 2.0), vec2(7.5, 4.5)), stalk);
        float body = sdRoundBox(q - vec2(0.0, 28.0), vec2(18.0, 19.0), 14.0);
        animePart(C, body, stalk);
        animeGloss(C, q, body, vec2(-8.0, 24.0), vec2(5.0, 7.0), 0.35);
        // The cap bobs a little behind the body and tilts back on the wind-up.
        vec2 rim = vec2(0.0, 13.0 + sin(time * 3.4 + 0.6) * (state == kAnimeIdle ? 1.2 : 0.0));
        vec2 cq = rotate2d(q - rim, (state == kAnimeWindup ? 0.22 * smoothstep(0.0, 0.7, progress) : 0.0)) + rim;
        float capD = sdBellCap(cq, rim, 34.0, 40.0, 7.0);
        animeCastShadow(C, body, sdBellCap(q - vec2(1.0, 4.0), rim, 32.0, 40.0, 7.0), stalk);
        critterEye(q, vec2(-6.0, 27.0), 4.8, m.mood, -1.0);
        critterEye(q, vec2(9.0, 27.0), 4.8, m.mood, 1.0);
        animeBlush(C, q, vec2(-12.0, 35.0), vec2(4.0, 2.0), 0.7, ink);
        animeBlush(C, q, vec2(15.0, 35.0), vec2(4.0, 2.0), 0.7, ink);
        animeMonsterMouth(C, q, vec2(1.5, 37.0), 7.0, m.mood, ink);
        float knob = sdCircle(cq - rim - vec2(0.0, -42.0), 5.0);
        animePart(C, knob, animeMaterial(animeHex(0xFFF3D6u)));
        animePart(C, capD, cap);
        float stripes = max(animeDomeStripes(cq, rim - vec2(0.0, 6.0), 34.0, 13.0, 4.5), capD + 2.5);
        animePartNoLine(C, stripes, animeMaterial(animeHex(0xFFF3D6u)));
        animeGloss(C, cq, capD, rim + vec2(-11.0, -30.0), vec2(6.0, 8.0), 0.4);
        if (elite) {
            if (gVariant == 0) {
                // A ring of little flowers round the cap.
                for (int i = 0; i < 5; ++i) {
                    vec2 f = rim + vec2(-24.0 + float(i) * 12.0, -14.0 - abs(float(i) - 2.0) * 3.0);
                    vec3 petal = i % 2 == 0 ? animeHex(0xF790B6u) : animeHex(0xFFF3A0u);
                    for (int k = 0; k < 5; ++k) {
                        float a = float(k) * 1.2566;
                        animePart(C, sdCircle(cq - f - vec2(cos(a), sin(a)) * 3.2, 2.6), animeMaterial(petal));
                    }
                    animeFlat(C, sdCircle(cq - f, 1.8), animeHex(0xFAD65Eu));
                }
            } else if (gVariant == 1) {
                // A little lantern hanging from the brim.
                vec2 hook = rim + vec2(30.0, 2.0);
                animeStroke(C, sdSegment(cq, hook, hook + vec2(0.0, 8.0), 0.0), 1.0, animeHex(0x4A3F66u));
                float lamp = sdRoundBox(cq - hook - vec2(0.0, 14.0), vec2(5.0, 6.5), 2.5);
                C.color = over(vec4(vec3(1.0, 0.8, 0.4) * 1.5, glowFalloff(lamp, 10.0) * 0.6), C.color);
                animePart(C, lamp, animeGlow(animeHex(0xFFC860u)));
            } else {
                // Curled horns on the cap.
                animePart(C, sdHorn(cq, rim + vec2(-14.0, -34.0), rim + vec2(-26.0, -52.0), 5.0, 0.5), animeMaterial(animeHex(0xF3E6C8u)));
                animePart(C, sdHorn(cq, rim + vec2(12.0, -36.0), rim + vec2(24.0, -54.0), 5.0, -0.5), animeMaterial(animeHex(0xF3E6C8u)));
            }
        }
    } else if (kind == kMochi) {
        // A round jelly with a translucent core (a floating bubble inside),
        // two bobbing antennae with bubble tips and a glossy highlight.
        AnimeMaterial gel = animeMaterial(bodyColor);
        gel.shadeDepth = 1.3;
        float sway = sin(time * 2.6) * 2.5;
        for (int i = 0; i < 2; ++i) {
            vec2 base = vec2(i == 0 ? -9.0 : 9.0, 14.0);
            vec2 tip = base + vec2((i == 0 ? -6.0 : 6.0) + sway, -17.0);
            animeStroke(C, sdBezier(q, base, base + vec2(0.0, -10.0), tip), 2.6, gel.ink);
            float ball = sdCircle(q - tip, 4.2);
            animePart(C, ball, animeMaterial(mix(bodyColor, vec3(1.0), 0.45)));
            animeGloss(C, q, ball, tip + vec2(-1.3, -1.3), vec2(1.6), 0.9);
        }
        float body = sdEllipse(q - vec2(0.0, 32.0), vec2(31.0, 19.5));
        body = animeSmoothUnion(body, sdRoundBox(q - vec2(0.0, 44.0), vec2(30.0, 6.0), 6.0), 6.0);
        body = max(body, q.y - 50.0);
        animePart(C, body, gel);
        // The translucent core with a floating bubble.
        float core = sdEllipse(q - vec2(-2.0, 34.0), vec2(21.0, 11.0));
        animeFlatAlpha(C, max(core, body + 3.0), mix(bodyColor, vec3(1.0), 0.45), 0.45);
        float bubble = sdCircle(q - vec2(-14.0 + sin(time * 1.3) * 3.0, 38.0 - fract(time * 0.25) * 6.0), 2.6);
        animeStroke(C, abs(bubble), 0.9, mix(bodyColor, vec3(1.0), 0.8));
        animeGloss(C, q, body, vec2(-16.0, 22.0), vec2(6.0, 4.5), 0.75);
        critterEye(q, vec2(-8.0, 30.0), 5.0, m.mood, -1.0);
        critterEye(q, vec2(10.0, 30.0), 5.0, m.mood, 1.0);
        animeBlush(C, q, vec2(-16.0, 38.0), vec2(4.5, 2.2), 0.6, ink);
        animeBlush(C, q, vec2(18.0, 38.0), vec2(4.5, 2.2), 0.6, ink);
        animeMonsterMouth(C, q, vec2(1.0, 39.0), 7.0, m.mood, ink);
        if (elite) {
            if (gVariant == 0) {
                // A big ribbon bow.
                AnimeMaterial bow = animeCloth(animeHex(0xEC5662u));
                vec2 b = vec2(-14.0, 16.0);
                animePart(C, sdLockBelly(q, b, b + vec2(-6.0, -8.0), b + vec2(-14.0, -3.0), 2.0, 5.5), bow);
                animePart(C, sdLockBelly(q, b, b + vec2(6.0, -8.0), b + vec2(14.0, -4.0), 2.0, 5.5), bow);
                animePart(C, sdCircle(q - b, 3.0), bow);
            } else if (gVariant == 1) {
                // A flame crest.
                for (int i = 0; i < 3; ++i) {
                    vec2 base = vec2(-8.0 + float(i) * 8.0, 15.0);
                    float h = 12.0 + float(i % 2) * 6.0 + sin(time * 9.0 + float(i)) * 2.0;
                    float fl = sdLock(q, base, base + vec2(0.0, -h * 0.5), base + vec2(sin(time * 7.0 + float(i)) * 2.0, -h), 5.0, 0.6);
                    C.color = over(vec4(vec3(1.0, 0.55, 0.2) * 1.6, glowFalloff(fl, 6.0) * 0.6), C.color);
                    animeFlat(C, fl, mix(vec3(1.0, 0.5, 0.15), vec3(1.0, 0.9, 0.5), smoothstep(base.y, base.y - h, q.y)) * 1.5);
                }
            } else if (gVariant == 2) {
                // A lily pad hat with a pink bloom.
                animePart(C, max(sdEllipse(q - vec2(0.0, 13.0), vec2(20.0, 5.0)), -sdTriangle(q, vec2(0.0, 13.0), vec2(14.0, 6.0), vec2(16.0, 12.0))), animeMaterial(animeHex(0x5CB86Au)));
                for (int k = 0; k < 5; ++k) {
                    float a = float(k) * 1.2566 + 0.3;
                    animePart(C, sdEllipse(q - vec2(-4.0, 6.0) - vec2(cos(a), sin(a)) * 4.0, vec2(3.0, 4.0)), animeMaterial(animeHex(0xF790B6u)));
                }
                animeFlat(C, sdCircle(q - vec2(-4.0, 6.0), 2.0), animeHex(0xFAD65Eu));
            } else {
                // An ice-crystal hairpin.
                for (int i = 0; i < 3; ++i) {
                    vec2 base = vec2(-14.0 + float(i) * 5.0, 15.0);
                    float d = sdLock(q, base, base + vec2(float(i - 1) * 2.0, -6.0), base + vec2(float(i - 1) * 4.0, -12.0 - float(i % 2) * 4.0), 3.0, 0.5);
                    animePart(C, d, animeGlow(animeHex(0xBDEBFFu)));
                }
            }
        }
    } else if (kind == kTurbanshell) {
        // A soft body with stubby eye stalks carrying a tall turban shell:
        // three stacked rounded tiers with stripes and a little flower on top.
        // When hit it pulls its head back in.
        AnimeMaterial soft = animeMaterial(bodyColor);
        AnimeMaterial shell = animeMaterial(accent);
        float hide = state == kAnimeHit ? 1.0 - progress : (state == kAnimeDefeat ? 1.0 : 0.0);
        float stretch = state == kAnimeMove ? sin(progress * 2.0 * kAnimePi) * 3.0 : 0.0;
        vec2 head = vec2(23.0 + stretch - hide * 12.0, 32.0 + hide * 8.0);
        float body = sdRoundBox(q - vec2(2.0 + stretch * 0.5, 43.5), vec2(32.0 + stretch * 0.5, 6.5), 6.5);
        body = animeSmoothUnion(body, sdCircle(q - head, 10.5), 6.0);
        float sway = sin(time * 2.5) * 1.5;
        for (int i = 0; i < 2; ++i) {
            vec2 base = head + vec2(-2.0 + float(i) * 6.0, -6.0);
            vec2 tip = base + vec2(float(i) * 3.0 + sway, -9.0 + hide * 8.0);
            animePart(C, sdSegment(q, base, tip, 2.0), i == 0 ? animeTinted(soft, vec3(0.85, 0.85, 0.92)) : soft);
            animePart(C, sdCircle(q - tip, 2.4), soft);
        }
        animePart(C, body, soft);
        critterEye(q, head + vec2(-1.0, -1.0), 3.4, m.mood, -1.0);
        critterEye(q, head + vec2(6.5, -1.0), 3.0, m.mood, 1.0);
        animeMonsterMouth(C, q, head + vec2(3.5, 5.0), 4.5, m.mood, ink);
        vec2 sc = vec2(-8.0, 38.0);
        float tiers = 1e5;
        for (int i = 0; i < 3; ++i) {
            float w = 25.0 - float(i) * 7.0;
            tiers = min(tiers, sdEllipse(q - sc - vec2(float(i) * 2.0, -float(i) * 13.0 - 2.0), vec2(w, 9.5)));
        }
        tiers = min(tiers, sdEllipse(q - sc - vec2(6.0, -40.0), vec2(5.5, 6.0)));
        animePart(C, tiers, shell);
        for (int i = 0; i < 3; ++i) {
            float w = 25.0 - float(i) * 7.0;
            vec2 c = sc + vec2(float(i) * 2.0, -float(i) * 13.0 - 2.0);
            animeLine(C, max(abs(sdEllipse(q - c, vec2(w, 9.5))), tiers + 0.6), shell.ink);
            float band = max(sdEllipse(q - c - vec2(0.0, 1.0), vec2(w * 0.9, 3.0)), tiers + 1.6);
            animePartNoLine(C, band, animeMaterial(animeHex(0xFFF3D6u)));
        }
        animeGloss(C, q, tiers, sc + vec2(-10.0, -8.0), vec2(6.0, 4.0), 0.45);
        // The flower on top.
        vec2 fl = sc + vec2(6.0, -47.0);
        for (int i = 0; i < 5; ++i) {
            float a = float(i) * 1.2566 + time * 0.3;
            animePart(C, sdCircle(q - fl - vec2(cos(a), sin(a)) * 3.6, 2.8), animeMaterial(animeHex(0xF790B6u)));
        }
        animePart(C, sdCircle(q - fl, 2.2), animeMaterial(animeHex(0xFAD65Eu)));
        if (elite) {
            // Spikes along the shell's tiers (desert shells wear gold ones).
            for (int i = 0; i < 3; ++i) {
                float w = 25.0 - float(i) * 7.0;
                vec2 c = sc + vec2(float(i) * 2.0, -float(i) * 13.0 - 2.0);
                for (int k = -1; k <= 1; ++k) {
                    vec2 b = c + vec2(float(k) * w * 0.55, -8.0);
                    float spike = sdTriangle(q, b + vec2(-3.5, 2.0), b + vec2(3.5, 2.0), b + vec2(0.0, -6.0)) - 0.6;
                    animePart(C, spike, gVariant == 1 ? animeMetal(animeHex(0xF2C14Eu)) : animeMaterial(animeHex(0xF3E6C8u)));
                }
            }
        }
    } else if (kind == kPudgy) {
        // A round cream-and-caramel hog: a ball body with patches, round
        // bear-like ears, a hair tuft, a button snout and a curly tail.
        AnimeMaterial hide = animeMaterial(bodyColor);
        AnimeMaterial inner = animeMaterial(accent);
        float trot = sin(progress * 2.0 * kAnimePi) * (state == kAnimeMove ? 1.0 : 0.0);
        for (int i = 0; i < 2; ++i) {
            float x = i == 0 ? -16.0 : 10.0;
            animePart(C, sdRoundBox(q - vec2(x - trot * 3.0, 44.0), vec2(4.5, 6.5), 4.0), animeTinted(inner, vec3(0.82, 0.82, 0.92)));
        }
        animeStroke(C, animeSpiral(q, vec2(-31.0, 24.0), 5.0, 1.6), 2.4, hide.ink);
        float earB = sdCircle(q - vec2(-10.0, 4.0 - trot), 7.0);
        animePart(C, earB, animeTinted(inner, vec3(0.85, 0.85, 0.92)));
        float body = sdEllipse(q - vec2(0.0, 26.0), vec2(31.0, 23.0));
        animePart(C, body, hide);
        float patches = max(min(sdEllipse(q - vec2(-17.0, 22.0), vec2(11.0, 13.0)), sdCircle(q - vec2(4.0, 6.0), 7.0)), body + 1.2);
        animePartNoLine(C, patches, inner);
        animeGloss(C, q, body, vec2(-12.0, 12.0), vec2(9.0, 5.0), 0.35);
        for (int i = 0; i < 2; ++i) {
            float x = i == 0 ? -10.0 : 16.0;
            animePart(C, sdRoundBox(q - vec2(x + trot * 3.0, 45.0), vec2(4.8, 6.0), 4.0), inner);
        }
        float earF = sdCircle(q - vec2(14.0, 3.0 + trot), 7.0);
        animePart(C, earF, inner);
        animeFlat(C, sdCircle(q - vec2(14.0, 4.0 + trot), 3.5), mix(accent, animeHex(0xF5B8C4u), 0.6));
        animePart(C, sdEar(q, vec2(1.0, 4.0), vec2(-2.0, -6.0), 2.6, 0.4), inner);
        animePart(C, sdEar(q, vec2(3.0, 4.0), vec2(5.0, -5.0), 2.2, -0.4), inner);
        float snout = sdSnout(q, vec2(26.0, 30.0), vec2(7.5, 6.0));
        animePart(C, snout, animeMaterial(animeHex(0xF2B9A6u)));
        animeFlat(C, sdEllipse(q - vec2(24.0, 30.0), vec2(1.4, 2.2)), inner.ink);
        animeFlat(C, sdEllipse(q - vec2(28.5, 30.0), vec2(1.4, 2.2)), inner.ink);
        critterEye(q, vec2(10.0, 19.0), 4.6, m.mood, -1.0);
        critterEye(q, vec2(22.0, 19.0), 4.0, m.mood, 1.0);
        animeBlush(C, q, vec2(14.0, 30.0), vec2(4.0, 2.0), 0.7, ink);
        animeMonsterMouth(C, q, vec2(21.0, 39.5), 5.0, m.mood, ink);
        if (state == kAnimeMove) {
            // Dust puffs behind the trot.
            for (int i = 0; i < 3; ++i) {
                float t = fract(progress * 2.0 + float(i) / 3.0);
                float d = sdCircle(q - vec2(-30.0 - t * 14.0, 47.0 - t * 6.0), 4.0 * (1.0 - t) + 1.0);
                C.color = over(vec4(vec3(0.95, 0.92, 0.85), animeCover(C, d) * (1.0 - t) * 0.8), C.color);
            }
        }
        if (elite) {
            // A scarf (and earmuffs in the snow).
            AnimeMaterial scarf = animeCloth(gVariant == 1 ? animeHex(0x5C8CE8u) : animeHex(0xEC5662u));
            float wrap = max(sdEllipse(q - vec2(14.0, 34.0), vec2(16.0, 6.0)), -sdEllipse(q - vec2(14.0, 29.0), vec2(14.0, 4.5)));
            animePart(C, sdLock(q, vec2(2.0, 36.0), vec2(-4.0, 42.0 + sin(time * 4.0)), vec2(-10.0, 46.0), 4.0, 2.5), scarf);
            animePart(C, wrap, scarf);
            animeLine(C, max(abs(q.x - 20.0), wrap + 1.0), scarf.ink);
            if (gVariant == 1) {
                animePart(C, sdCircle(q - vec2(14.0, 3.0), 7.5), animeMatte(animeHex(0xF6F8FFu)));
                animePart(C, sdCircle(q - vec2(-10.0, 4.0), 7.0), animeMatte(animeHex(0xE2E8F6u)));
            }
        }
    } else if (kind == kStumpling) {
        // A log body with a face in the bark, a cut top with rings, a sprout,
        // branch arms and root feet.
        AnimeMaterial bark = animeMaterial(bodyColor);
        AnimeMaterial leaf = animeMaterial(accent);
        AnimeMaterial wood = animeMaterial(mix(bodyColor, animeHex(0xF1D9A8u), 0.65));
        float swing = state == kAnimeWindup ? -1.6 * smoothstep(0.0, 0.7, progress) : sin(time * 2.0) * 0.15;
        vec2 shoulderB = vec2(-17.0, 24.0);
        animePart(C, sdLock(q, shoulderB, shoulderB + vec2(-9.0, -4.0), shoulderB + rotate2d(vec2(-14.0, -10.0), -swing * 0.3), 4.0, 1.6),
                  animeTinted(bark, vec3(0.82, 0.82, 0.92)));
        for (int i = -1; i <= 1; ++i) {
            vec2 r = vec2(float(i) * 12.0, 44.0);
            float lift = state == kAnimeMove ? max(0.0, sin(progress * 2.0 * kAnimePi + float(i) * 2.0)) * 3.0 : 0.0;
            animePart(C, sdLock(q, r + vec2(0.0, -6.0), r + vec2(float(i) * 3.0, 0.0), r + vec2(float(i) * 6.0 + 2.0, 5.0 - lift), 5.0, 2.0), bark);
        }
        float log = sdRoundBox(q - vec2(0.0, 22.0), vec2(21.0, 24.0), 9.0);
        animePart(C, log, bark);
        animeLine(C, max(animeBark(q, 7.0, seed * 10.0), max(log + 2.0, -(sdEllipse(q - vec2(2.0, 28.0), vec2(14.0, 11.0))))), bark.ink);
        // The cut top with growth rings.
        vec2 topC = vec2(0.0, 0.5);
        float cut = sdEllipse(q - topC, vec2(20.0, 5.5));
        animePart(C, cut, wood);
        animeLine(C, max(abs(sdEllipse(q - topC, vec2(12.0, 3.2))), cut + 1.0), wood.ink);
        animeLine(C, max(abs(sdEllipse(q - topC, vec2(5.5, 1.5))), cut + 1.0), wood.ink);
        // The sprout.
        animeStroke(C, sdBezier(q, topC + vec2(4.0, -1.0), topC + vec2(4.0, -8.0), topC + vec2(2.0, -12.0)), 2.0, leaf.ink);
        animePart(C, sdEar(q, topC + vec2(2.5, -11.0), topC + vec2(-8.0, -18.0 + sin(time * 3.0)), 4.5, 0.25), leaf);
        animePart(C, sdEar(q, topC + vec2(3.0, -11.0), topC + vec2(13.0, -20.0 - sin(time * 3.0)), 5.0, -0.25), leaf);
        critterEye(q, vec2(-3.0, 24.0), 4.4, m.mood, -1.0);
        critterEye(q, vec2(10.0, 24.0), 4.4, m.mood, 1.0);
        animeMonsterMouth(C, q, vec2(4.0, 33.0), 6.0, m.mood, ink);
        animeBlush(C, q, vec2(-8.0, 31.0), vec2(3.5, 1.8), 0.5, ink);
        animeBlush(C, q, vec2(16.0, 31.0), vec2(3.5, 1.8), 0.5, ink);
        // The front branch arm, raised for a swing on wind-up.
        vec2 shoulderF = vec2(18.0, 24.0);
        vec2 hand = shoulderF + rotate2d(vec2(12.0, 8.0), swing);
        animePart(C, sdLock(q, shoulderF, mix(shoulderF, hand, 0.5) + vec2(2.0, 3.0), hand, 4.2, 1.8), bark);
        animePart(C, sdEar(q, hand, hand + rotate2d(vec2(9.0, -2.0), swing), 3.5, 0.3), leaf);
        if (elite) {
            // A mossy beard (an old stump grows mushrooms too).
            AnimeMaterial mossM = animeMaterial(animeHex(0x6DBE57u));
            float beard = sdCircle(q - vec2(4.0, 47.0), 6.5);
            for (int i = 0; i < 5; ++i) {
                float x = -8.0 + float(i) * 6.0;
                beard = min(beard, sdCircle(q - vec2(x, 39.0 + 3.0 * (1.0 - abs(float(i) - 2.0) / 2.0)), 5.5));
            }
            animePart(C, beard, mossM);
            if (gVariant == 1) {
                for (int i = 0; i < 2; ++i) {
                    vec2 b = vec2(-10.0 + float(i) * 18.0, -1.0);
                    animePart(C, sdSegment(q, b, b + vec2(0.0, -6.0), 1.8), animeMaterial(animeHex(0xF6E8D2u)));
                    animePart(C, sdBellCap(q, b + vec2(0.0, -5.0), 7.0, 8.0, 2.0), animeMaterial(animeHex(0xEC5662u)));
                }
            }
        }
    } else if (kind == kPebblet) {
        // A round pebble critter: a boulder body on stubby feet, a glowing
        // ore crystal growing from its back, a face on the front.
        AnimeMaterial rock = animeMatte(bodyColor);
        float trot = state == kAnimeMove ? sin(progress * 2.0 * kAnimePi) : 0.0;
        for (int i = 0; i < 2; ++i) {
            float x = i == 0 ? -12.0 : 12.0;
            float lift = i == 0 ? max(trot, 0.0) : max(-trot, 0.0);
            animePart(C, sdEllipse(q - vec2(x, 46.0 - lift * 3.0), vec2(8.0, 5.0)), i == 0 ? animeTinted(rock, vec3(0.82, 0.82, 0.92)) : rock);
        }
        float body = sdEllipse(q - vec2(0.0, 26.0), vec2(30.0, 22.0)) + valueNoise(q * 0.12 + seed * 7.0) * 2.5;
        animePart(C, body, rock);
        vec3 v = voronoi(q * 0.09 + seed * 3.0);
        animeLine(C, max(abs(v.y - v.x) * 12.0 - 0.4, body + 3.0), rock.ink);
        animeGloss(C, q, body, vec2(-12.0, 12.0), vec2(8.0, 5.0), 0.3);
        // The ore on its back.
        AnimeMaterial ore = animeGlow(accent);
        float crystal = 1e5;
        for (int i = 0; i < 3; ++i) {
            vec2 base = vec2(-10.0 + float(i) * 8.0, 8.0);
            vec2 tip = base + vec2(-4.0 + float(i) * 4.0, -16.0 - float(i % 2) * 6.0);
            crystal = min(crystal, sdLock(q, base, mix(base, tip, 0.5), tip, 5.0, 1.2));
        }
        C.color = over(vec4(accent * 1.4, glowFalloff(crystal, 10.0) * 0.5), C.color);
        animePart(C, crystal, ore);
        critterEye(q, vec2(8.0, 24.0), 4.6, m.mood, -1.0);
        critterEye(q, vec2(21.0, 24.0), 4.2, m.mood, 1.0);
        animeBlush(C, q, vec2(5.0, 33.0), vec2(4.0, 2.0), 0.6, ink);
        animeBlush(C, q, vec2(25.0, 33.0), vec2(3.5, 2.0), 0.6, ink);
        animeMonsterMouth(C, q, vec2(15.0, 35.0), 5.0, m.mood, ink);
        if (elite) {
            // A glowing gem core set into its chest.
            vec2 g = vec2(-6.0, 36.0);
            vec2 vv[kMaxPolygonVertices];
            vv[0] = g + vec2(0.0, -9.0);
            vv[1] = g + vec2(8.0, -1.0);
            vv[2] = g + vec2(0.0, 9.0);
            vv[3] = g + vec2(-8.0, -1.0);
            float gem = sdPolygon(q, vv, 4);
            C.color = over(vec4(accent * 1.5, glowFalloff(gem, 9.0) * (0.5 + 0.2 * sin(time * 3.0))), C.color);
            animePart(C, gem, animeGlow(accent));
            animeFlat(C, max(gem + 2.0, abs(q.x - g.x + 1.5 + (q.y - g.y) * 0.3) - 0.8), vec3(1.2));
        }
    } else if (kind == kFlitter) {
        // A fluffy round flier: a pom-pom body with big ears, flapping
        // wings, tiny feet and a tuft.
        AnimeMaterial fluff = animeMaterial(bodyColor);
        AnimeMaterial wing = animeMaterial(accent);
        float flap = sin(time * 14.0) * 0.5 + 0.5;
        if (state == kAnimeWindup) {
            flap = 0.1;
        }
        q.y += 8.0;
        animePart(C, sdWing(q, vec2(-10.0, 12.0), 34.0, flap, -1.0), animeTinted(wing, vec3(0.82, 0.82, 0.92)));
        animePart(C, sdEar(q, vec2(-12.0, 0.0), vec2(-18.0, -22.0), 6.0, 0.15), animeTinted(fluff, vec3(0.85, 0.85, 0.92)));
        float body = sdCircle(q - vec2(0.0, 16.0), 22.0);
        for (int i = 0; i < 8; ++i) {
            float a = float(i) * 0.785;
            body = min(body, sdCircle(q - vec2(0.0, 16.0) - vec2(cos(a), sin(a)) * 18.0, 7.0));
        }
        animePart(C, body, fluff);
        animeFlat(C, max(sdEllipse(q - vec2(6.0, 24.0), vec2(12.0, 9.0)), body + 2.0), mix(bodyColor, vec3(1.0), 0.5));
        animePart(C, sdEar(q, vec2(8.0, -2.0), vec2(12.0, -24.0), 6.0, -0.15), fluff);
        animeFlat(C, sdEar(q, vec2(8.5, -4.0), vec2(11.5, -19.0), 2.6, -0.15), mix(accent, vec3(1.0), 0.4));
        animePart(C, sdWing(q, vec2(10.0, 12.0), 34.0, flap, 1.0), wing);
        animePart(C, sdLock(q, vec2(2.0, -4.0), vec2(-2.0, -10.0), vec2(4.0, -14.0), 2.4, 0.6), fluff);
        for (int i = 0; i < 2; ++i) {
            animePart(C, sdEllipse(q - vec2(-6.0 + float(i) * 10.0, 39.0), vec2(3.0, 2.5)), animeMaterial(animeHex(0xF2B9A6u)));
        }
        critterEye(q, vec2(4.0, 14.0), 4.6, m.mood, -1.0);
        critterEye(q, vec2(16.0, 14.0), 4.2, m.mood, 1.0);
        animeBlush(C, q, vec2(1.0, 22.0), vec2(3.5, 1.8), 0.6, ink);
        animeMonsterMouth(C, q, vec2(10.0, 24.0), 4.0, m.mood, ink);
        if (elite) {
            // A crest of long feathers.
            for (int i = 0; i < 3; ++i) {
                vec2 b = vec2(4.0 + float(i) * 3.0, -6.0);
                vec2 tip = b + vec2(-14.0 - float(i) * 5.0, -22.0 + float(i) * 4.0);
                vec3 col = i == 1 ? animeHex(0xFAD65Eu) : (gVariant == 1 ? animeHex(0x7CC8A8u) : animeHex(0xF790B6u));
                animePart(C, sdLockBelly(q, b, mix(b, tip, 0.5) + vec2(0.0, -4.0), tip, 2.0, 5.0), animeMaterial(col));
            }
        }
    } else {
        // Wisp: a floating teardrop with a trailing, waving tail.
        AnimeMaterial sheet = animeMaterial(bodyColor);
        float wave = sin(time * 3.0);
        float body = sdLockBelly(q, vec2(2.0, 4.0), vec2(4.0, 30.0), vec2(-26.0 + wave * 4.0, 44.0), 22.0, 2.0);
        body = animeSmoothUnion(body, sdCircle(q - vec2(2.0, 12.0), 20.0), 6.0);
        C.color = over(vec4(mix(bodyColor, vec3(1.0), 0.5), glowFalloff(body, 8.0) * 0.35), C.color);
        animePart(C, body, sheet);
        animeLine(C, max(sdBezier(q, vec2(-12.0, 28.0), vec2(-14.0, 36.0), vec2(-22.0 + wave * 3.0, 42.0)), body + 1.0), sheet.ink);
        animeGloss(C, q, body, vec2(-8.0, 2.0), vec2(6.0, 8.0), 0.5);
        // The same eyes as everyone, and a small "o" when calm.
        int mood = m.mood;
        critterEye(q, vec2(-4.0, 12.0), 5.0, mood, -1.0);
        critterEye(q, vec2(10.0, 12.0), 5.0, mood, 1.0);
        animeBlush(C, q, vec2(-9.0, 21.0), vec2(4.0, 2.0), 0.6, ink);
        animeBlush(C, q, vec2(15.0, 21.0), vec2(4.0, 2.0), 0.6, ink);
        animeMonsterMouth(C, q, vec2(3.0, 24.0), 6.0, mood == kAnimeMoodCalm ? kAnimeMoodHurt : mood, ink);
        C.alpha = 0.92;
        if (elite) {
            if (gVariant == 0) {
                // A small lantern on a string.
                vec2 hand = vec2(22.0, 26.0);
                animeStroke(C, sdBezier(q, vec2(14.0, 18.0), vec2(22.0, 16.0), hand), 1.0, animeInkOf(bodyColor));
                float lamp = sdRoundBox(q - hand - vec2(0.0, 6.0), vec2(5.5, 7.0), 2.5);
                C.color = over(vec4(vec3(1.0, 0.75, 0.35) * 1.6, glowFalloff(lamp, 12.0) * 0.6), C.color);
                animePart(C, lamp, animeGlow(animeHex(0xFFC860u)));
            } else {
                // A halo of ice.
                float halo = abs(sdEllipse(q - vec2(2.0, -12.0), vec2(14.0, 4.0))) - 1.6;
                C.color = over(vec4(vec3(0.8, 0.95, 1.2), glowFalloff(halo, 6.0) * 0.6), C.color);
                animeFlat(C, halo, vec3(0.85, 0.97, 1.15));
            }
        }
    }

    // The defeat pop goes over the dissolving body.
    AnimeCanvas pop = C;
    pop.color = animeMotionFinish(C, p, m, accent);
    animePop(pop, p, feet - vec2(0.0, 20.0), m.state == kAnimeDefeat ? m.progress : 0.0, mix(bodyColor, accent, 0.5), 40.0);
    outColor = pop.color;
    outColor.a *= s.color.a;
}
