#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_monster.glsl"
#include "gamelib/noise.glsl"

// Wayfarer's five guardians, big multi-part versions of the region's
// creatures in the same chibi construction (ink lines, one cel tone, the
// face kit's eyes), each with slow, heavy motion and phase colors:
//   0 Grandcap         - a giant bellcap elder with a frilled cap, little
//                        bellcaps growing on it, a crown and a branch club
//   1 Rumblerock       - a boulder golem with glowing ore and stone fists
//   2 Queen Murkshell  - a huge turbanshell with a pearl, a tiara and eye
//                        stalks; spins inside her shell when guarded
//   3 Lantern Warden   - a wisp in a knight's helm holding a lantern
//   4 The Hush         - a hooded cloak full of night sky with a pale face
//                        and a silver crown
// Angry faces are angry: lids down to the inside, brows pinched, a frown.
//
// Design space: 128 x 128 units over the quad height, feet on y = +50,
// facing +x; the body is drawn at 1/1.35 of the box so motions stay inside.
//   color   body color      color2  accent (cap, ore, shell, flame, cloak)
//   params  x phase (0 calm, 1 angry, 2 enraged), y state (0 idle, 1 move,
//           2 wind-up, 3 hit, 4 defeat), z progress 0..1, w facing (-1, 1)
//   bits.x  0-2 kind, 3 guarded (in the shell)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

AnimeCanvas C;
float T;

const int kGrandcap = 0, kRumblerock = 1, kMurkshell = 2, kWarden = 3, kHush = 4;

AnimeFace bossFace(int mood, int phase, int state)
{
    AnimeFace f = animeExpression(kAnimeNeutral);
    if (mood == kAnimeMoodHurt) {
        return animeExpression(kAnimeHurt);
    }
    if (mood == kAnimeMoodKnocked) {
        return animeExpression(kAnimeKnocked);
    }
    if (phase >= 1 || state == kAnimeWindup || mood == kAnimeMoodAngry) {
        f = animeExpression(kAnimeAngry);
        f.sweat = 0.0;
        f.blush = 0.0;
        f.open = 0.95;
        f.slant = phase >= 2 ? 1.0 : 0.8;
        f.browTilt = 1.0;
        f.mouthCurve = -0.8;
        f.mouthOpen = state == kAnimeWindup ? 0.7 : 0.3;
        f.anger = phase >= 1 ? 1.0 : 0.0;
        return f;
    }
    f.open = 0.9;
    f.mouthCurve = 0.2;
    return f;
}

// Two eyes with brows in the hero's face kit, glowing in the angry phases.
void bossEyes(vec2 q, vec2 left, vec2 right, float h, AnimeFace f, vec3 iris, vec3 glow, int phase, float seed)
{
    vec3 ink = animeInkOf(iris * 0.3 + vec3(0.12, 0.05, 0.06));
    if (f.eyeShape == kAnimeEyeOpen) {
        f.open *= animeBlink(T, seed);
        if (f.open < 0.12) {
            f.eyeShape = kAnimeEyeClosed;
        }
    }
    vec3 eyeIris = phase >= 1 ? glow : iris;
    if (phase >= 1 && f.eyeShape == kAnimeEyeOpen) {
        for (int i = 0; i < 2; ++i) {
            vec2 e = i == 0 ? left : right;
            C.color = over(vec4(glow * 1.3, glowFalloff(sdEllipse(q - e, vec2(h * 0.4, h * 0.5)), h * 0.5) * 0.45), C.color);
        }
    }
    f.look = vec2(0.3, 0.1);
    animeEye(C, q, left, vec2(h * 0.8, h), -1.0, f, eyeIris, ink);
    animeEye(C, q, right, vec2(h * 0.8, h), 1.0, f, eyeIris, ink);
    float tilt = f.browTilt;
    float bw = h * 1.0;
    animeBrow(C, q, left + vec2(0.0, -h * (0.82 + f.browLift)), bw, -1.0, tilt, ink);
    animeBrow(C, q, right + vec2(0.0, -h * (0.82 + f.browLift)), bw, 1.0, tilt, ink);
}

void bossMouth(vec2 q, vec2 at, float w, AnimeFace f, vec3 ink)
{
    animeMouth(C, q, at, w, f.mouthCurve, f.mouthOpen, ink);
}

void grandcap(vec2 q, vec2 feet, AnimeMotion m, int phase, int state, float progress, vec3 stalkColor, vec3 capBase, vec3 glow)
{
    vec3 capColor = phase >= 1 ? animeSaturate(capBase * vec3(0.8, 0.62, 0.75), 1.15) : capBase;
    AnimeMaterial stalk = animeMaterial(stalkColor);
    AnimeMaterial stalkBack = animeTinted(stalk, vec3(0.82, 0.82, 0.92));
    AnimeMaterial cap = animeMaterial(capColor);
    AnimeMaterial moss = animeMaterial(animeHex(0x6DBE57u));
    AnimeMaterial wood = animeMaterial(animeHex(0x9C6438u));
    float walk = state == kAnimeMove ? sin(progress * 2.0 * kAnimePi) : 0.0;
    float raise = state == kAnimeWindup ? smoothstep(0.0, 0.8, progress) : 0.0;
    vec2 shoulderB = vec2(-24.0, 16.0);
    vec2 handB = shoulderB + vec2(-10.0, 16.0 - raise * 6.0);
    animePart(C, sdLockBelly(q, shoulderB, shoulderB + vec2(-9.0, 4.0), handB, 7.0, 6.0), stalkBack);
    animePart(C, sdCircle(q - handB, 7.0), stalkBack);
    animePart(C, sdEllipse(q - feet - vec2(-15.0 + walk * 4.0, -5.0), vec2(13.0, 6.5)), stalkBack);
    float body = sdTrapezoid(q - vec2(0.0, 23.0), 30.0, 23.0, 19.0) - 6.0;
    animePart(C, body, stalk);
    animePartNoLine(C, max(sdEllipse(q - vec2(6.0, 36.0), vec2(17.0, 12.0)), body + 2.0), animeMaterial(mix(stalkColor, vec3(1.0), 0.35)));
    float mossD = max(min(sdEllipse(q - vec2(-26.0, 40.0), vec2(12.0, 7.0)), sdEllipse(q - vec2(20.0, 10.0), vec2(9.0, 5.0))), body);
    animePart(C, mossD, moss);
    animePart(C, sdEllipse(q - feet - vec2(15.0 - walk * 4.0, -5.0), vec2(13.5, 7.0)), stalk);
    AnimeFace f = bossFace(m.mood, phase, state);
    bossEyes(q, vec2(-8.0, 22.0), vec2(13.0, 22.0), 15.0, f, animeHex(0x6A4A2Eu), glow, phase, 0.3);
    bossMouth(q, vec2(3.0, 35.0), 9.0, f, animeInkOf(stalkColor) * 1.4);
    animeBlush(C, q, vec2(-15.0, 30.0), vec2(5.0, 2.4), phase >= 1 ? 0.0 : 0.6, animeInkOf(stalkColor));
    animeBlush(C, q, vec2(21.0, 30.0), vec2(5.0, 2.4), phase >= 1 ? 0.0 : 0.6, animeInkOf(stalkColor));
    vec2 rim = vec2(0.0, 8.0);
    vec2 cq = rotate2d(q - rim, -0.08 * raise) + rim;
    float capD = sdBellCap(cq, rim, 60.0, 52.0, 9.0);
    animeCastShadow(C, body, sdBellCap(q - vec2(2.0, 6.0), rim, 58.0, 52.0, 9.0), stalk);
    for (int i = 0; i < 2; ++i) {
        vec2 at = rim + (i == 0 ? vec2(-30.0, -40.0) : vec2(26.0, -44.0));
        animePart(C, sdSegment(cq, at, at + vec2(0.0, -8.0), 3.0), stalk);
        animePart(C, sdBellCap(cq, at + vec2(0.0, -6.0), 9.0, 11.0, 3.0), animeMaterial(i == 0 ? animeHex(0xF790B6u) : animeHex(0xFAD65Eu)));
    }
    animePart(C, capD, cap);
    float stripes = max(animeDomeStripes(cq, rim - vec2(0.0, 8.0), 60.0, 17.0, 6.0), capD + 3.0);
    vec3 stripeColor = phase >= 2 ? mix(glow, vec3(1.0), 0.4) : animeHex(0xFFF3D6u);
    if (phase >= 2) {
        C.color = over(vec4(glow, glowFalloff(stripes, 4.0) * 0.7 * (0.6 + 0.4 * sin(T * 6.0))), C.color);
    }
    animePartNoLine(C, stripes, phase >= 2 ? animeGlow(stripeColor) : animeMaterial(stripeColor));
    animeGloss(C, cq, capD, rim + vec2(-20.0, -38.0), vec2(12.0, 7.0), 0.35);
    animeCrown(C, cq, rim + vec2(-2.0, -53.0), 26.0, -0.12);
    vec2 shoulderF = vec2(25.0, 18.0);
    float a = mix(0.45, -2.0, raise) + sin(T * 1.5) * 0.05 * (1.0 - raise);
    vec2 handF = shoulderF + vec2(cos(a), sin(a)) * 18.0;
    float clubAngle = mix(a - 0.9, a + 0.9, raise);
    vec2 dir = vec2(cos(clubAngle), sin(clubAngle));
    vec2 clubTip = handF + dir * 34.0;
    float club = sdLockBelly(q, handF - dir * 6.0, handF + dir * 16.0, clubTip, 3.5, 8.0);
    animePart(C, club, wood);
    animePart(C, sdEar(q, clubTip - dir * 8.0, clubTip - dir * 4.0 + vec2(-dir.y, dir.x) * 12.0, 3.5, 0.3), moss);
    animePart(C, sdLockBelly(q, shoulderF, mix(shoulderF, handF, 0.5) + vec2(0.0, 2.0), handF, 7.0, 6.5), stalk);
    animePart(C, sdCircle(q - handF, 7.5), stalk);
}

void rumblerock(vec2 q, vec2 feet, AnimeMotion m, int phase, int state, float progress, vec3 rockColor, vec3 ore, vec3 glow)
{
    AnimeMaterial rock = animeMatte(rockColor);
    AnimeMaterial rockBack = animeTinted(rock, vec3(0.8, 0.8, 0.9));
    float walk = state == kAnimeMove ? sin(progress * 2.0 * kAnimePi) : 0.0;
    float raise = state == kAnimeWindup ? smoothstep(0.0, 0.8, progress) : 0.0;
    vec3 oreColor = phase >= 1 ? glow : ore;
    // The back fist and leg.
    vec2 fistB = vec2(-40.0, 26.0 - raise * 10.0 + sin(T * 1.3) * 1.5);
    animePart(C, sdSegment(q, vec2(-28.0, 10.0), fistB, 7.0), rockBack);
    animePart(C, sdRoundBox(q - fistB, vec2(11.0, 10.0), 7.0), rockBack);
    animePart(C, sdRoundBox(q - feet - vec2(-16.0 + walk * 4.0, -7.0), vec2(11.0, 8.0), 5.0), rockBack);
    // The boulder body with cracks and moss.
    float body = sdRoundBox(q - vec2(0.0, 12.0), vec2(36.0, 32.0), 26.0) + valueNoise(q * 0.07) * 4.0;
    animePart(C, body, rock);
    vec3 v = voronoi(q * 0.06 + 2.0);
    animeLine(C, max(abs(v.y - v.x) * 14.0 - 0.6, body + 4.0), rock.ink);
    float moss = max(body + 0.5, (q.y + 16.0) - sin(q.x * 0.25) * 3.0);
    animePartNoLine(C, moss, animeMaterial(animeHex(0x7CC062u)));
    // Ore crystals on the back.
    float crystal = 1e5;
    for (int i = 0; i < 4; ++i) {
        float fi = float(i);
        vec2 base = vec2(-28.0 + fi * 9.0, -10.0 - fi * 3.0);
        vec2 tip = base + vec2(-8.0 + fi * 3.0, -18.0 - fi * 3.0);
        crystal = min(crystal, sdLock(q, base, mix(base, tip, 0.5), tip, 6.5, 1.2));
    }
    C.color = over(vec4(oreColor * 1.5, glowFalloff(crystal, 14.0) * (0.45 + 0.25 * sin(T * 3.0) * float(phase >= 2))), C.color);
    animePart(C, crystal, animeGlow(oreColor));
    // The front leg.
    animePart(C, sdRoundBox(q - feet - vec2(16.0 - walk * 4.0, -7.0), vec2(12.0, 8.0), 5.0), rock);
    // The face.
    AnimeFace f = bossFace(m.mood, phase, state);
    bossEyes(q, vec2(6.0, 10.0), vec2(24.0, 10.0), 14.0, f, animeHex(0x3E6A8Au), glow, phase, 0.6);
    bossMouth(q, vec2(16.0, 26.0), 10.0, f, rock.ink);
    // The front fist, raised on the wind-up.
    vec2 shoulder = vec2(26.0, 14.0);
    vec2 fist = shoulder + mix(vec2(16.0, 18.0), vec2(6.0, -36.0), raise) + vec2(0.0, sin(T * 1.3 + 1.0) * 1.5);
    animePart(C, sdSegment(q, shoulder, fist, 8.0), rock);
    float fistD = sdRoundBox(q - fist, vec2(13.0, 12.0), 8.0);
    animePart(C, fistD, rock);
    animeLine(C, max(abs(q.y - fist.y), fistD + 3.0), rock.ink);
}

void murkshell(vec2 q, vec2 feet, AnimeMotion m, int phase, int state, float progress, bool guarded, vec3 soft, vec3 shellColor, vec3 glow)
{
    AnimeMaterial body = animeMaterial(soft);
    AnimeMaterial shell = animeMaterial(phase >= 1 ? animeSaturate(shellColor * vec3(0.85, 0.7, 0.9), 1.2) : shellColor);
    float stretch = state == kAnimeMove ? sin(progress * 2.0 * kAnimePi) * 4.0 : 0.0;
    float hide = guarded ? 1.0 : 0.0;
    // The soft foot along the ground, the head at the front.
    vec2 head = vec2(36.0 + stretch - hide * 26.0, 24.0 + hide * 14.0);
    float foot = sdRoundBox(q - vec2(4.0 + stretch * 0.5, 41.0), vec2(48.0 + stretch * 0.5, 9.0), 9.0);
    foot = animeSmoothUnion(foot, sdCircle(q - head, 16.0), 8.0);
    if (!guarded) {
        for (int i = 0; i < 2; ++i) {
            vec2 base = head + vec2(-4.0 + float(i) * 9.0, -12.0);
            vec2 tip = base + vec2(float(i) * 4.0 + sin(T * 2.0 + float(i)) * 2.0, -14.0);
            animePart(C, sdSegment(q, base, tip, 3.0), i == 0 ? animeTinted(body, vec3(0.85, 0.85, 0.92)) : body);
            animePart(C, sdCircle(q - tip, 3.6), body);
        }
        animePart(C, foot, body);
        AnimeFace f = bossFace(m.mood, phase, state);
        bossEyes(q, head + vec2(-3.0, -1.0), head + vec2(9.0, -1.0), 11.0, f, animeHex(0x2E7A6Au), glow, phase, 0.9);
        bossMouth(q, head + vec2(4.0, 9.0), 6.0, f, body.ink);
        animeBlush(C, q, head + vec2(-8.0, 6.0), vec2(3.5, 1.8), phase >= 1 ? 0.0 : 0.7, body.ink);
        // A little tiara.
        animeCrown(C, q, head + vec2(2.0, -15.0), 14.0, 0.1);
    }
    // The shell: three tiers, a pearl on top; it spins when guarded.
    vec2 sc = vec2(-10.0, 34.0);
    vec2 sq = q;
    if (guarded) {
        sq = rotate2d(q - vec2(-10.0, 10.0), T * 9.0) + vec2(-10.0, 10.0);
        C.color = over(vec4(vec3(1.0), (1.0 - smoothstep(0.0, 3.0, abs(length(q - vec2(-10.0, 10.0)) - 46.0))) * 0.5), C.color);
    }
    float tiers = 1e5;
    for (int i = 0; i < 3; ++i) {
        float w = 34.0 - float(i) * 9.0;
        tiers = min(tiers, sdEllipse(sq - sc - vec2(float(i) * 3.0, -float(i) * 17.0 - 2.0), vec2(w, 12.0)));
    }
    animePart(C, tiers, shell);
    for (int i = 0; i < 3; ++i) {
        float w = 34.0 - float(i) * 9.0;
        vec2 c = sc + vec2(float(i) * 3.0, -float(i) * 17.0 - 2.0);
        animeLine(C, max(abs(sdEllipse(sq - c, vec2(w, 12.0))), tiers + 0.6), shell.ink);
        float band = max(sdEllipse(sq - c - vec2(0.0, 1.5), vec2(w * 0.9, 3.6)), tiers + 2.0);
        animePartNoLine(C, band, phase >= 2 ? animeGlow(mix(glow, vec3(1.0), 0.4)) : animeMaterial(animeHex(0xFFF3D6u)));
    }
    animeGloss(C, sq, tiers, sc + vec2(-14.0, -10.0), vec2(8.0, 5.0), 0.45);
    float pearl = sdCircle(sq - sc - vec2(8.0, -56.0), 7.0);
    C.color = over(vec4(vec3(1.0, 0.95, 1.0), glowFalloff(pearl, 8.0) * 0.5), C.color);
    animePart(C, pearl, animeMetal(animeHex(0xF6EEFFu)));
}

void warden(vec2 q, AnimeMotion m, int phase, int state, float progress, vec3 sheetColor, vec3 flame, vec3 glow)
{
    AnimeMaterial sheet = animeMaterial(sheetColor);
    AnimeMaterial steel = animeMetal(animeHex(0xC8CCDAu));
    AnimeMaterial gold = animeMetal(animeHex(0xF2C14Eu));
    float wave = sin(T * 2.5);
    float raise = state == kAnimeWindup ? smoothstep(0.0, 0.8, progress) : 0.0;
    vec3 fire = phase >= 1 ? mix(flame, glow, 0.5) : flame;
    // A cape behind.
    float cape = sdLockBelly(q, vec2(-6.0, -16.0), vec2(-24.0, 10.0), vec2(-36.0 + wave * 4.0, 44.0), 16.0, 22.0);
    animePart(C, cape, animeCloth(animeHex(0x4E5FA8u)));
    // The sheet body.
    float body = sdLockBelly(q, vec2(0.0, -10.0), vec2(4.0, 20.0), vec2(-18.0 + wave * 5.0, 46.0), 28.0, 6.0);
    body = animeSmoothUnion(body, sdCircle(q - vec2(2.0, -6.0), 26.0), 8.0);
    C.color = over(vec4(mix(sheetColor, vec3(1.0), 0.5), glowFalloff(body, 10.0) * 0.3), C.color);
    animePart(C, body, sheet);
    animeLine(C, max(sdBezier(q, vec2(-12.0, 28.0), vec2(-16.0, 38.0), vec2(-22.0 + wave * 4.0, 44.0)), body + 1.0), sheet.ink);
    // The helm on top of the head, its visor slit glowing.
    float helm = max(sdCircle(q - vec2(4.0, -20.0), 22.0), q.y + 4.0);
    helm = min(helm, sdRoundBox(q - vec2(4.0, -6.0), vec2(22.0, 3.0), 2.0));
    animePartShine(C, helm, steel, animeCover(C, abs(sdCircle(q - vec2(-2.0, -27.0), 12.0)) - 1.0) * 0.6, vec3(1.0));
    animeLine(C, max(abs(q.x - 10.0 - (q.y + 20.0) * 0.1), helm + 2.0), steel.ink);
    float plume = sdLockBelly(q, vec2(4.0, -40.0), vec2(-14.0, -54.0), vec2(-34.0, -40.0), 5.0, 2.0);
    animePart(C, plume, animeCloth(animeHex(0xEC5662u)));
    float slit = sdRoundBox(q - vec2(11.0, -15.0), vec2(13.0, 4.0), 3.0);
    animeFlat(C, slit, animeHex(0x1E1A30u));
    AnimeFace f = bossFace(m.mood, phase, state);
    vec3 eyeGlow = phase >= 1 ? glow : fire;
    for (int i = 0; i < 2; ++i) {
        vec2 e = vec2(5.0 + float(i) * 12.0, -15.0);
        float eye = max(sdEllipse(q - e, vec2(4.0, 3.5 * (f.eyeShape == kAnimeEyeOpen ? 1.0 : 0.3))), slit);
        if (phase >= 1) {
            eye = max(eye, -(q.y - e.y + 1.5 + (q.x - e.x) * (i == 0 ? -0.5 : 0.5)));
        }
        C.color = over(vec4(eyeGlow * 1.5, glowFalloff(eye, 4.0) * 0.8), C.color);
        animeFlat(C, eye, eyeGlow * 1.6);
    }
    animePart(C, sdRoundBox(q - vec2(4.0, -2.0), vec2(22.0, 2.5), 2.0), gold);
    // The lantern on its staff, held out in front; raised to cast.
    vec2 hand = vec2(30.0, 8.0) + vec2(0.0, -raise * 24.0);
    animePart(C, sdSegment(q, hand + vec2(0.0, 30.0), hand + vec2(0.0, -26.0), 2.4), animeMaterial(animeHex(0x8A5A3Cu)));
    vec2 lantern = hand + vec2(0.0, -30.0);
    float frame = sdRoundBox(q - lantern - vec2(0.0, 8.0), vec2(11.0, 13.0), 4.0);
    float flick = 0.85 + 0.15 * sin(T * 11.0) * sin(T * 7.0);
    C.color = over(vec4(fire * 1.6, glowFalloff(frame, 26.0) * 0.55 * flick), C.color);
    animePart(C, frame, gold);
    animeFlat(C, sdRoundBox(q - lantern - vec2(0.0, 8.0), vec2(7.5, 10.0), 3.0), fire * 1.8 * flick);
    animePart(C, sdCircle(q - hand, 6.5), sheet);
}

void hush(vec2 q, AnimeMotion m, int phase, int state, float progress, vec3 face, vec3 cloakColor, vec3 glow)
{
    float wave = sin(T * 1.8);
    float raise = state == kAnimeWindup ? smoothstep(0.0, 0.8, progress) : 0.0;
    vec3 cloakC = phase >= 2 ? mix(cloakColor, animeHex(0x2A1450u), 0.4) : cloakColor;
    AnimeMaterial cloak = animeCloth(cloakC);
    AnimeMaterial silver = animeMetal(animeHex(0xDCE4F4u));
    // The back sleeve.
    vec2 handB = vec2(-40.0 + wave * 3.0, 18.0 - raise * 26.0);
    animePart(C, sdLockBelly(q, vec2(-18.0, -6.0), vec2(-32.0, 6.0), handB, 9.0, 8.0), animeTinted(cloak, vec3(0.8, 0.8, 0.9)));
    animePart(C, sdCircle(q - handB, 6.0), animeMaterial(face));
    // The cloak: a hood and a long hem with tatters that wave.
    float hem = 44.0 + sin(q.x * 0.3 + T * 3.0) * 3.0;
    float cloakD = sdTrapezoid(q - vec2(0.0, 14.0), 40.0, 18.0, 32.0) - 6.0;
    cloakD = animeSmoothUnion(cloakD, sdCircle(q - vec2(0.0, -22.0), 27.0), 10.0);
    cloakD = max(cloakD, q.y - hem);
    if (phase >= 1) {
        C.color = over(vec4(glow, glowFalloff(cloakD, 14.0) * (0.25 + 0.2 * float(phase >= 2))), C.color);
    }
    animePart(C, cloakD, cloak);
    // Night sky inside the cloak: stars.
    vec2 cell = floor(q / 7.0);
    float h = hashCell(cell);
    vec2 sp = (cell + 0.5) * 7.0 + (vec2(hashCell(cell + 1.3), hashCell(cell + 2.7)) - 0.5) * 4.0;
    float twinkle = 0.5 + 0.5 * sin(T * 3.0 + h * 20.0);
    float star = max(sdCircle(q - sp, 0.9 + 0.6 * twinkle), cloakD + 4.0);
    animeFlat(C, star + step(h, 0.72) * 100.0, vec3(1.3, 1.25, 1.1));
    // The face in the hood.
    float faceD = sdEllipse(q - vec2(6.0, -18.0), vec2(16.0, 17.0));
    animeFlat(C, max(sdEllipse(q - vec2(6.0, -16.0), vec2(20.0, 21.0)), cloakD), animeShadowTone(cloakC, 0.5));
    animePart(C, faceD, animeSkin(face));
    AnimeFace f = bossFace(m.mood, phase, state);
    if (phase == 0 && state != kAnimeWindup && m.mood == kAnimeMoodCalm) {
        f = animeExpression(kAnimeBlink); // the hush: eyes closed, silent
        f.mouthCurve = 0.0;
        f.mouthOpen = 0.0;
    }
    bossEyes(q, vec2(0.0, -16.0), vec2(13.0, -16.0), 11.0, f, animeHex(0x5A6ACAu), glow, phase, 0.2);
    bossMouth(q, vec2(7.0, -5.0), 5.0, f, animeInkOf(face) * 1.6);
    // The silver crown.
    animeCrown(C, q, vec2(2.0, -44.0), 30.0, -0.1);
    animePart(C, sdRoundBox(q - vec2(2.0, -40.0), vec2(16.0, 2.5), 2.0), silver);
    // The front sleeve, raised to cast.
    vec2 handF = vec2(36.0 + wave * 3.0, 20.0 - raise * 34.0);
    animePart(C, sdLockBelly(q, vec2(18.0, 6.0), vec2(32.0, 14.0), handF, 9.0, 8.0), cloak);
    animePart(C, sdCircle(q - handF, 6.0), animeMaterial(face));
    if (state == kAnimeWindup || phase >= 2) {
        float orb = sdCircle(q - handF - vec2(4.0, -8.0), 6.0 + raise * 3.0);
        C.color = over(vec4(glow * 1.6, glowFalloff(orb, 16.0) * 0.7), C.color);
        animeFlat(C, orb, mix(glow, vec3(1.0), 0.5) * 1.5);
    }
}

void main()
{
    SpriteData s = sprite;
    float k = s.size.y / 128.0;
    float facing = s.params.w < 0.0 ? -1.0 : 1.0;
    vec2 local = spriteLocal(inUv);
    vec2 feet = vec2(0.0, 50.0);
    vec2 p = feet + (vec2(local.x * facing, local.y) / k - feet) * 1.35;
    int phase = int(s.params.x + 0.5);
    int state = int(s.params.y + 0.5);
    float progress = s.params.z;
    int kind = int(s.bits.x & 7u);
    bool guarded = ((s.bits.x >> 3) & 1u) != 0u;
    T = s.time;
    vec3 body = s.color.rgb;
    vec3 accent = s.color2.rgb;
    vec3 glow = phase >= 1 ? animeHex(0xFF6A5Au) : animeHex(0xFFE27Au);
    if (kind == kHush) {
        glow = phase >= 1 ? animeHex(0xB89CFFu) : animeHex(0xDDE6FFu);
    } else if (kind == kRumblerock) {
        glow = phase >= 1 ? animeHex(0xFF8A3Au) : animeHex(0x7FE6FFu);
    }

    C = animeBegin(p, 2.4);
    C.shadeDepth = 5.0;
    bool flying = kind == kWarden || kind == kHush;
    AnimeMotion m = animeMonsterMotion(state, progress, T * 0.6, flying ? 0.0 : 0.5);
    if (state == kAnimeMove) {
        float w = sin(progress * 2.0 * kAnimePi);
        m.offset.y = -abs(w) * 2.0;
        m.squash = 1.0 + 0.03 * abs(w);
        m.tilt = 0.05 * w;
    }
    if (flying) {
        m.offset.y += sin(T * 1.6) * 3.0;
        m.tilt *= 0.5;
    }
    m.tilt *= state == kAnimeDefeat ? 0.55 : 1.0;
    if (phase >= 1 && m.mood == kAnimeMoodCalm) {
        m.mood = kAnimeMoodAngry;
    }
    vec2 q = animeApplyMotion(p, m, feet);
    if (!flying) {
        animeGroundShadow(C, p, feet, kind == kMurkshell ? 54.0 : 46.0, m);
    }
    if (phase >= 2) {
        animeAuraRing(C, p, feet, 52.0, T, glow);
    }

    if (kind == kRumblerock) {
        rumblerock(q, feet, m, phase, state, progress, body, accent, glow);
    } else if (kind == kMurkshell) {
        murkshell(q, feet, m, phase, state, progress, guarded, body, accent, glow);
    } else if (kind == kWarden) {
        warden(q, m, phase, state, progress, body, accent, glow);
    } else if (kind == kHush) {
        hush(q, m, phase, state, progress, body, accent, glow);
    } else {
        grandcap(q, feet, m, phase, state, progress, body, accent, glow);
    }

    AnimeCanvas pop = C;
    pop.color = animeMotionFinish(C, p, m, accent);
    animePop(pop, p, feet - vec2(0.0, 40.0), m.state == kAnimeDefeat ? m.progress : 0.0, mix(body, accent, 0.5), 70.0);
    outColor = pop.color;
    outColor.a *= s.color.a;
}
