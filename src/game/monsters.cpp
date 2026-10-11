#include "monsters.hpp"

#include <gamelib/core/enemy_motion.hpp>

#include <array>
#include <cmath>

namespace wayfarer::game {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;

constexpr float kGravity = 0.62f;
constexpr float kMaxFall = 13.0f;
constexpr int kSpawnTicks = 32;
constexpr int kHitTicks = 18;
constexpr int kDefeatTicks = 44;

const std::array<Species, kSpeciesCount>& table()
{
    using A = Archetype;
    using B = Behavior;
    using R = Region;
    using M = Material;
    static const std::array<Species, kSpeciesCount> t{{
        {"Bellcap", A::Bellcap, B::Hopper, R::Forest, 1, 1.0f, {58, 74}, M::SoftCap},
        {"Pink Mochi", A::Mochi, B::Hopper, R::Forest, 2, 1.1f, {72, 52}, M::JellyDrop},
        {"Pudgy", A::Pudgy, B::Charger, R::Forest, 4, 1.2f, {74, 62}, M::Acorn},
        {"Stumpling", A::Stumpling, B::Walker, R::Forest, 5, 0.9f, {58, 82}, M::Twig},
        {"Pebblet", A::Pebblet, B::Walker, R::Mines, 7, 1.0f, {66, 60}, M::GlimmerOre},
        {"Flitter", A::Flitter, B::Flyer, R::Mines, 9, 1.6f, {62, 52}, M::FlitterFluff},
        {"Ember Mochi", A::Mochi, B::Hopper, R::Mines, 10, 1.2f, {72, 52}, M::EmberJelly},
        {"Lantern Cap", A::Bellcap, B::Hopper, R::Mines, 11, 1.1f, {58, 74}, M::GlimmerOre},
        {"Turbanshell", A::Turbanshell, B::Walker, R::Marsh, 13, 0.7f, {84, 66}, M::SwirlShell},
        {"Bog Mochi", A::Mochi, B::Hopper, R::Marsh, 14, 1.2f, {72, 52}, M::BogJelly},
        {"Murkcap", A::Bellcap, B::Hopper, R::Marsh, 16, 1.2f, {58, 74}, M::MarshCap},
        {"Reed Flitter", A::Flitter, B::Flyer, R::Marsh, 17, 1.8f, {62, 52}, M::FlitterFluff},
        {"Wisp", A::Wisp, B::Flyer, R::Ruins, 19, 1.5f, {58, 66}, M::WispLight},
        {"Rune Pebblet", A::Pebblet, B::Walker, R::Ruins, 21, 1.1f, {66, 60}, M::RunePebble},
        {"Old Stump", A::Stumpling, B::Walker, R::Ruins, 22, 1.0f, {58, 82}, M::OldBark},
        {"Sand Turban", A::Turbanshell, B::Charger, R::Ruins, 23, 0.9f, {84, 66}, M::SwirlShell},
        {"Snow Pudgy", A::Pudgy, B::Charger, R::Peak, 25, 1.4f, {74, 62}, M::FrostFur},
        {"Frost Wisp", A::Wisp, B::Flyer, R::Peak, 27, 1.7f, {58, 66}, M::IceShard},
        {"Ice Pebblet", A::Pebblet, B::Walker, R::Peak, 28, 1.2f, {66, 60}, M::IceShard},
        {"Frost Mochi", A::Mochi, B::Hopper, R::Peak, 29, 1.3f, {72, 52}, M::SnowFeather},
    }};
    return t;
}

int sign(const float v)
{
    return v < 0.0f ? -1 : 1;
}

void enter(Monster& m, const MonsterState state, const int length)
{
    m.state = state;
    m.stateTicks = 0;
    m.stateLength = std::max(length, 1);
}

// A deterministic wander rhythm per monster: walk for a while, stand for a while.
bool wandering(const Monster& m, const int tick)
{
    const int period = 150 + (m.seed % 5) * 30;
    return ((tick + m.seed * 37) % period) < period * 3 / 5;
}

} // namespace

const Species& species(const SpeciesId id)
{
    return table()[static_cast<std::size_t>(id)];
}

int monsterHp(const int level, const bool elite)
{
    const int hp = 30 + 14 * level + level * level * 13 / 5;
    return elite ? hp * 3 : hp;
}

int monsterAttack(const int level, const bool elite)
{
    const int atk = 6 + level * 16 / 5;
    return elite ? atk * 3 / 2 : atk;
}

int monsterDefense(const int level)
{
    return level * 6 / 5;
}

int monsterXp(const int level, const bool elite)
{
    const int xp = 5 + 3 * level + level * level * 7 / 20;
    return elite ? xp * 3 : xp;
}

Monster makeMonster(const SpeciesId id, const int level, const bool elite, const Vec2 feet, const float patrolLeft,
                    const float patrolRight, const std::uint8_t seed)
{
    const Species& s = species(id);
    Monster m;
    m.species = id;
    m.elite = elite;
    m.level = level;
    const Vec2 size = elite ? s.size * 1.3f : s.size;
    m.box = Rect::fromBottomCenter(feet, size);
    m.previous = m.box;
    m.home = m.box.center();
    m.patrolLeft = patrolLeft;
    m.patrolRight = patrolRight;
    m.facing = (seed & 1) != 0 ? 1 : -1;
    m.hp = monsterHp(level, elite);
    m.maxHp = m.hp;
    m.seed = seed;
    m.hopTimer = 20 + seed % 40;
    m.attackCooldown = 60;
    m.vulnerable = gamelib::core::Vulnerable{m.box, 2};
    enter(m, MonsterState::Spawn, kSpawnTicks);
    return m;
}

void hurtMonster(Monster& m, const int damage, const int direction, const int tick)
{
    if (!m.alive()) {
        return;
    }
    m.hp -= damage;
    m.lastHitTick = tick;
    m.aggro = 600;
    const bool flyer = m.kind().behavior == Behavior::Flyer;
    if (m.hp <= 0) {
        m.hp = 0;
        enter(m, MonsterState::Defeat, kDefeatTicks);
        m.velocity = flyer ? Vec2{direction * 2.0f, -1.0f} : Vec2{direction * 2.5f, -5.0f};
        return;
    }
    // Elites shrug off flinches while lunging.
    if (m.elite && m.state == MonsterState::Attack) {
        return;
    }
    enter(m, MonsterState::Hit, kHitTicks);
    m.facing = -direction;
    m.velocity = flyer ? Vec2{direction * 3.0f, -1.0f} : Vec2{direction * 3.5f, m.grounded ? -2.5f : m.velocity.y};
}

MonsterStep stepMonster(Monster& m, const HeroView& hero, const gamelib::core::TileWorld& world, const int tick)
{
    MonsterStep step;
    m.previous = m.box;
    m.vulnerable.tick();
    ++m.stateTicks;
    m.attackCooldown = std::max(0, m.attackCooldown - 1);
    m.aggro = std::max(0, m.aggro - 1);
    const Species& s = m.kind();
    const bool flyer = s.behavior == Behavior::Flyer;
    const Vec2 center = m.box.center();
    const Vec2 heroCenter = hero.box.center();
    const float dx = heroCenter.x - center.x;
    const float dy = heroCenter.y - center.y;

    if (m.state == MonsterState::Defeat) {
        if (!flyer) {
            m.velocity.y = std::min(m.velocity.y + kGravity, kMaxFall);
            m.velocity.x *= 0.9f;
            const gamelib::core::BoxMove move = gamelib::core::moveBox(m.box, m.velocity, world);
            m.box = move.box;
            if (move.contacts.ground) {
                m.velocity = {};
            }
        } else {
            m.box.x += m.velocity.x;
            m.box.y += m.velocity.y;
            m.velocity = m.velocity * 0.9f;
        }
        step.finished = m.stateTicks >= m.stateLength;
        return step;
    }
    if (m.state == MonsterState::Spawn) {
        if (m.stateTicks >= m.stateLength) {
            enter(m, MonsterState::Idle, 60);
        }
        if (!flyer) {
            m.velocity.y = std::min(m.velocity.y + kGravity, kMaxFall);
            const gamelib::core::BoxMove move = gamelib::core::moveBox(m.box, {0.0f, m.velocity.y}, world);
            m.box = move.box;
            m.grounded = move.contacts.ground;
            if (m.grounded) {
                m.velocity.y = 0.0f;
            }
        }
        m.vulnerable.setHurtbox(m.box);
        return step;
    }

    const float noticeX = flyer ? 380.0f : 300.0f;
    if (hero.alive && std::abs(dx) < noticeX && std::abs(dy) < (flyer ? 260.0f : 110.0f)
        && (s.behavior == Behavior::Charger || flyer || m.elite || m.level >= 7)) {
        m.aggro = std::max(m.aggro, 120);
    }
    if (!hero.alive) {
        m.aggro = 0;
    }
    const bool chasing = m.aggro > 0;

    if (m.state == MonsterState::Hit && m.stateTicks >= m.stateLength) {
        enter(m, MonsterState::Move, 40);
        m.attackCooldown = std::max(m.attackCooldown, 30);
    }

    if (flyer) {
        // Float around home; drift at the hero while chasing; swoop.
        const float t = static_cast<float>(tick) + m.seed * 13.0f;
        Vec2 target = m.home + Vec2{std::sin(t * 0.015f) * 90.0f, std::cos(t * 0.031f) * 26.0f};
        if (chasing) {
            target = heroCenter + Vec2{-sign(dx) * 70.0f, -70.0f};
            const Vec2 fromHome = target - m.home;
            const float far = std::sqrt(fromHome.x * fromHome.x + fromHome.y * fromHome.y);
            if (far > 520.0f) {
                target = m.home + fromHome * (520.0f / far);
            }
        }
        switch (m.state) {
        case MonsterState::Idle:
        case MonsterState::Move: {
            const Vec2 d = target - center;
            const float len = std::max(std::sqrt(d.x * d.x + d.y * d.y), 1e-3f);
            const float speed = s.speed * (chasing ? 1.3f : 0.7f);
            const Vec2 want = len > 4.0f ? d * (std::min(speed, len) / len) : Vec2{};
            m.velocity = m.velocity + (want - m.velocity) * 0.12f;
            if (std::abs(m.velocity.x) > 0.15f) {
                m.facing = sign(m.velocity.x);
            }
            if (chasing && m.attackCooldown == 0 && std::abs(dx) < 230.0f && std::abs(dy) < 200.0f) {
                enter(m, MonsterState::Windup, m.elite ? 26 : 34);
                m.facing = sign(dx);
            } else if (m.state == MonsterState::Idle && m.stateTicks > 40) {
                enter(m, MonsterState::Move, 60);
            }
            break;
        }
        case MonsterState::Windup:
            m.velocity = m.velocity * 0.85f + Vec2{0.0f, -0.08f};
            m.facing = sign(dx);
            if (m.stateTicks >= m.stateLength) {
                enter(m, MonsterState::Attack, 34);
                const float len = std::max(std::sqrt(dx * dx + dy * dy), 1.0f);
                m.velocity = Vec2{dx, dy} * ((m.elite ? 8.0f : 7.0f) / len);
                step.lunged = true;
            }
            break;
        case MonsterState::Attack:
            step.strike = m.box.expanded(4.0f);
            m.velocity = m.velocity * 0.97f;
            if (m.stateTicks >= m.stateLength) {
                enter(m, MonsterState::Move, 60);
                m.attackCooldown = 110 + m.seed % 40;
            }
            break;
        case MonsterState::Hit: m.velocity = m.velocity * 0.88f; break;
        default: break;
        }
        m.box.x += m.velocity.x;
        m.box.y += m.velocity.y;
        // Never far from home and never into the ground's thick part.
        m.box.y = std::max(m.box.y, m.home.y - 360.0f);
        m.box.y = std::min(m.box.y, m.home.y + 220.0f);
        step.contact = true;
        m.vulnerable.setHurtbox(m.box);
        return step;
    }

    // Ground monsters.
    const float speed = s.speed * (m.elite ? 1.15f : 1.0f);
    float vx = m.velocity.x;
    switch (m.state) {
    case MonsterState::Idle:
    case MonsterState::Move: {
        int dir = m.facing;
        bool moving = wandering(m, tick);
        if (chasing && std::abs(dx) > 10.0f) {
            dir = sign(dx);
            moving = true;
        }
        m.state = moving ? MonsterState::Move : MonsterState::Idle;
        // Keep to the patrol run and away from ledges.
        const bool edge = (dir < 0 && m.box.left() <= m.patrolLeft + 2.0f) || (dir > 0 && m.box.right() >= m.patrolRight - 2.0f)
                          || (m.grounded && gamelib::core::ledgeAhead(m.box, dir, world, 6.0f))
                          || gamelib::core::wallAhead(m.box, dir, world, 2.0f);
        if (edge) {
            if (chasing) {
                moving = false;
                m.facing = dir;
            } else {
                dir = -dir;
            }
        }
        m.facing = dir;
        if (s.behavior == Behavior::Hopper) {
            if (m.grounded) {
                vx *= 0.6f;
                if (moving && --m.hopTimer <= 0 && !edge) {
                    m.velocity.y = -6.5f;
                    vx = static_cast<float>(dir) * speed * 2.4f;
                    m.hopTimer = 40 + m.seed % 20;
                    m.grounded = false;
                }
            }
        } else {
            const float target = moving ? static_cast<float>(dir) * speed * (chasing ? 1.35f : 1.0f) : 0.0f;
            vx += (target - vx) * 0.25f;
        }
        const float reach = s.behavior == Behavior::Charger ? 340.0f : s.behavior == Behavior::Hopper ? 170.0f : 96.0f;
        if (chasing && m.attackCooldown == 0 && m.grounded && std::abs(dx) < reach && std::abs(dy) < 80.0f) {
            enter(m, MonsterState::Windup, m.elite ? 24 : 32);
            m.facing = sign(dx);
            vx = 0.0f;
        }
        break;
    }
    case MonsterState::Windup:
        vx *= 0.7f;
        if (m.stateTicks >= m.stateLength) {
            step.lunged = true;
            if (s.behavior == Behavior::Hopper) {
                enter(m, MonsterState::Attack, 40);
                vx = static_cast<float>(m.facing) * std::min(std::abs(dx) / 22.0f + 2.0f, 7.0f);
                m.velocity.y = -9.0f;
                m.grounded = false;
            } else if (s.behavior == Behavior::Charger) {
                enter(m, MonsterState::Attack, 46);
                vx = static_cast<float>(m.facing) * (m.elite ? 9.5f : 8.5f);
            } else {
                enter(m, MonsterState::Attack, 18);
                vx = static_cast<float>(m.facing) * 7.0f;
            }
        }
        break;
    case MonsterState::Attack: {
        step.strike = m.box.expanded(6.0f);
        const bool edge = (m.facing < 0 && m.box.left() <= m.patrolLeft) || (m.facing > 0 && m.box.right() >= m.patrolRight)
                          || (m.grounded && gamelib::core::ledgeAhead(m.box, m.facing, world, 4.0f));
        if (s.behavior == Behavior::Charger) {
            if (edge) {
                vx = 0.0f;
            }
            vx *= m.stateTicks > 30 ? 0.85f : 1.0f;
        } else if (s.behavior == Behavior::Walker) {
            vx *= 0.88f;
            if (edge) {
                vx = 0.0f;
            }
        }
        const bool done = m.stateTicks >= m.stateLength || (s.behavior == Behavior::Hopper && m.grounded && m.stateTicks > 6);
        if (done) {
            enter(m, MonsterState::Move, 40);
            m.attackCooldown = 100 + m.seed % 50;
            vx = 0.0f;
        }
        break;
    }
    case MonsterState::Hit: vx *= 0.82f; break;
    default: break;
    }
    m.velocity.x = vx;
    m.velocity.y = std::min(m.velocity.y + kGravity, kMaxFall);
    const bool wasGrounded = m.grounded;
    const gamelib::core::BoxMove move = gamelib::core::moveBox(m.box, m.velocity, world);
    m.box = move.box;
    m.grounded = move.contacts.ground;
    if (move.contacts.blockedX) {
        m.velocity.x = 0.0f;
    }
    if (move.contacts.blockedY) {
        m.velocity.y = 0.0f;
    }
    step.landed = m.grounded && !wasGrounded;
    // A monster that fell off its run (knocked off a ledge) adopts the floor it lands on.
    if (step.landed && (m.box.right() < m.patrolLeft || m.box.left() > m.patrolRight)) {
        m.patrolLeft = m.box.left() - 140.0f;
        m.patrolRight = m.box.right() + 140.0f;
    }
    step.contact = true;
    m.vulnerable.setHurtbox(m.box);
    return step;
}

} // namespace wayfarer::game
