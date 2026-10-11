#pragma once

// The hero's growth, MapleStory-like: a level from 1 to 30 on an experience
// curve, four core stats (strength, agility, vitality, magic) raised with
// five ability points per level, skills learned with skill points, HP and
// MP, timed buffs, the 24-slot bag, five equipment slots and coins. Derived
// values (HP, MP, attack, magic, defense, critical chance, Speed and Jump)
// are recomputed from the base stats, gear and buffs; Speed and Jump turn
// into the platformer tuning, so they really change walking and jumping.

#include "items.hpp"
#include "rules.hpp"

#include <gamelib/core/items.hpp>
#include <gamelib/core/platformer.hpp>
#include <gamelib/core/progression.hpp>
#include <gamelib/core/skills.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace wayfarer::game {

using CoreStats = gamelib::core::Stats<Core, kCoreCount>;

// Everything the stat window shows beside the core stats.
struct Derived {
    CoreStats core;   // base points + gear + buffs
    int maxHp = 0;
    int maxMp = 0;
    int attack = 0;
    int magic = 0;
    int defense = 0;
    int crit = 0;     // percent
    int speed = 100;  // percent, capped at kSpeedCap
    int jump = 100;   // percent, capped at kJumpCap
};
inline constexpr int kSpeedCap = 140;
inline constexpr int kJumpCap = 120;

// A timed buff (Tailwind): what it adds while active.
struct BuffEffect {
    int speed = 0;
    int jump = 0;
};
inline constexpr int kTailwindBuff = 1;

[[nodiscard]] const gamelib::core::ExperienceCurve& experienceCurve();

class Character {
public:
    Character();

    int level = 1;
    std::int64_t xp = 0;
    // Ability points: kStartCore in each stat at level 1, five more a level.
    gamelib::core::AbilityPoints<Core, kCoreCount> points;
    gamelib::core::SkillBook skills;
    gamelib::core::Resource hp;
    gamelib::core::Resource mp;
    gamelib::core::Buffs<BuffEffect> buffs;
    gamelib::core::Inventory bag;
    gamelib::core::Equipment gear;
    std::int64_t coins = 0;
    std::array<int, 2> quick{kRedPotion, kBluePotion}; // the HP and MP quick slots (item ids)

    [[nodiscard]] Derived derived() const { return derivedWith(points.base()); }
    // The derived values as they would be with other base points (the stat
    // window's preview of a point before it is spent).
    [[nodiscard]] Derived derivedWith(const CoreStats& base) const;

    // Spends one ability point on a core stat; false without points.
    bool spendPoint(Core stat);
    // Spends every point by the balanced weights (STR 2 : AGI 1 : VIT 1 :
    // MAG 1); returns how many were spent.
    int autoAssign();
    // The core stat the next auto-assigned point would raise.
    [[nodiscard]] Core autoAssignNext() const;

    // Adds experience; level-ups grant points, raise HP and MP and refill them.
    gamelib::core::LevelUp gainXp(std::int64_t amount);
    [[nodiscard]] float xpProgress() const;
    [[nodiscard]] std::int64_t xpToNext() const;

    // Keeps HP and MP within the maxima after gear, stats or buffs changed.
    void refreshResources();

    // Movement from Speed and Jump.
    [[nodiscard]] gamelib::core::PlatformerTuning tuning() const;

    [[nodiscard]] int skillLevel(SkillId id) const { return skills.levelOf(static_cast<int>(id)); }
    [[nodiscard]] gamelib::core::Wearer wearer() const { return {.level = level, .job = 0}; }
    [[nodiscard]] int gearBonus(Bonus bonus) const;
};

// A skill level's numbers (SkillBook definitions: cost, cooldown, duration, value).
[[nodiscard]] const gamelib::core::SkillDef& skillDef(SkillId id);
// The character level that opens a skill's first point.
[[nodiscard]] int skillUnlockLevel(SkillId id);
// What a skill does at a level, for the skill window ("deals 240% damage").
[[nodiscard]] std::string skillEffectText(SkillId id, int level);
[[nodiscard]] std::string_view skillDescription(SkillId id);
// Starbolt targets per cast at a level (piercing from level 6).
[[nodiscard]] int starboltTargets(int level);

} // namespace wayfarer::game
