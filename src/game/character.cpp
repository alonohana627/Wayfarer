#include "character.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <vector>

namespace wayfarer::game {

namespace {

// helpers

using gamelib::core::SkillDef;
using gamelib::core::SkillLevel;

// Walking and jumping at 100% Speed and Jump (world units and ticks).
constexpr float kWalkSpeed = 5.2f;      // 312 units a second: six and a half tiles
constexpr float kJumpHeight = 150.0f;   // a little over three tiles
constexpr int kJumpApexTicks = 23;
// The hyper jump at 100% (its skill level scales the forward burst and the
// straight-up rise): a burst of 12 units a tick with a small lift, the input
// locked for 12 ticks and no air drag for 26 so it carries.
constexpr float kHyperForward = 12.0f;
constexpr float kHyperRise = 5.0f;
constexpr float kHyperUp = 15.0f;
constexpr int kHyperLockTicks = 12;
constexpr int kHyperGlideTicks = 26;

// Auto-assign weights (a balanced adventurer).
const CoreStats kAutoWeights{{2, 1, 1, 1}};

std::array<SkillDef, kSkillCount> buildSkills()
{
    std::array<SkillDef, kSkillCount> defs;
    const auto make = [&](const SkillId id, const int levels, const bool passive, auto&& level) {
        SkillDef& d = defs[static_cast<std::size_t>(id)];
        d.id = static_cast<int>(id);
        d.passive = passive;
        d.characterLevel = skillUnlockLevel(id);
        for (int l = 1; l <= levels; ++l) {
            d.levels.push_back(level(l));
        }
    };
    make(SkillId::Combo, 10, true, [](const int l) { return SkillLevel{.value = 6 * l}; });
    make(SkillId::HyperJump, 5, false, [](const int l) { return SkillLevel{.cost = 5 - l / 2, .value = 100 + 9 * (l - 1)}; });
    make(SkillId::Roll, 5, false, [](const int l) {
        return SkillLevel{.cost = 5 - l / 2, .cooldownTicks = 54 - 5 * l, .durationTicks = 22 + 2 * l, .value = 100 + 10 * l};
    });
    make(SkillId::Sweep, 10, false, [](const int l) {
        return SkillLevel{.cost = 8 + l, .cooldownTicks = 150 - 7 * l, .value = 180 + 22 * l};
    });
    make(SkillId::Starbolt, 10, false, [](const int l) {
        return SkillLevel{.cost = 6 + l / 2, .cooldownTicks = 38 - l, .value = 150 + 22 * l};
    });
    make(SkillId::Tailwind, 10, false, [](const int l) {
        return SkillLevel{.cost = 18 + l, .cooldownTicks = 90, .durationTicks = (40 + 10 * l) * kTickRate,
                          .value = 6 + (l * 14) / 10};
    });
    make(SkillId::Bloom, 10, false, [](const int l) {
        return SkillLevel{.cost = 20 + 2 * l, .cooldownTicks = 900 - 50 * l, .value = 18 + 3 * l};
    });
    return defs;
}

const std::array<SkillDef, kSkillCount>& skillDefs()
{
    static const std::array<SkillDef, kSkillCount> defs = buildSkills();
    return defs;
}

} // namespace

const gamelib::core::ExperienceCurve& experienceCurve()
{
    static const gamelib::core::ExperienceCurve curve = gamelib::core::ExperienceCurve::fromSteps(
        kMaxLevel, [](const int level) { return static_cast<std::int64_t>(20 + 8 * level + 6 * level * level); });
    return curve;
}

const SkillDef& skillDef(const SkillId id)
{
    return skillDefs()[static_cast<std::size_t>(id)];
}

int skillUnlockLevel(const SkillId id)
{
    switch (id) {
    case SkillId::Combo: return 1;
    case SkillId::HyperJump: return 1;
    case SkillId::Roll: return 2;
    case SkillId::Sweep: return 4;
    case SkillId::Starbolt: return 7;
    case SkillId::Tailwind: return 10;
    case SkillId::Bloom: return 13;
    case SkillId::Count: break;
    }
    return 1;
}

int starboltTargets(const int level)
{
    return level >= 6 ? 1 + level / 3 : 1;
}

std::string_view skillDescription(const SkillId id)
{
    switch (id) {
    case SkillId::Combo: return "Your three-hit sword combo grows stronger.";
    case SkillId::HyperJump: return "Jump again in the air to burst forward. Hold up to soar straight up.";
    case SkillId::Roll: return "Tumble forward, untouchable while you roll.";
    case SkillId::Sweep: return "A wide crescent slash that strikes everything around you twice.";
    case SkillId::Starbolt: return "Hurl a homing star of magic. At level 6 it pierces.";
    case SkillId::Tailwind: return "A friendly wind lifts your Speed and Jump.";
    case SkillId::Bloom: return "Flowers bloom around you and mend your wounds.";
    case SkillId::Count: break;
    }
    return "";
}

std::string skillEffectText(const SkillId id, const int level)
{
    const SkillDef& d = skillDef(id);
    if (level <= 0 || level > static_cast<int>(d.levels.size())) {
        return "";
    }
    const SkillLevel& l = d.levels[static_cast<std::size_t>(level - 1)];
    const auto seconds = [](const int ticks) { return std::format("{:.1f}s", static_cast<float>(ticks) / kTickRate); };
    switch (id) {
    case SkillId::Combo: return std::format("combo damage +{}%", l.value);
    case SkillId::HyperJump: return std::format("burst {}%, {} mp", l.value, l.cost);
    case SkillId::Roll: return std::format("{} invincible, {} mp, cooldown {}", seconds(l.durationTicks), l.cost, seconds(l.cooldownTicks));
    case SkillId::Sweep: return std::format("2 x {}% damage, {} mp, cooldown {}", l.value, l.cost, seconds(l.cooldownTicks));
    case SkillId::Starbolt:
        return std::format("{}% magic damage, {} target{}, {} mp", l.value, starboltTargets(level), starboltTargets(level) > 1 ? "s" : "",
                           l.cost);
    case SkillId::Tailwind:
        return std::format("speed +{}%, jump +{}% for {}s, {} mp", l.value, l.value / 2, l.durationTicks / kTickRate, l.cost);
    case SkillId::Bloom: return std::format("heals {}% hp + magic, {} mp, cooldown {}", l.value, l.cost, seconds(l.cooldownTicks));
    case SkillId::Count: break;
    }
    return "";
}

Character::Character()
    : points{CoreStats{{kStartCore, kStartCore, kStartCore, kStartCore}}}
    , bag{itemRegistry(), kBagSlots}
    , gear{itemRegistry(), kSlotCount}
{
    for (const SkillDef& d : skillDefs()) {
        skills.define(d);
    }
    skills.setLevel(static_cast<int>(SkillId::Combo), 1);
    const Derived d = derived();
    hp = {.current = d.maxHp, .max = d.maxHp};
    mp = {.current = d.maxMp, .max = d.maxMp};
}

int Character::gearBonus(const Bonus bonus) const
{
    int sum = 0;
    for (int s = 0; s < gear.size(); ++s) {
        sum += stackBonus(gear.at(s), bonus);
    }
    return sum;
}

Derived Character::derivedWith(const CoreStats& base) const
{
    Derived d;
    d.core = base;
    d.core[Core::Str] += gearBonus(Bonus::Str);
    d.core[Core::Agi] += gearBonus(Bonus::Agi);
    d.core[Core::Vit] += gearBonus(Bonus::Vit);
    d.core[Core::Mag] += gearBonus(Bonus::Mag);
    const int str = d.core[Core::Str];
    const int agi = d.core[Core::Agi];
    const int vit = d.core[Core::Vit];
    const int mag = d.core[Core::Mag];
    d.maxHp = 60 + 14 * (level - 1) + 8 * vit + gearBonus(Bonus::Hp);
    d.maxMp = 30 + 6 * (level - 1) + 6 * mag + gearBonus(Bonus::Mp);
    d.attack = gearBonus(Bonus::Attack) + 2 * str + agi / 2 + level;
    d.magic = gearBonus(Bonus::Magic) + 2 * mag + level;
    d.defense = gearBonus(Bonus::Defense) + vit;
    d.crit = std::min(5 + agi / 4 + gearBonus(Bonus::Crit), 50);
    int buffSpeed = 0;
    int buffJump = 0;
    for (const auto& b : buffs.all()) {
        buffSpeed += b.effect.speed;
        buffJump += b.effect.jump;
    }
    d.speed = std::min(100 + (agi - kStartCore) / 4 + gearBonus(Bonus::Speed) + buffSpeed, kSpeedCap);
    d.jump = std::min(100 + (agi - kStartCore) / 8 + gearBonus(Bonus::Jump) + buffJump, kJumpCap);
    return d;
}

bool Character::spendPoint(const Core stat)
{
    if (!points.spend(stat)) {
        return false;
    }
    refreshResources();
    return true;
}

int Character::autoAssign()
{
    const int spent = points.autoAssign(kAutoWeights);
    refreshResources();
    return spent;
}

gamelib::core::LevelUp Character::gainXp(const std::int64_t amount)
{
    const gamelib::core::LevelUp up = experienceCurve().gain(xp, amount);
    if (up.happened()) {
        level = up.to;
        points.grant(up.levels() * kPointsPerLevel);
        skills.addPoints(up.levels() * kSkillPointsPerLevel);
        refreshResources();
        hp.refill();
        mp.refill();
    }
    return up;
}

float Character::xpProgress() const
{
    return level >= kMaxLevel ? 1.0f : experienceCurve().progress(xp);
}

std::int64_t Character::xpToNext() const
{
    return level >= kMaxLevel ? 0 : experienceCurve().toNextLevel(xp);
}

void Character::refreshResources()
{
    const Derived d = derived();
    hp.max = d.maxHp;
    mp.max = d.maxMp;
    hp.current = std::min(hp.current, hp.max);
    mp.current = std::min(mp.current, mp.max);
}

gamelib::core::PlatformerTuning Character::tuning() const
{
    const Derived d = derived();
    gamelib::core::PlatformerTuning t;
    t.maxSpeed = kWalkSpeed;
    t.groundAccel = 1.0f;
    t.groundDecel = 1.3f;
    t.airAccel = 0.42f;
    t.airDecel = 0.16f;
    t.setJumpArc(kJumpHeight, kJumpApexTicks);
    t.maxFallSpeed = 15.0f;
    t.fallGravity = 1.12f;
    t.jumpCut = 0.55f;
    t.coyoteTicks = 6;
    t.jumpBufferTicks = 7;
    t.cornerCorrection = 10.0f;
    t.climbSpeed = 3.6f;
    t.climbJumpSpeed = {4.5f, 8.0f};
    t.dropThroughTicks = 12;
    // The hyper jump, stronger with its skill level (off until learned).
    const int hyper = skillLevel(SkillId::HyperJump);
    if (hyper > 0) {
        const float power = static_cast<float>(skillDef(SkillId::HyperJump).levels[static_cast<std::size_t>(hyper - 1)].value) / 100.0f;
        t.hyperForward = {kHyperForward * power, kHyperRise};
        t.hyperUp = kHyperUp * power;
        t.hyperLockTicks = kHyperLockTicks;
        t.hyperGlideTicks = kHyperGlideTicks;
    }
    // Speed and Jump percentages (gear, points, Tailwind) scale it all.
    return gamelib::core::scaledTuning(t, {.speed = static_cast<float>(d.speed) / 100.0f,
                                           .jump = static_cast<float>(d.jump) / 100.0f,
                                           .speedCap = static_cast<float>(kSpeedCap) / 100.0f,
                                           .jumpCap = static_cast<float>(kJumpCap) / 100.0f});
}

} // namespace wayfarer::game
