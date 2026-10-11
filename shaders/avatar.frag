#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_face.glsl"

// Sample: a MapleStory-style chibi avatar (about 2 heads tall: a big round
// head, a small body with short limbs) built from equipment layers - hair,
// hat, top, bottom, shoes, gloves, weapon, cape - each picked by a few bits,
// with eight expressions and seven poses, all from gamelib/anime*.glsl. Copy
// it into a game and add its own items, or register it as is.
//
// Construction: ONE skeleton of anchor points (Rig: head, neck, shoulders,
// elbows, hands, hips, knees, feet, the weapon grip) set per pose in
// rigFor(); every layer is drawn from those anchors in a fixed order (cape,
// back hair, far arm, far leg, near leg, bottom, top, head, face, front hair,
// hat, weapon, near arm). The body is in the same three-quarter view as the
// face: the far side narrower, buttons and zips offset towards the facing side.
//
// Design space: 128 x 128 units over the quad height (the width may differ),
// feet on y = +56, facing +x. anime_samples.hpp packs the fields.
//   color   hair color            color2  top (shirt, robe, armor) color
//   params  x facing (-1, 1), y pose (0 idle, 1 walk, 2 jump, 3 attack,
//           4 hurt, 5 cheer, 6 climb), z phase 0..1 (walk cycle, attack
//           progress, climb cycle), w expression blend A -> B 0..1
//   bits.x  0-2 hair style, 3-5 hat, 6-8 top, 9-11 bottom, 12-13 shoes,
//           14-15 gloves, 16-18 weapon, 19-20 cape, 21-23 eye color,
//           24-25 skin tone, 26-29 expression A, 30 draw the skeleton
//   bits.y  0-3 expression B, 4-7 hat color, 8-11 bottom color,
//           12-15 shoe color, 16-19 cape color, 20-23 accent (trim, gloves)
//           - colors are kAnimePalette indices - 24-31 blink seed

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

AnimeCanvas C;
vec2 P; // the pixel in design units
// Per-sprite equipment and materials, set at the start of main() (shared with
// the drawing helpers).
uint gWeapon, gTop, gGloves;
vec3 gAccent;
AnimeMaterial gCloth, gForearm, gHand, gTrim, gSteel, gWood, gLeather;

const uint kEyes[8] = uint[8](0x2F6FE0u, 0x23A866u, 0xE0952Au, 0x8E4FE0u, 0xD8323Fu, 0x14A6B4u, 0x6B4A3Au, 0xE05A9Au);
const uint kSkins[4] = uint[4](0xFFE6D6u, 0xF8D2B6u, 0xDDA780u, 0x9E6A48u);

// Slots.
const uint kHairShort = 0u, kHairPonytail = 1u, kHairBob = 2u, kHairLong = 3u, kHairTwinTails = 4u;
const uint kHatNone = 0u, kHatHeadband = 1u, kHatWizard = 2u, kHatBeanie = 3u, kHatHelmet = 4u, kHatRibbon = 5u;
const uint kTopTunic = 0u, kTopRobe = 1u, kTopVest = 2u, kTopArmor = 3u, kTopSailor = 4u;
const uint kBottomPants = 0u, kBottomShorts = 1u, kBottomSkirt = 2u;
const uint kShoesBoots = 0u, kShoesShoes = 1u, kShoesGreaves = 2u;
const uint kGlovesNone = 0u, kGlovesCloth = 1u, kGlovesGauntlets = 2u;
const uint kWeaponNone = 0u, kWeaponSword = 1u, kWeaponStaff = 2u, kWeaponBow = 3u, kWeaponDagger = 4u, kWeaponAxe = 5u;
const uint kCapeNone = 0u, kCapeCape = 1u, kCapeScarf = 2u;

const int kPoseIdle = 0, kPoseWalk = 1, kPoseJump = 2, kPoseAttack = 3, kPoseHurt = 4, kPoseCheer = 5, kPoseClimb = 6;

const float kUpperArm = 8.5, kForearm = 8.0, kThigh = 8.5, kShin = 8.0;
const vec3 kFar = vec3(0.8, 0.8, 0.9); // the far arm and leg: one shade darker

// The skeleton: every layer hangs on these anchors.
struct Rig {
    vec2 head;      // the middle of the face oval
    vec2 neck;      // the top of the torso, under the chin
    vec2 shoulderN; // N = near (towards the viewer), F = far
    vec2 shoulderF;
    vec2 elbowN;
    vec2 elbowF;
    vec2 handN;
    vec2 handF;
    vec2 hipN;
    vec2 hipF;
    vec2 kneeN;
    vec2 kneeF;
    vec2 footN;     // where the sole meets the ground (or would)
    vec2 footF;
    vec2 grip;      // the weapon grip: the near hand
    vec2 gripDir;   // the weapon's direction from the grip
    float lean;     // torso lean (radians, + forward)
};

// Two-bone IK: the middle joint for a limb from root to target with segment
// lengths a and b, bending to the side `bend` (+1 / -1) of root -> target.
vec2 ik(vec2 root, vec2 target, float a, float b, float bend)
{
    vec2 d = target - root;
    float len = clamp(length(d), 1e-3, a + b - 1e-3);
    vec2 dir = d / max(length(d), 1e-3);
    float x = (a * a - b * b + len * len) / (2.0 * len);
    float y = sqrt(max(a * a - x * x, 0.0));
    return root + dir * x + vec2(-dir.y, dir.x) * y * bend;
}

// A hand or foot target pulled back within reach of its root.
vec2 reach(vec2 root, vec2 target, float maxLength)
{
    vec2 d = target - root;
    float l = length(d);
    return l > maxLength ? root + d / l * maxLength : target;
}

Rig rigFor(int pose, float ph, float time, float seed, uint weapon)
{
    float wave = sin(ph * 2.0 * kAnimePi);
    float lift = cos(ph * 2.0 * kAnimePi);
    float breathe = pose == kPoseIdle ? sin(time * 2.6 + seed * 6.0) : 0.0;
    float bob = breathe * 0.5;
    if (pose == kPoseWalk) {
        bob = -abs(wave) * 3.2 + 1.2;
    } else if (pose == kPoseJump) {
        bob = -2.0;
    } else if (pose == kPoseAttack) {
        bob = ph < 0.4 ? ph * 3.0 : 1.2 - (ph - 0.4) * 2.0;
    } else if (pose == kPoseCheer) {
        bob = -abs(sin(time * 6.0)) * 2.5;
    }
    Rig r;
    r.lean = 0.0;
    r.head = vec2(2.5, -7.0 + bob);
    r.neck = vec2(2.0, 13.0 + bob);
    r.shoulderN = vec2(17.5, 18.5 + bob);
    r.shoulderF = vec2(-12.5, 18.5 + bob);
    r.hipN = vec2(7.5, 37.0 + bob);
    r.hipF = vec2(-3.5, 37.0 + bob);
    r.footN = vec2(9.5, 56.0);
    r.footF = vec2(-4.5, 56.0);
    r.handN = vec2(21.5, 33.5 + bob);
    r.handF = vec2(-16.5, 33.0 + bob);
    float bendN = 1.0;  // elbows bend backwards (towards -x) when the arm hangs
    float bendF = 1.0;

    if (pose == kPoseIdle) {
        r.handN += vec2(0.0, breathe * 0.4);
    } else if (pose == kPoseWalk) {
        // Legs and arms swing in opposition; the forward foot lifts.
        // Legs swing about 32 degrees each way, arms in opposition.
        r.footN = vec2(9.0 + 10.0 * wave, 56.0 - max(0.0, lift) * 5.5);
        r.footF = vec2(-4.0 - 10.0 * wave, 56.0 - max(0.0, -lift) * 5.5);
        r.handN = vec2(21.0 - 9.0 * wave, 32.5 + bob - max(0.0, -wave) * 4.0);
        r.handF = vec2(-16.0 + 9.0 * wave, 32.5 + bob - max(0.0, wave) * 4.0);
    } else if (pose == kPoseJump) {
        r.footN = vec2(13.0, 50.0);
        r.footF = vec2(-6.0, 53.0);
        r.handN = vec2(25.0, 8.0);
        r.handF = vec2(-19.0, 6.0);
        bendN = -1.0;
    } else if (pose == kPoseAttack) {
        r.footN = vec2(15.0, 56.0);
        r.footF = vec2(-8.0, 56.0);
        if (weapon == kWeaponBow) {
            // Bow arm forward, string hand pulls back to the cheek.
            float draw = smoothstep(0.0, 0.4, ph) * (ph < 0.6 ? 1.0 : 0.0);
            r.handN = vec2(32.0, 22.0 + bob);
            r.handF = mix(vec2(20.0, 22.0), vec2(4.0, 16.0), draw) + vec2(0.0, bob);
            bendF = -1.0;
        } else if (weapon == kWeaponStaff) {
            float up = smoothstep(0.0, 0.4, ph) * (1.0 - smoothstep(0.7, 1.0, ph));
            r.handN = mix(vec2(28.0, 24.0), vec2(26.0, 2.0), up) + vec2(0.0, bob);
            bendN = -1.0;
        } else if (weapon == kWeaponDagger) {
            float stab = smoothstep(0.3, 0.45, ph) * (1.0 - smoothstep(0.7, 1.0, ph));
            r.handN = mix(vec2(12.0, 30.0), vec2(31.0, 24.0), stab) + vec2(0.0, bob);
        } else {
            // Wind-up (0..0.4): the hand goes up behind the head; swing
            // (0.4..0.6): down in front; then the follow-through.
            vec2 up = vec2(22.0, 5.0); // raised beside the head, blade up
            vec2 front = vec2(29.0, 28.0);
            r.handN = (ph < 0.4 ? mix(vec2(17.0, 33.0), up, smoothstep(0.0, 0.4, ph))
                                : mix(up, front, smoothstep(0.4, 0.58, ph))) + vec2(0.0, bob);
            bendN = ph < 0.45 ? -1.0 : 1.0;
        }
        r.handF = weapon == kWeaponBow ? r.handF : vec2(-12.0, 30.0 + bob);
        r.lean = ph < 0.4 ? -0.08 * smoothstep(0.0, 0.4, ph) : 0.1 * (1.0 - smoothstep(0.6, 1.0, ph));
    } else if (pose == kPoseHurt) {
        r.handN = vec2(24.0, 10.0);
        r.handF = vec2(-20.0, 8.0);
        r.footN = vec2(13.0, 55.0);
        r.footF = vec2(-6.0, 56.0);
        bendN = -1.0;
    } else if (pose == kPoseCheer) {
        float pump = sin(time * 8.0);
        r.handN = vec2(25.0, -2.0 + pump * 1.5 + bob);
        r.handF = vec2(-21.0, 0.0 - pump * 1.5 + bob);
        bendN = -1.0;
    } else if (pose == kPoseClimb) {
        // From behind: both hands up on the rope (x = 0), pulling in turn.
        r.head = vec2(0.0, -7.0);
        r.shoulderN = vec2(11.0, 14.0);
        r.shoulderF = vec2(-11.0, 14.0);
        r.hipN = vec2(5.5, 37.0);
        r.hipF = vec2(-5.5, 37.0);
        r.handN = vec2(4.0, -1.0 - wave * 3.0);
        r.handF = vec2(-4.0, -1.0 + wave * 3.0);
        r.footN = vec2(6.0, 56.0 - max(0.0, -wave) * 6.0);
        r.footF = vec2(-6.0, 56.0 - max(0.0, wave) * 6.0);
        bendN = 1.0;
        bendF = -1.0;
    }
    if (weapon == kWeaponStaff && (pose == kPoseIdle || pose == kPoseWalk)) {
        r.handN = vec2(27.0, 25.0 + bob - (pose == kPoseWalk ? wave * 1.5 : 0.0)); // holds the staff upright
    }
    float arm = kUpperArm + kForearm - 0.1;
    if (pose != kPoseClimb) {
        r.handN = reach(r.shoulderN, r.handN, arm);
        r.handF = reach(r.shoulderF, r.handF, arm);
    }
    r.elbowN = ik(r.shoulderN, r.handN, kUpperArm, kForearm, bendN);
    r.elbowF = ik(r.shoulderF, r.handF, kUpperArm, kForearm, bendF);
    vec2 ankleN = r.footN - vec2(1.0, 4.0);
    vec2 ankleF = r.footF - vec2(1.0, 4.0);
    r.kneeN = ik(r.hipN, ankleN, kThigh, kShin, -1.0); // knees bend forward
    r.kneeF = ik(r.hipF, ankleF, kThigh, kShin, -1.0);

    // The weapon: in the near hand, along the forearm or held its own way.
    r.grip = r.handN;
    vec2 forearm = normalize(r.handN - r.elbowN);
    if (weapon == kWeaponStaff) {
        r.gripDir = pose == kPoseAttack ? normalize(vec2(0.25, -1.0)) : vec2(0.03, -1.0);
    } else if (weapon == kWeaponBow) {
        r.gripDir = vec2(0.0, -1.0);
    } else if (pose == kPoseAttack && (weapon == kWeaponSword || weapon == kWeaponAxe)) {
        // Up beside the head on the wind-up, swung down to the front.
        float a = mix(-1.45, 0.75, smoothstep(0.4, 0.58, ph));
        r.gripDir = vec2(cos(a), sin(a));
    } else if (pose == kPoseAttack || pose == kPoseJump || pose == kPoseHurt) {
        r.gripDir = forearm;
    } else if (pose == kPoseCheer || weapon == kWeaponAxe) {
        r.gripDir = normalize(vec2(0.25, -1.0)); // held upright (an axe rests against the shoulder)
    } else {
        r.gripDir = normalize(vec2(0.78, 0.64)); // carried at the side, tip down and forward
    }
    return r;
}

// The fringe over the forehead (a function, so its cast shadow can evaluate
// it moved).
float fringeShape(vec2 p, vec2 hc, uint style)
{
    float d = animeHairLock(p, hc + vec2(-13.0, -17.0), hc + vec2(-18.0, -6.0), hc + vec2(-17.0, 5.0), 7.0);
    d = min(d, animeHairLock(p, hc + vec2(-5.0, -20.0), hc + vec2(-8.0, -9.0), hc + vec2(-6.0, -1.0), 6.5));
    d = min(d, animeHairLock(p, hc + vec2(3.0, -21.0), hc + vec2(5.0, -10.0), hc + vec2(2.0, -2.0), 6.5));
    d = min(d, animeHairLock(p, hc + vec2(11.0, -19.0), hc + vec2(15.0, -9.0), hc + vec2(10.5, 0.0), 6.0));
    d = min(d, animeHairLock(p, hc + vec2(18.0, -14.0), hc + vec2(23.0, -2.0), hc + vec2(19.0, 8.0), 5.5));
    if (style == kHairBob || style == kHairLong || style == kHairTwinTails) {
        d = min(d, animeHairLock(p, hc + vec2(19.0, -6.0), hc + vec2(24.0, 8.0), hc + vec2(20.0, 18.0), 5.5));
    }
    return d;
}

// The hair cap over the top of the head, cut at the hairline.
float capShape(vec2 p, vec2 head)
{
    return max(sdCircle(p - head - vec2(-1.0, -6.0), 24.0), p.y - head.y + 9.0 - (p.x - head.x) * 0.12);
}

float faceShape(vec2 p, vec2 head, float turn)
{
    return animeSmoothUnion(sdEllipse(p - head - vec2(0.0, -3.0), vec2(21.0, 20.0)),
                            sdEllipse(p - head - vec2(turn * 6.0, 8.0), vec2(12.5, 10.0)), 7.0); // the chin turns
}

// An arm from the rig: sleeve (upper arm), forearm and a round mitten hand,
// with a cuff for gloves. wide: a robe's bell sleeve.
void drawArm(vec2 shoulder, vec2 elbow, vec2 hand, bool wide, AnimeMaterial sleeve, AnimeMaterial forearm,
             AnimeMaterial handMat, bool gloved)
{
    if (wide) {
        animePart(C, sdLockBelly(P, shoulder, elbow, hand + (hand - elbow) * 0.12, 4.6, 6.2), sleeve);
    } else {
        animePart(C, sdSegment(P, elbow, hand, 3.5), forearm);
        animePart(C, sdSegment(P, shoulder, elbow, 4.4), sleeve);
    }
    animePart(C, sdCircle(P - hand, 4.4), handMat);
    if (gloved) {
        animePart(C, max(sdCircle(P - mix(elbow, hand, 0.7), 4.3), -sdCircle(P - hand, 3.4)), handMat);
    }
}

// A leg from the rig: thigh and shin (pants or skin), then the shoe pointing
// forward (+x) with a boot shaft for boots.
void drawLeg(vec2 hip, vec2 knee, vec2 foot, AnimeMaterial legMat, AnimeMaterial shinMat, AnimeMaterial shoeMat,
             uint shoes)
{
    vec2 ankle = foot - vec2(1.0, 4.0);
    animePart(C, sdSegment(P, knee, ankle, 4.0), shinMat);
    animePart(C, sdSegment(P, hip, knee, 4.7), legMat);
    float shoe = sdRoundBox(P - foot - vec2(2.2, -3.4), vec2(6.6, 3.6), 3.3);
    if (shoes != kShoesShoes) {
        float shaft = sdRoundBox(P - ankle - vec2(0.0, -1.5), vec2(4.8, 4.0), 2.0);
        animePart(C, shaft, shoeMat);
        animeLine(C, max(abs(P.y - ankle.y + 4.5), shaft), shoeMat.ink);
    }
    animePart(C, shoe, shoeMat);
}

// The weapon at the grip, then the near arm over its handle (and the armor's
// shoulder pad), drawn last.
void drawWeaponAndNearArm(Rig r, int pose, float ph)
{
    vec2 g = r.grip;
    vec2 dir = r.gripDir;
    vec2 side = vec2(-dir.y, dir.x);
    if (gWeapon == kWeaponSword || gWeapon == kWeaponAxe || gWeapon == kWeaponDagger) {
        float len = gWeapon == kWeaponDagger ? 16.0 : (gWeapon == kWeaponAxe ? 28.0 : 37.0);
        vec2 tip = g + dir * len;
        vec2 guard = g + dir * 4.0;
        if (pose == kPoseAttack && ph > 0.42 && ph < 0.75 && gWeapon != kWeaponDagger) {
            // The swing smear: a fading crescent the blade leaves behind.
            vec2 q = P - r.shoulderN;
            float ang = atan(q.y, q.x);
            float a1 = atan(r.handN.y - r.shoulderN.y, r.handN.x - r.shoulderN.x);
            float a0 = max(a1 - 1.8, -1.0); // only in front, never over the face
            float inArc = step(a0, ang) * step(ang, a1);
            float smear = (1.0 - smoothstep(0.0, 1.0, abs(length(q) - 30.0) / 9.0)) * inArc * smoothstep(0.75, 0.45, ph)
                          * smoothstep(a0, a1, ang);
            C.color = over(vec4(mix(vec3(1.0), gAccent, 0.35), smear * 0.85), C.color);
        }
        if (gWeapon == kWeaponAxe) {
            animePart(C, sdSegment(P, g - dir * 4.0, tip, 1.8), gWood);
            vec2 headAt = tip - dir * 4.0;
            float blade = sdLockBelly(P, headAt, headAt + side * 6.0 + dir * 1.0, headAt + side * 10.0, 3.0, 6.5);
            animePartShine(C, blade, gSteel, animeCover(C, abs(blade + 1.6) - 0.6) * 0.8, vec3(1.0));
        } else {
            float blade = sdLock(P, guard, mix(guard, tip, 0.5), tip, gWeapon == kWeaponDagger ? 2.4 : 3.6, 0.7);
            animePartShine(C, blade, gSteel, animeCover(C, abs(sdSegment(P, guard, tip, 0.0) + 0.9) - 0.5) * 0.9, vec3(1.0));
            animePart(C, sdSegment(P, guard - side * 4.5, guard + side * 4.5, 1.5), gTrim);
            animePart(C, sdSegment(P, g - dir * 3.5, guard, 1.6), gLeather);
            animePart(C, sdCircle(P - g + dir * 4.0, 1.9), gTrim); // pommel
        }
    } else if (gWeapon == kWeaponStaff) {
        // Upright beside the body: the orb above head height.
        // The hand slides along the staff so the orb never rises past
        // y = -40 (the quad's top is -64): a raised hand holds it lower down.
        float above = clamp((g.y + 38.0) / max(-dir.y, 0.2), 16.0, 56.0);
        vec2 tipAt = g + dir * above;
        vec2 butt = g - dir * (78.0 - above);
        animePart(C, sdSegment(P, butt, tipAt, 1.9), gWood);
        vec2 orbAt = tipAt + dir * 4.0;
        animePart(C, sdLock(P, tipAt - dir * 2.0, tipAt + side * 4.0, orbAt + side * 5.5 - dir * 2.0, 1.9, 0.6), gWood);
        float orb = sdCircle(P - orbAt, 5.0);
        float charge = pose == kPoseAttack ? smoothstep(0.0, 0.4, ph) * (1.0 - smoothstep(0.6, 1.0, ph)) : 0.3;
        C.color = over(vec4(gAccent * 1.4, glowFalloff(orb, 6.0 + 9.0 * charge) * (0.35 + 0.5 * charge)), C.color);
        animePart(C, orb, animeGlow(mix(gAccent, vec3(1.0), 0.35)));
        animeFlat(C, sdCircle(P - orbAt - vec2(-1.5, -1.7), 1.4), vec3(1.0));
    } else if (gWeapon == kWeaponBow) {
        float draw = pose == kPoseAttack && ph < 0.6 ? smoothstep(0.0, 0.4, ph) : 0.0;
        vec2 topB = g + vec2(-3.0 - draw * 2.0, -17.0);
        vec2 botB = g + vec2(-3.0 - draw * 2.0, 17.0);
        float bow = min(sdLock(P, topB, g + vec2(7.0, -9.0), g, 0.8, 1.9), sdLock(P, botB, g + vec2(7.0, 9.0), g, 0.8, 1.9));
        vec2 nock = draw > 0.0 ? r.handF : mix(topB, botB, 0.5);
        animeStroke(C, min(sdSegment(P, topB, nock, 0.0), sdSegment(P, nock, botB, 0.0)), 0.7, vec3(0.95, 0.92, 0.82));
        animePart(C, bow, gWood);
    }
    drawArm(r.shoulderN, r.elbowN, r.handN, gTop == kTopRobe, gCloth, gForearm, gHand, gGloves != kGlovesNone);
    if (gTop == kTopArmor) {
        float pad = max(sdEllipse(P - r.shoulderN - vec2(-0.5, -0.5), vec2(7.0, 5.5)), P.y - r.shoulderN.y - 2.5);
        animePartShine(C, pad, gCloth, animeCover(C, abs(sdEllipse(P - r.shoulderN - vec2(-1.5, -2.0), vec2(4.0, 2.5))) - 0.4) * 0.8, vec3(1.0));
    }
}

void main()
{
    SpriteData s = sprite;
    float k = s.size.y / 128.0;
    float facing = s.params.x < 0.0 ? -1.0 : 1.0;
    vec2 local = spriteLocal(inUv);
    vec2 p = vec2(local.x * facing, local.y) / k;
    int pose = int(s.params.y + 0.5);
    float ph = fract(s.params.z);
    float blend = clamp(s.params.w, 0.0, 1.0);
    uint bx = s.bits.x;
    uint by = s.bits.y;
    uint hairStyle = bx & 7u;
    uint hat = (bx >> 3) & 7u;
    uint top = (bx >> 6) & 7u;
    uint bottom = (bx >> 9) & 7u;
    uint shoes = (bx >> 12) & 3u;
    uint gloves = (bx >> 14) & 3u;
    uint weapon = (bx >> 16) & 7u;
    uint cape = (bx >> 19) & 3u;
    vec3 iris = animeHex(kEyes[(bx >> 21) & 7u]);
    vec3 skinColor = animeHex(kSkins[(bx >> 24) & 3u]);
    int exprA = int((bx >> 26) & 15u);
    bool debugRig = ((bx >> 30) & 1u) != 0u;
    int exprB = int(by & 15u);
    vec3 hatColor = animePalette((by >> 4) & 15u);
    vec3 bottomColor = animePalette((by >> 8) & 15u);
    vec3 shoeColor = animePalette((by >> 12) & 15u);
    vec3 capeColor = animePalette((by >> 16) & 15u);
    vec3 accent = animePalette((by >> 20) & 15u);
    float seed = float((by >> 24) & 0xffu) / 255.0;
    float time = s.time;
    bool climbing = pose == kPoseClimb;
    float turn = climbing ? 0.0 : (pose == kPoseCheer ? 0.2 : 0.42);
    float wave = sin(ph * 2.0 * kAnimePi);

    // Hurt: the whole body leans back around the feet.
    if (pose == kPoseHurt) {
        p = rotate2d(p - vec2(0.0, 56.0), 0.2 * (1.0 - ph)) + vec2(-3.0 * (1.0 - ph), 56.0);
    }
    float breathe = pose == kPoseIdle ? sin(time * 2.6 + seed * 6.0) : 0.0;
    p = animeSquash(p, vec2(0.0, 56.0), 1.0 + breathe * 0.012);
    P = p;

    Rig r = rigFor(pose, ph, time, seed, weapon);
    // The torso lean turns the upper body around the hips.
    if (r.lean != 0.0) {
        vec2 pivot = vec2(2.0, 37.0);
        r.head = rotate2d(r.head - pivot, r.lean) + pivot;
        r.neck = rotate2d(r.neck - pivot, r.lean) + pivot;
    }

    C = animeBegin(p, 3.6); // soft, thick outlines: about 4% of the avatar's height
    C.shadeDepth = 4.5;
    AnimeMaterial skin = animeSkin(skinColor);
    AnimeMaterial hair = animeHair(s.color.rgb);
    AnimeMaterial cloth = top == kTopArmor ? animeMetal(s.color2.rgb) : animeCloth(s.color2.rgb);
    AnimeMaterial trim = animeMetal(accent);
    AnimeMaterial bottomMat = animeCloth(bottomColor);
    AnimeMaterial shoeMat = shoes == kShoesGreaves ? animeMetal(shoeColor) : animeMaterial(shoeColor);
    AnimeMaterial capeMat = animeCloth(capeColor);
    AnimeMaterial steel = animeMetal(animeHex(0xCAD6EAu));
    AnimeMaterial wood = animeMaterial(animeHex(0xA0683Au));
    AnimeMaterial leather = animeMaterial(animeHex(0x7A4A2Eu));
    AnimeMaterial handMat = gloves == kGlovesNone ? skin : (gloves == kGlovesCloth ? animeCloth(accent) : steel);
    AnimeMaterial forearmMat = top == kTopTunic || top == kTopVest ? skin : cloth;
    gWeapon = weapon;
    gTop = top;
    gGloves = gloves;
    gAccent = accent;
    gCloth = cloth;
    gForearm = forearmMat;
    gHand = handMat;
    gTrim = trim;
    gSteel = steel;
    gWood = wood;
    gLeather = leather;
    float sway = (pose == kPoseWalk ? wave : sin(time * 1.7 + seed * 4.0) * 0.4) * 2.0;
    vec2 head = r.head;
    float zipX = r.neck.x + turn * 7.0; // the torso's middle line, turned towards +x

    // ---- 1. cape and things on the back
    if (cape == kCapeCape) {
        vec2 a = r.neck + vec2(-6.0, 3.0);
        float capeD = sdLockBelly(p, a, a + vec2(-12.0 - sway, 16.0), vec2(-15.0 - sway * 2.0, 52.0), 8.0, 10.0);
        capeD = max(capeD, -(p.x - r.neck.x - 2.0));
        animePart(C, capeD, capeMat);
        animeLine(C, max(sdBezier(p, a + vec2(-4.0, 12.0), a + vec2(-9.0, 24.0), vec2(-13.0 - sway * 2.0, 48.0)), capeD + 0.6), capeMat.ink);
    }
    if (weapon == kWeaponBow && !climbing) {
        vec2 q0 = r.neck + vec2(-10.0, 0.0);
        vec2 q1 = r.neck + vec2(-4.0, 20.0);
        animePart(C, sdSegment(p, q0, q1, 3.2), leather);
        for (int i = 0; i < 3; ++i) {
            vec2 f = q0 + vec2(-1.0 + float(i) * 2.4, -3.5 - float(i));
            animePart(C, sdLock(p, f + vec2(0.0, 3.0), f + vec2(-0.5, 0.0), f + vec2(-1.0, -4.0), 1.5, 0.3),
                      animeMaterial(animeHex(0xF4F1E6u)));
        }
    }

    // ---- 2. back hair (behind the head and shoulders only)
    if (hairStyle == kHairPonytail) {
        vec2 tie = head + vec2(-19.0, -12.0);
        animePart(C, sdLockBelly(p, tie, tie + vec2(-11.0 - sway, 8.0), tie + vec2(-12.0 - sway * 2.0, 34.0), 7.0, 0.8), hair);
        animePart(C, sdLock(p, tie, tie + vec2(-5.0 - sway, 12.0), tie + vec2(-3.0 - sway * 2.0, 28.0), 5.0, 0.6), hair);
    } else if (hairStyle == kHairLong) {
        float curtain = sdLockBelly(p, head + vec2(-8.0, -6.0), head + vec2(-16.0, 20.0), head + vec2(-14.0 - sway, 40.0), 19.0, 6.0);
        curtain = max(curtain, p.x - head.x - 6.0); // never over the near arm
        animePart(C, curtain, hair);
        animeLine(C, max(sdBezier(p, head + vec2(-14.0, 14.0), head + vec2(-18.0, 26.0), head + vec2(-15.0 - sway, 37.0)), curtain + 0.8), hair.ink);
    } else if (hairStyle == kHairTwinTails) {
        vec2 tie = head + vec2(-20.0, -14.0);
        animePart(C, sdLockBelly(p, tie, tie + vec2(-9.0 - sway, 12.0), tie + vec2(-7.0 - sway * 1.5, 36.0), 6.5, 1.0), hair);
    } else if (hairStyle == kHairBob) {
        animePart(C, sdRoundBox(p - head - vec2(-4.0, 3.0), vec2(22.0, 19.0), 12.0), hair);
    }
    float backHair = sdCircle(p - head - vec2(-3.0, -5.0), 25.0);
    if (hairStyle == kHairShort) {
        backHair = min(backHair, animeHairLock(p, head + vec2(-15.0, -15.0), head + vec2(-24.0, -17.0), head + vec2(-32.0, -12.0), 7.0));
        backHair = min(backHair, animeHairLock(p, head + vec2(-15.0, -2.0), head + vec2(-25.0, -1.0), head + vec2(-31.0, 5.0), 6.5));
        backHair = min(backHair, animeHairLock(p, head + vec2(-11.0, 7.0), head + vec2(-18.0, 11.0), head + vec2(-22.0, 15.0), 5.5));
    }
    animePart(C, backHair, hair);

    // ---- 3. far arm (behind the torso, a shade darker)
    AnimeMaterial sleeveFar = animeTinted(cloth, kFar);
    if (!climbing) {
        drawArm(r.shoulderF, r.elbowF, r.handF, top == kTopRobe, sleeveFar, animeTinted(forearmMat, kFar), animeTinted(handMat, kFar),
                gloves != kGlovesNone);
        if (weapon == kWeaponBow && pose == kPoseAttack && ph < 0.6) {
            // The arrow, nocked by the string hand.
            animeStroke(C, sdSegment(p, r.handF, r.handN + vec2(8.0, 0.0), 0.0), 1.2, animeHex(0x5A3A22u));
        }
    }

    // ---- 4-6. legs and the bottom
    AnimeMaterial legMat = bottom == kBottomPants && top != kTopRobe ? bottomMat : skin;
    AnimeMaterial thighMat = bottom == kBottomSkirt || top == kTopRobe ? skin : bottomMat;
    drawLeg(r.hipF, r.kneeF, r.footF, animeTinted(thighMat, kFar), animeTinted(legMat, kFar), animeTinted(shoeMat, kFar), shoes);
    drawLeg(r.hipN, r.kneeN, r.footN, thighMat, legMat, shoeMat, shoes);
    vec2 waist = vec2(r.neck.x + 0.5, r.hipN.y - 3.0);
    if (top != kTopRobe) {
        if (bottom == kBottomSkirt) {
            float flare = 1.5 + abs(wave) * 1.5;
            float skirt = sdTrapezoid(p - waist - vec2(0.5, 4.5), 15.0 + flare, 11.5, 5.5) - 1.5;
            animePart(C, skirt, bottomMat);
            for (int i = -1; i <= 2; ++i) {
                float x = waist.x + float(i) * 5.0 + 0.5;
                animeLine(C, max(sdSegment(p, vec2(x - float(i) * 0.6, waist.y + 0.5), vec2(x + float(i) * 0.8, waist.y + 11.0), 0.0), skirt + 0.6), bottomMat.ink);
            }
        } else {
            float len = bottom == kBottomShorts ? 5.0 : 3.5;
            animePart(C, sdTrapezoid(p - waist - vec2(0.5, len), 13.5, 12.0, len) - 1.5, bottomMat);
            if (bottom == kBottomShorts) {
                animeLine(C, max(sdSegment(p, waist + vec2(3.0, 3.0), waist + vec2(3.0, 9.0), 0.0), 0.0), bottomMat.ink);
            }
        }
    }

    // ---- 7. the top: a rounded trapezoid, shoulders wider than the waist,
    // turned like the face (the zip / buttons / collar off-center)
    vec2 chest = vec2(r.neck.x + 1.0, (r.neck.y + r.hipN.y) * 0.5 - 1.0);
    float torsoHalf = (r.hipN.y - r.neck.y) * 0.5;
    if (top == kTopRobe) {
        float robe = sdTrapezoid(p - vec2(chest.x + 0.5, (r.neck.y + 50.0) * 0.5), 17.5 + abs(wave) * 1.5, 13.0,
                                 (50.0 - r.neck.y) * 0.5) - 3.0;
        animePart(C, robe, cloth);
        float hemY = 49.5;
        animePartNoLine(C, max(robe, -(p.y - hemY)), trim);
        animeLine(C, max(abs(p.y - hemY), robe), cloth.ink);
        // The front opening, turned towards +x.
        float front = abs(p.x - zipX - (p.y - r.neck.y) * 0.12) - 1.5;
        animePartNoLine(C, max(robe, front), trim);
        animeLine(C, max(abs(front), robe), cloth.ink);
        animeLine(C, max(sdBezier(p, vec2(chest.x - 7.0, 30.0), vec2(chest.x - 9.0, 40.0), vec2(chest.x - 11.0, 49.0)), robe + 0.6), cloth.ink);
        animePart(C, sdRoundBox(p - vec2(zipX + 1.0, r.hipN.y - 4.0), vec2(14.0, 1.6), 1.6), animeMaterial(accent)); // sash
    } else {
        float torso = sdTrapezoid(p - chest, 12.5 + abs(wave) * 0.4, 15.0, torsoHalf) - 3.0;
        animePart(C, torso, cloth);
        float hemY = r.hipN.y + 1.0;
        if (top == kTopTunic) {
            animePartNoLine(C, max(torso, -(p.y - hemY + 2.5)), trim);
            animeLine(C, max(abs(p.y - hemY + 2.5), torso), cloth.ink);
            // A V collar at the turned middle.
            animeLine(C, max(sdBezier(p, vec2(zipX - 5.0, r.neck.y + 0.5), vec2(zipX, r.neck.y + 6.0), vec2(zipX + 4.0, r.neck.y + 0.5)), -torso), cloth.ink);
        } else if (top == kTopVest) {
            float shirt = max(torso, abs(p.x - zipX) - 3.5);
            animePartNoLine(C, shirt, animeCloth(animeHex(0xF4EEE0u)));
            animeLine(C, max(abs(abs(p.x - zipX) - 3.5), torso), cloth.ink);
            for (int i = 0; i < 2; ++i) {
                animePart(C, sdCircle(p - vec2(zipX, r.neck.y + 6.0 + float(i) * 6.0), 0.9), trim);
            }
        } else if (top == kTopArmor) {
            // A chest plate with a ridge on the turned middle and rivets.
            animeLine(C, max(abs(p.x - zipX), torso + 2.0), cloth.ink);
            animeLine(C, max(abs(p.y - r.neck.y - 11.0), torso), cloth.ink);
            animeGloss(C, p, torso, vec2(zipX - 6.0, r.neck.y + 6.0), vec2(4.0, 3.0), 0.5);
            for (int i = -1; i <= 1; i += 2) {
                animePart(C, sdCircle(p - vec2(zipX + float(i) * 6.0, r.neck.y + 5.0), 1.0), trim);
            }
        } else {
            // Sailor blouse: a square collar with a stripe and a ribbon.
            AnimeMaterial navy = animeCloth(animeHex(0x3D4C92u));
            float collarShape = sdTrapezoid(p - vec2(zipX - 1.5, r.neck.y + 3.5), 13.0, 10.0, 3.5);
            float collar = max(collarShape, torso);
            animePart(C, collar, navy);
            animeLine(C, max(abs(collarShape + 1.4), collar), vec3(0.9));
            vec2 knot = vec2(zipX, r.neck.y + 7.5);
            AnimeMaterial ribbon = animeCloth(accent);
            animePart(C, sdLock(p, knot, knot + vec2(-3.0, 3.0), knot + vec2(-2.5, 8.0), 2.0, 1.0), ribbon);
            animePart(C, sdLock(p, knot, knot + vec2(3.0, 3.0), knot + vec2(3.5, 8.0), 2.0, 1.0), ribbon);
            animePart(C, sdCircle(p - knot, 2.0), ribbon);
        }
        if (top != kTopSailor) {
            float beltY = r.hipN.y - 4.0;
            float belt = max(torso, abs(p.y - beltY) - 1.7);
            animePart(C, belt, leather);
            animePart(C, sdRoundBox(p - vec2(zipX, beltY), vec2(2.1, 2.1), 0.6), trim);
        }
    }

    // ---- 8. neck and head
    float neckD = sdSegment(p, r.neck + vec2(0.0, -4.0), r.neck + vec2(0.0, 1.0), 3.4);
    animePart(C, neckD, skin);
    animeCastShadow(C, neckD, faceShape(p - vec2(0.8, 3.0), head, turn), skin);
    float face = faceShape(p, head, turn);
    animePart(C, face, skin);
    if (turn > 0.15) {
        float ear = sdEllipse(p - head - vec2(-13.0, 4.0), vec2(3.4, 4.4));
        animePart(C, max(ear, -face - 0.2), skin);
    }
    vec2 hc = head + vec2(turn * 2.0, 0.0);
    // ---- 9. face (with the hair's shadow on the forehead first)
    if (!climbing) {
        vec2 drop = vec2(1.0, 2.8);
        animeCastShadow(C, face, min(fringeShape(p - drop, hc, hairStyle), capShape(p - drop, head)), skin);
        AnimeFace f = animeFaceMix(animeExpression(exprA), animeExpression(exprB), blend);
        if (pose == kPoseHurt) {
            f = animeExpression(kAnimeHurt);
        } else if (pose == kPoseAttack && f.eyeShape == kAnimeEyeOpen) {
            f = animeFaceMix(f, animeExpression(kAnimeDetermined), 0.8);
        } else if (pose == kPoseCheer) {
            f = animeExpression(kAnimeHappy);
        }
        if (f.eyeShape == kAnimeEyeOpen) {
            f.open *= animeBlink(time, seed);
            if (f.open < 0.12) {
                f.eyeShape = kAnimeEyeClosed;
            }
        }
        animeFaceDraw(C, p, head + vec2(0.0, -1.0), 50.0, turn, 1.0, f, iris, skinColor);
    }

    // ---- 10. front hair: cap and fringe with the sheen band
    float cap = capShape(p, head);
    float fringe = climbing ? sdCircle(p - head, 21.0) : fringeShape(p, hc, hairStyle);
    float shine = animeHairShine(C, p, head + vec2(-1.0, -5.0), 17.5, 2.8);
    vec3 sheen = mix(s.color.rgb, vec3(1.0), 0.32);
    animePartShine(C, cap, hair, shine, sheen);
    animePartShine(C, fringe, hair, climbing ? 0.0 : shine, sheen);
    animeLine(C, max(sdBezier(p, hc + vec2(-2.0, -27.0), hc + vec2(-6.0, -20.0), hc + vec2(-9.0, -13.0)), cap + 0.8), hair.ink);
    animeLine(C, max(sdBezier(p, hc + vec2(6.0, -27.0), hc + vec2(8.0, -20.0), hc + vec2(9.0, -13.0)), cap + 0.8), hair.ink);
    if (hairStyle == kHairPonytail) {
        animePart(C, sdCircle(p - head - vec2(-19.0, -12.0), 3.2), animeMaterial(accent));
    } else if (hairStyle == kHairTwinTails) {
        animePart(C, sdCircle(p - head - vec2(-20.0, -14.0), 3.0), animeMaterial(accent));
        if (!climbing) {
            // The near tail hangs in front of the near shoulder's edge.
            vec2 tie = head + vec2(15.0, -15.0);
            animePart(C, sdLockBelly(p, tie, tie + vec2(6.0 - sway, 10.0), tie + vec2(5.0 - sway, 24.0), 5.5, 1.0), hair);
            animePart(C, sdCircle(p - tie, 3.0), animeMaterial(accent));
        }
    }

    // ---- 11. hat: turned with the face (offset towards +x, the far side shorter)
    AnimeMaterial hatMat = animeCloth(hatColor);
    vec2 hp = rotate2d(p - head, 0.06 * turn) + head; // a slight tilt with the turn
    if (hat == kHatWizard) {
        vec2 brim = head + vec2(turn * 6.0, -24.0);
        vec2 radii = vec2(hp.x < brim.x ? 24.0 : 30.0, 5.5);
        animePart(C, sdEllipse(hp - brim, radii), hatMat);
        // The crown sits on the brim: a flared base that tapers into a bent tip;
        // the band wraps the base.
        vec2 base = brim + vec2(1.0, -1.0);
        float crown = sdTrapezoid(hp - base - vec2(0.0, -6.0), 14.0, 10.5, 6.0) - 1.0;
        vec2 tip = base + vec2(-15.0 - sway * 0.5, -25.0);
        crown = animeSmoothUnion(crown, sdLock(hp, base + vec2(0.0, -10.0), base + vec2(2.0, -24.0), tip, 10.0, 1.6), 3.0);
        animePart(C, crown, hatMat);
        float band = max(crown + 0.5, abs(hp.y - base.y + 4.0) - 2.6);
        animePartNoLine(C, band, trim);
        animeLine(C, max(abs(abs(hp.y - base.y + 4.0) - 2.6), crown), hatMat.ink);
        animePart(C, sdCircle(hp - tip, 2.6), trim);
    } else if (hat == kHatHeadband) {
        float band = max(cap - 0.5, abs(p.y - (head.y - 13.0 - (p.x - head.x) * 0.1)) - 2.0);
        animePart(C, band, hatMat);
        vec2 knot = head + vec2(-22.0, -13.0);
        animePart(C, sdLock(p, knot, knot + vec2(-6.0, 2.0 - sway), knot + vec2(-12.0, 6.0 - sway), 2.2, 0.8), hatMat);
        animePart(C, sdLock(p, knot, knot + vec2(-5.0, 5.0 - sway), knot + vec2(-8.0, 11.0 - sway), 2.0, 0.7), hatMat);
    } else if (hat == kHatBeanie) {
        vec2 hb = head + vec2(turn * 2.0 - 1.0, -9.0);
        float beanie = max(sdCircle(hp - hb, 24.5), hp.y - head.y + 15.0);
        animePart(C, beanie, hatMat);
        float rim = max(sdCircle(hp - hb, 25.5), abs(hp.y - head.y + 14.0) - 3.0);
        animePart(C, rim, animeTinted(hatMat, vec3(0.9)));
        for (int i = -2; i <= 2; ++i) {
            animeLine(C, max(abs(hp.x - hb.x - float(i) * 6.0 - turn * 2.0), rim + 0.8), hatMat.ink);
        }
        animePart(C, sdCircle(hp - hb - vec2(-3.0, -26.0), 5.0), animeCloth(animePalette(0u)));
    } else if (hat == kHatHelmet) {
        vec2 hb = head + vec2(turn * 2.5 - 1.0, -8.0);
        AnimeMaterial metal = animeMetal(hatColor);
        float helm = max(sdCircle(hp - hb, 25.5), hp.y - head.y + 10.0);
        animePartShine(C, helm, metal, animeCover(C, abs(sdCircle(hp - hb - vec2(-3.0, -6.0), 15.0)) - 1.0) * 0.7, vec3(1.0));
        float rim = max(sdCircle(hp - hb, 26.5), abs(hp.y - head.y + 11.5) - 2.2);
        animePart(C, rim, metal);
        animeLine(C, max(abs(hp.x - hb.x - turn * 9.0), helm + 1.5), metal.ink); // the crest ridge, turned
        float plume = sdLockBelly(hp, hb + vec2(-2.0, -24.0), hb + vec2(-16.0, -34.0 - sway), hb + vec2(-30.0, -22.0), 4.5, 1.0);
        animePart(C, plume, animeCloth(accent));
    } else if (hat == kHatRibbon) {
        vec2 bow = head + vec2(10.0 + turn * 3.0, -27.0);
        AnimeMaterial ribbon = animeCloth(hatColor);
        animePart(C, sdLockBelly(p, bow, bow + vec2(-6.0, -8.0), bow + vec2(-12.0, -4.0), 2.0, 5.0), ribbon);
        animePart(C, sdLockBelly(p, bow, bow + vec2(6.0, -8.0), bow + vec2(13.0, -3.0), 2.0, 5.0), ribbon);
        animePart(C, sdCircle(p - bow, 3.0), ribbon);
    }
    if (cape == kCapeScarf) {
        float wrap = sdRoundBox(p - r.neck - vec2(1.0, 0.5), vec2(8.5, 3.2), 3.0);
        float tail = sdLock(p, r.neck + vec2(-6.0, 1.0), r.neck + vec2(-13.0, 2.0 + sway), r.neck + vec2(-21.0 - sway, 7.0 + sway), 3.0, 1.2);
        animePart(C, tail, capeMat);
        animePart(C, wrap, capeMat);
    }

    // ---- climbing: both arms over the back of the head, no weapon
    if (climbing) {
        AnimeMaterial sleeve = cloth;
        drawArm(r.shoulderF, r.elbowF, r.handF, top == kTopRobe, animeTinted(sleeve, kFar), animeTinted(forearmMat, kFar),
                animeTinted(handMat, kFar), gloves != kGlovesNone);
        drawArm(r.shoulderN, r.elbowN, r.handN, top == kTopRobe, sleeve, forearmMat, handMat, gloves != kGlovesNone);
    } else {
        drawWeaponAndNearArm(r, pose, ph);
    }

    outColor = animeFinish(C);

    if (debugRig) {
        // The skeleton over the finished avatar: bones as lines, anchors as dots.
        vec4 dbg = vec4(0.0);
        float lw = 1.2 * C.px;
        float bones = min(min(sdSegment(p, r.shoulderN, r.elbowN, 0.0), sdSegment(p, r.elbowN, r.handN, 0.0)),
                          min(sdSegment(p, r.shoulderF, r.elbowF, 0.0), sdSegment(p, r.elbowF, r.handF, 0.0)));
        bones = min(bones, min(min(sdSegment(p, r.hipN, r.kneeN, 0.0), sdSegment(p, r.kneeN, r.footN - vec2(1.0, 4.0), 0.0)),
                               min(sdSegment(p, r.hipF, r.kneeF, 0.0), sdSegment(p, r.kneeF, r.footF - vec2(1.0, 4.0), 0.0))));
        bones = min(bones, min(sdSegment(p, r.shoulderN, r.shoulderF, 0.0), sdSegment(p, r.hipN, r.hipF, 0.0)));
        bones = min(bones, min(sdSegment(p, r.neck, (r.hipN + r.hipF) * 0.5, 0.0), sdSegment(p, r.neck, r.head, 0.0)));
        dbg = over(vec4(vec3(1.0, 0.25, 0.6), animeCover(C, bones - lw)), dbg);
        float joints = 1e5;
        joints = min(joints, min(sdCircle(p - r.head, 2.0), sdCircle(p - r.neck, 1.6)));
        joints = min(joints, min(sdCircle(p - r.shoulderN, 1.6), sdCircle(p - r.shoulderF, 1.6)));
        joints = min(joints, min(sdCircle(p - r.elbowN, 1.4), sdCircle(p - r.elbowF, 1.4)));
        joints = min(joints, min(sdCircle(p - r.handN, 1.6), sdCircle(p - r.handF, 1.6)));
        joints = min(joints, min(sdCircle(p - r.hipN, 1.6), sdCircle(p - r.hipF, 1.6)));
        joints = min(joints, min(sdCircle(p - r.kneeN, 1.4), sdCircle(p - r.kneeF, 1.4)));
        joints = min(joints, min(sdCircle(p - r.footN, 1.6), sdCircle(p - r.footF, 1.6)));
        dbg = over(vec4(vec3(1.0, 0.95, 0.2), animeCover(C, joints)), dbg);
        float grip = sdSegment(p, r.grip, r.grip + r.gripDir * 10.0, 0.0);
        dbg = over(vec4(vec3(0.2, 1.0, 0.5), animeCover(C, grip - lw)), dbg);
        outColor = over(dbg, outColor);
    }
    outColor.a *= s.color.a;
}
