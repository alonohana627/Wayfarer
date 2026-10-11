#pragma once

// Monsters: the species (an archetype's look and behaviour at a level, with
// region-specific names and drops), their numbers by level and elite
// status, and the runtime monster with its small brain - patrol a platform
// without falling off, notice the hero, wind up and lunge, hop, charge or
// swoop, flinch when hit and pop when defeated.

#include "items.hpp"
#include "rules.hpp"

#include <gamelib/core/combat.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/core/tile_collision.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace wayfarer::game {

// The drawn creature (the monster shader's kinds).
enum class Archetype : std::uint8_t { Bellcap, Mochi, Turbanshell, Pudgy, Stumpling, Wisp, Pebblet, Flitter, Count };

enum class Behavior : std::uint8_t {
    Walker,  // patrols, walks at the hero when it notices it, a short lunge
    Hopper,  // moves in hops, a big hop at the hero
    Charger, // patrols, then a fast charge along its platform
    Flyer,   // floats over its home, drifts at the hero and swoops
};

enum class SpeciesId : std::uint8_t {
    Bellcap, PinkMochi, Pudgy, Stumpling,                 // forest
    Pebblet, Flitter, EmberMochi, LanternCap,             // mines
    Turbanshell, BogMochi, Murkcap, ReedFlitter,          // marsh
    Wisp, RunePebblet, OldStump, SandTurban,              // ruins
    SnowPudgy, FrostWisp, IcePebblet, FrostMochi,         // peak
    Count
};
inline constexpr int kSpeciesCount = static_cast<int>(SpeciesId::Count);

struct Species {
    std::string_view name;
    Archetype look = Archetype::Bellcap;
    Behavior behavior = Behavior::Walker;
    Region region = Region::Forest;
    int level = 1;
    float speed = 1.0f;        // walk speed, units per tick
    gamelib::core::Vec2 size;  // collision box
    Material material = Material::SoftCap;
};

[[nodiscard]] const Species& species(SpeciesId id);

// Numbers by level (elites: more HP, harder hits, more experience).
[[nodiscard]] int monsterHp(int level, bool elite);
[[nodiscard]] int monsterAttack(int level, bool elite);
[[nodiscard]] int monsterDefense(int level);
[[nodiscard]] int monsterXp(int level, bool elite);

enum class MonsterState : std::uint8_t { Spawn, Idle, Move, Windup, Attack, Hit, Defeat };

struct Monster {
    SpeciesId species = SpeciesId::Bellcap;
    bool elite = false;
    int level = 1;
    int spawn = -1;            // the spawn slot it came from
    gamelib::core::Rect box;
    gamelib::core::Rect previous; // last tick's box, for drawing
    gamelib::core::Vec2 velocity;
    gamelib::core::Vec2 home;     // flyers float around it
    float patrolLeft = 0.0f;      // the run of floor it keeps to
    float patrolRight = 0.0f;
    int facing = -1;
    MonsterState state = MonsterState::Spawn;
    int stateTicks = 0;           // ticks in the state
    int stateLength = 1;          // ticks the state lasts (for drawing progress)
    int hp = 1;
    int maxHp = 1;
    int attackCooldown = 0;
    int aggro = 0;                // ticks left chasing the hero
    int hopTimer = 0;
    bool grounded = false;
    int lastHitTick = -1000;      // the health bar shows for a while after a hit
    std::uint8_t seed = 0;
    gamelib::core::Vulnerable vulnerable;

    [[nodiscard]] const Species& kind() const { return wayfarer::game::species(species); }
    [[nodiscard]] bool alive() const { return state != MonsterState::Defeat; }
    // 0..1 through the current state, for the drawing.
    [[nodiscard]] float progress() const
    {
        return stateLength > 0 ? std::min(1.0f, static_cast<float>(stateTicks) / static_cast<float>(stateLength)) : 0.0f;
    }
};

// What the monster brain needs to know about the hero this tick.
struct HeroView {
    gamelib::core::Rect box;
    bool alive = true;
};

// What one monster tick produced.
struct MonsterStep {
    bool lunged = false;     // an attack started (a sound)
    bool landed = false;
    bool finished = false;   // the defeat animation ended: remove it
    bool contact = false;    // its body is dangerous this tick
    gamelib::core::Rect strike; // the lunge hitbox while attacking (empty otherwise)
};

// Creates a monster standing on the floor at `feet` (flyers float there).
[[nodiscard]] Monster makeMonster(SpeciesId id, int level, bool elite, gamelib::core::Vec2 feet, float patrolLeft,
                                  float patrolRight, std::uint8_t seed);

// One tick of a monster's brain and body.
MonsterStep stepMonster(Monster& m, const HeroView& hero, const gamelib::core::TileWorld& world, int tick);

// The monster was hit: knockback, a flinch, and the defeat when hp runs out.
void hurtMonster(Monster& m, int damage, int direction, int tick);

} // namespace wayfarer::game
