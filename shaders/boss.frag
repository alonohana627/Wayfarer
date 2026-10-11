#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_monster.glsl"

// Sample: a large multi-part boss in the MapleStory-style chibi look - the
// Elder Puffcap, an original giant fungus: a frilled, spotted cap with little
// mushrooms growing on it and a crown, a stalk body with a face, moss and a
// belly patch, two arms (one swinging a branch club), root feet. Slow, heavy
// motion (the shared states at half bounce) and phase colors.
//
// Design space: 128 x 128 units over the quad height, feet on y = +50,
// facing +x. anime_samples.hpp packs the fields (Boss).
//   color   stalk color      color2  cap color
//   params  x phase (0 calm, 1 angry: darker cap and glowing eyes,
//           2 enraged: an aura and glowing spots), y state (0 idle, 1 move,
//           2 wind-up: the club goes up, 3 hit, 4 defeat), z progress 0..1,
//           w facing (-1, 1)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

AnimeCanvas C;

void main()
{
    SpriteData s = sprite;
    float k = s.size.y / 128.0;
    float facing = s.params.w < 0.0 ? -1.0 : 1.0;
    vec2 local = spriteLocal(inUv);
    vec2 feet = vec2(0.0, 50.0);
    // The boss is drawn at 1/1.35 of the design box, so its tumbling defeat
    // and raised club stay inside the quad.
    vec2 p = feet + (vec2(local.x * facing, local.y) / k - feet) * 1.35;
    int phase = int(s.params.x + 0.5);
    int state = int(s.params.y + 0.5);
    float progress = s.params.z;
    float time = s.time;
    vec3 stalkColor = s.color.rgb;
    vec3 capColor = phase >= 1 ? animeSaturate(s.color2.rgb * vec3(0.8, 0.62, 0.75), 1.15) : s.color2.rgb;
    vec3 glow = phase >= 1 ? animeHex(0xFF6A5Au) : animeHex(0xFFE27Au);

    C = animeBegin(p, 2.4); // the boss is drawn ~4x larger: about the same on-screen weight
    C.shadeDepth = 5.0;
    AnimeMotion m = animeMonsterMotion(state, progress, time * 0.6, 0.5);
    if (state == kAnimeMove) {
        float w = sin(progress * 2.0 * kAnimePi);
        m.offset.y = -abs(w) * 2.0;
        m.squash = 1.0 + 0.03 * abs(w);
        m.tilt = 0.05 * w;
    }
    m.tilt *= state == kAnimeDefeat ? 0.55 : 1.0; // heavy: topples less (and stays in the quad)
    if (phase >= 1 && m.mood == kAnimeMoodCalm) {
        m.mood = kAnimeMoodAngry;
    }
    vec2 q = animeApplyMotion(p, m, feet);
    vec3 ink = animeInkOf(stalkColor * 0.5 + capColor * 0.5);

    animeGroundShadow(C, p, feet, 46.0, m);
    if (phase >= 2) {
        animeAuraRing(C, p, feet, 52.0, time, glow);
        float body = sdRoundBox(q - vec2(0.0, 10.0), vec2(48.0, 40.0), 30.0);
        C.color = over(vec4(glow, glowFalloff(body, 10.0) * (0.35 + 0.15 * sin(time * 5.0))), C.color);
    }

    AnimeMaterial stalk = animeMaterial(stalkColor);
    AnimeMaterial stalkBack = animeTinted(stalk, vec3(0.82, 0.82, 0.92));
    AnimeMaterial cap = animeMaterial(capColor);
    AnimeMaterial moss = animeMaterial(animeHex(0x6DBE57u));
    AnimeMaterial wood = animeMaterial(animeHex(0x9C6438u));
    float walk = state == kAnimeMove ? sin(progress * 2.0 * kAnimePi) : 0.0;
    float raise = state == kAnimeWindup ? smoothstep(0.0, 0.8, progress) : 0.0;

    // Back arm and back foot.
    vec2 shoulderB = vec2(-24.0, 16.0);
    vec2 handB = shoulderB + vec2(-10.0, 16.0 - raise * 6.0);
    animePart(C, sdLockBelly(q, shoulderB, shoulderB + vec2(-9.0, 4.0), handB, 7.0, 6.0), stalkBack);
    animePart(C, sdCircle(q - handB, 7.0), stalkBack);
    animePart(C, sdEllipse(q - feet - vec2(-15.0 + walk * 4.0, -5.0), vec2(13.0, 6.5)), stalkBack);

    // The stalk body.
    float body = sdTrapezoid(q - vec2(0.0, 23.0), 30.0, 23.0, 19.0) - 6.0;
    animePart(C, body, stalk);
    animePartNoLine(C, max(sdEllipse(q - vec2(6.0, 36.0), vec2(17.0, 12.0)), body + 2.0), animeMaterial(mix(stalkColor, vec3(1.0), 0.35)));
    // Moss patches with little leaves.
    float mossD = max(min(sdEllipse(q - vec2(-26.0, 40.0), vec2(12.0, 7.0)), sdEllipse(q - vec2(20.0, 10.0), vec2(9.0, 5.0))), body);
    animePart(C, mossD, moss);
    animePart(C, sdEar(q, vec2(-28.0, 36.0), vec2(-36.0, 30.0), 3.0, 0.3), moss);
    // The front foot.
    animePart(C, sdEllipse(q - feet - vec2(15.0 - walk * 4.0, -5.0), vec2(13.5, 7.0)), stalk);
    for (int i = 0; i < 3; ++i) {
        float x = 15.0 - walk * 4.0 + 6.0 + float(i) * 3.5;
        animeLine(C, max(abs(q.x - x), sdEllipse(q - feet - vec2(15.0 - walk * 4.0, -5.0), vec2(13.5, 7.0)) + 2.0 + step(q.y, feet.y - 6.0) * 1e3), stalk.ink);
    }

    // The face: big brows, eyes that glow in the angry phases, a fanged mouth.
    vec2 eyeL = vec2(-9.0, 22.0);
    vec2 eyeR = vec2(13.0, 22.0);
    if (phase >= 1 && m.mood != kAnimeMoodKnocked && m.mood != kAnimeMoodHurt) {
        for (int i = 0; i < 2; ++i) {
            vec2 e = i == 0 ? eyeL : eyeR;
            float eye = max(sdEllipse(q - e, vec2(5.5, 6.5)), -((q.y - e.y + 1.5) + (q.x - e.x) * (i == 0 ? 0.6 : -0.6)));
            C.color = over(vec4(glow, glowFalloff(eye, 5.0) * 0.6), C.color);
            animeFlat(C, eye, ink);
            animeFlat(C, max(sdCircle(q - e - vec2(0.0, 2.0), 3.0), eye), glow);
            animeFlat(C, max(sdCircle(q - e - vec2(-1.2, 1.0), 1.2), eye), vec3(1.0));
        }
    } else {
        animeDotEye(C, q, eyeL, 6.5, m.mood, ink, -1.0);
        animeDotEye(C, q, eyeR, 6.5, m.mood, ink, 1.0);
    }
    float browTilt = phase >= 1 || state == kAnimeWindup ? 1.0 : 0.2;
    animeBrow(C, q, eyeL + vec2(0.0, -10.0), 13.0, -1.0, browTilt, ink);
    animeBrow(C, q, eyeR + vec2(0.0, -10.0), 13.0, 1.0, browTilt, ink);
    animeMonsterMouth(C, q, vec2(3.0, 36.0), 13.0, phase >= 1 || state == kAnimeWindup ? kAnimeMoodAngry : (m.mood == kAnimeMoodCalm ? kAnimeMoodHappy : m.mood), ink);

    // The cap: a wide frilled bell with stripe bands, small bellcaps growing
    // on it and a crown.
    vec2 rim = vec2(0.0, 8.0);
    vec2 cq = rotate2d(q - rim, -0.08 * raise) + rim;
    float capD = sdBellCap(cq, rim, 60.0, 52.0, 9.0);
    // The brim's shadow on the stalk.
    animeCastShadow(C, body, sdBellCap(q - vec2(2.0, 6.0), rim, 58.0, 52.0, 9.0), stalk);
    for (int i = 0; i < 2; ++i) {
        vec2 at = rim + (i == 0 ? vec2(-30.0, -40.0) : vec2(26.0, -44.0));
        animePart(C, sdSegment(cq, at, at + vec2(0.0, -8.0), 3.0), stalk);
        float small = sdBellCap(cq, at + vec2(0.0, -6.0), 9.0, 11.0, 3.0);
        animePart(C, small, animeMaterial(i == 0 ? animeHex(0xF790B6u) : animeHex(0xFAD65Eu)));
    }
    animePart(C, capD, cap);
    float stripes = max(animeDomeStripes(cq, rim - vec2(0.0, 8.0), 60.0, 17.0, 6.0), capD + 3.0);
    vec3 stripeColor = phase >= 2 ? mix(glow, vec3(1.0), 0.4) : animeHex(0xFFF3D6u);
    if (phase >= 2) {
        C.color = over(vec4(glow, glowFalloff(stripes, 4.0) * 0.7 * (0.6 + 0.4 * sin(time * 6.0))), C.color);
    }
    animePartNoLine(C, stripes, phase >= 2 ? animeGlow(stripeColor) : animeMaterial(stripeColor));
    animeGloss(C, cq, capD, rim + vec2(-20.0, -38.0), vec2(12.0, 7.0), 0.35);
    animeCrown(C, cq, rim + vec2(-2.0, -53.0), 26.0, -0.12);

    // The front arm swinging a branch club; raised high on the wind-up.
    vec2 shoulderF = vec2(25.0, 18.0);
    // At rest the club is held ready, pointing up and forward.
    float a = mix(0.45, -2.0, raise) + sin(time * 1.5) * 0.05 * (1.0 - raise);
    vec2 handF = shoulderF + vec2(cos(a), sin(a)) * 18.0;
    float clubAngle = mix(a - 0.9, a + 0.9, raise);
    vec2 dir = vec2(cos(clubAngle), sin(clubAngle));
    vec2 clubTip = handF + dir * 34.0;
    float club = sdLockBelly(q, handF - dir * 6.0, handF + dir * 16.0, clubTip, 3.5, 8.0);
    animePart(C, club, wood);
    animeLine(C, max(sdBezier(q, handF + dir * 10.0, handF + dir * 18.0 + vec2(-dir.y, dir.x) * 2.0, handF + dir * 28.0), club + 1.2), wood.ink);
    animePart(C, sdEar(q, clubTip - dir * 8.0, clubTip - dir * 4.0 + vec2(-dir.y, dir.x) * 12.0, 3.5, 0.3), moss);
    animePart(C, sdLockBelly(q, shoulderF, mix(shoulderF, handF, 0.5) + vec2(0.0, 2.0), handF, 7.0, 6.5), stalk);
    animePart(C, sdCircle(q - handF, 7.5), stalk);

    AnimeCanvas pop = C;
    pop.color = animeMotionFinish(C, p, m, capColor);
    animePop(pop, p, feet - vec2(0.0, 40.0), m.state == kAnimeDefeat ? m.progress : 0.0, capColor, 70.0);
    outColor = pop.color;
    outColor.a *= s.color.a;
}
