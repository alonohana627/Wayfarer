#include "bosses.hpp"

#include <gamelib/ai/opponent.hpp>
#include <gamelib/core/state_machine.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace wayfarer::game {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;

constexpr float kGravity = 0.62f;
constexpr int kIntroTicks = 100;
constexpr int kDefeatTicks = 150;

const std::array<BossInfo, kBossCount>& infos()
{
    using A = BossAction;
    static const std::array<BossInfo, kBossCount> t{{
        {"Grandcap", "Elder of Bellwood", 7, false, {150, 168}, 1.5f, {A::Stomp, A::Rain, A::Summon}, SpeciesId::Bellcap,
         ShotLook::Spore},
        {"Rumblerock", "Heart of the Glimmerdeep", 13, false, {176, 150}, 1.4f, {A::Throw, A::Charge, A::Rain},
         SpeciesId::Pebblet, ShotLook::Rock},
        {"Queen Murkshell", "Mother of the Mire", 19, false, {190, 156}, 1.1f, {A::Spin, A::Volley, A::Summon},
         SpeciesId::BogMochi, ShotLook::Bubble},
        {"The Lantern Warden", "Keeper of Sunspire", 25, true, {150, 176}, 2.0f, {A::Fan, A::Teleport, A::Dive, A::Rain},
         SpeciesId::Wisp, ShotLook::Flame},
        {"The Hush", "Silence upon the Peak", 31, true, {180, 210}, 2.2f,
         {A::Fan, A::Dive, A::Rain, A::Summon, A::Teleport}, SpeciesId::FrostWisp, ShotLook::Ice},
    }};
    return t;
}

int sign(const float v)
{
    return v < 0.0f ? -1 : 1;
}

void enter(Boss& b, const BossState state, const int length)
{
    b.state = state;
    b.stateTicks = 0;
    b.stateLength = std::max(length, 1);
}

// Shorter windups as the boss gets angry.
int paced(const Boss& b, const int ticks)
{
    return b.phase == 0 ? ticks : b.phase == 1 ? ticks * 4 / 5 : ticks * 2 / 3;
}

int windupOf(const BossAction a)
{
    switch (a) {
    case BossAction::Stomp: return 40;
    case BossAction::Rain: return 36;
    case BossAction::Summon: return 34;
    case BossAction::Throw: return 34;
    case BossAction::Charge: return 46;
    case BossAction::Spin: return 36;
    case BossAction::Volley: return 32;
    case BossAction::Fan: return 34;
    case BossAction::Teleport: return 24;
    case BossAction::Dive: return 36;
    case BossAction::None: break;
    }
    return 30;
}

void moveGround(Boss& b, const gamelib::core::TileWorld& world, BossStep& step)
{
    b.velocity.y = std::min(b.velocity.y + kGravity, 16.0f);
    const bool was = b.grounded;
    const gamelib::core::BoxMove move = gamelib::core::moveBox(b.box, b.velocity, world);
    b.box = move.box;
    b.grounded = move.contacts.ground;
    if (move.contacts.blockedY) {
        b.velocity.y = 0.0f;
    }
    if (move.contacts.blockedX) {
        b.velocity.x = 0.0f;
    }
    // Stay inside the arena.
    if (b.box.left() < b.bounds.left()) {
        b.box.x = b.bounds.left();
        b.velocity.x = std::abs(b.velocity.x);
    }
    if (b.box.right() > b.bounds.right()) {
        b.box.x = b.bounds.right() - b.box.width;
        b.velocity.x = -std::abs(b.velocity.x);
    }
    step.landed = b.grounded && !was;
}

float hoverY(const Boss& b)
{
    return b.bounds.bottom() - 300.0f - b.box.height * 0.5f;
}

void startAction(Boss& b, gamelib::core::Rng& rng, BossStep& step)
{
    const std::vector<BossAction>& actions = b.info().actions;
    const int last = static_cast<int>(std::ranges::find(actions, b.lastAction) - actions.begin());
    BossAction next = actions[static_cast<std::size_t>(gamelib::core::nextPattern(rng, static_cast<int>(actions.size()),
                                                                                  last < static_cast<int>(actions.size()) ? last : -1))];
    b.action = next;
    b.lastAction = next;
    b.repeats = b.phase >= 2 && (next == BossAction::Stomp || next == BossAction::Dive || next == BossAction::Fan) ? 1 : 0;
    enter(b, BossState::Windup, paced(b, windupOf(next)));
    step.roared = true;
}

} // namespace

const BossInfo& bossInfo(const BossId id)
{
    return infos()[static_cast<std::size_t>(id)];
}

int bossHp(const BossId id)
{
    const int level = bossInfo(id).level;
    return monsterHp(level, false) * (id == BossId::Hush ? 16 : 12);
}

int bossAttack(const BossId id)
{
    return monsterAttack(bossInfo(id).level, false) * 13 / 10;
}

int bossXp(const BossId id)
{
    return monsterXp(bossInfo(id).level, false) * 25;
}

Boss makeBoss(const BossId id, const Vec2 feet, const Rect bounds)
{
    Boss b;
    b.id = id;
    const BossInfo& info = bossInfo(id);
    b.box = Rect::fromBottomCenter(info.flying ? Vec2{feet.x, feet.y - 160.0f} : Vec2{feet.x, feet.y - 420.0f}, info.size);
    b.previous = b.box;
    b.bounds = bounds;
    b.hp = bossHp(id);
    b.maxHp = b.hp;
    b.vulnerable = gamelib::core::Vulnerable{b.box, 2};
    enter(b, BossState::Intro, kIntroTicks);
    return b;
}

bool hurtBoss(Boss& b, const int damage, const int tick)
{
    if (!b.alive() || b.state == BossState::Intro) {
        return false;
    }
    b.hp = std::max(0, b.hp - damage);
    b.lastHitTick = tick;
    if (b.hp == 0) {
        enter(b, BossState::Defeat, kDefeatTicks);
        b.velocity = {};
        b.guarded = false;
        b.vulnerable.setGuarded(false);
        return true;
    }
    return false;
}

BossStep stepBoss(Boss& b, const Rect hero, const gamelib::core::TileWorld& world, gamelib::core::Rng& rng, const int tick)
{
    BossStep step;
    b.previous = b.box;
    b.vulnerable.tick();
    ++b.stateTicks;
    const BossInfo& info = b.info();
    const Vec2 c = b.box.center();
    const Vec2 h = hero.center();
    const float dx = h.x - c.x;

    const int phase = b.hp * 100 <= b.maxHp * 30 ? 2 : b.hp * 100 <= b.maxHp * 60 ? 1 : 0;
    if (phase != b.phase && b.alive()) {
        b.phase = phase;
        step.phaseChanged = true;
    }

    if (b.state == BossState::Defeat) {
        if (info.flying) {
            b.box.y += 0.6f;
        } else {
            moveGround(b, world, step);
        }
        step.finished = b.stateTicks >= b.stateLength;
        b.vulnerable.setHurtbox(b.box);
        return step;
    }
    if (b.state == BossState::Intro) {
        if (info.flying) {
            b.fade = std::min(1.0f, static_cast<float>(b.stateTicks) / 60.0f);
            b.box.y += (hoverY(b) - b.box.center().y) * 0.05f;
        } else {
            moveGround(b, world, step);
        }
        if (b.stateTicks >= b.stateLength) {
            enter(b, BossState::Idle, 70);
        }
        b.vulnerable.setHurtbox(b.box);
        return step;
    }

    step.contact = b.fade > 0.5f;
    const float speed = info.speed * (b.phase == 2 ? 1.35f : b.phase == 1 ? 1.15f : 1.0f);
    switch (b.state) {
    case BossState::Idle: {
        b.facing = sign(dx);
        if (info.flying) {
            const Vec2 target{std::clamp(h.x - b.facing * 220.0f, b.bounds.left() + 100.0f, b.bounds.right() - 100.0f),
                              hoverY(b) + std::sin(static_cast<float>(tick) * 0.04f) * 30.0f};
            b.velocity = b.velocity + (Vec2{(target.x - c.x) * 0.02f, (target.y - c.y) * 0.04f} - b.velocity) * 0.1f;
            b.velocity.x = std::clamp(b.velocity.x, -speed, speed);
        } else {
            b.velocity.x = std::abs(dx) > 140.0f ? static_cast<float>(b.facing) * speed : 0.0f;
        }
        if (b.stateTicks >= b.stateLength) {
            startAction(b, rng, step);
        }
        break;
    }
    case BossState::Windup:
        b.velocity.x *= 0.8f;
        if (info.flying) {
            b.velocity.y *= 0.8f;
        }
        b.facing = sign(dx);
        if (b.action == BossAction::Teleport) {
            b.fade = 1.0f - b.progress();
        }
        if (b.stateTicks >= b.stateLength) {
            enter(b, BossState::Act, 60);
            switch (b.action) {
            case BossAction::Stomp: {
                const Vec2 feet = b.box.bottomCenter();
                const Vec2 to{std::clamp(h.x, b.bounds.left() + 80.0f, b.bounds.right() - 80.0f), feet.y};
                b.velocity = gamelib::ai::leapVelocity(feet, to, 15.0f, kGravity, 9.0f);
                b.grounded = false;
                b.stateLength = 200;
                break;
            }
            case BossAction::Rain: {
                const int count = 5 + b.phase * 2;
                for (int i = 0; i < count; ++i) {
                    const float x = std::clamp(h.x + (static_cast<float>(i) - static_cast<float>(count - 1) * 0.5f) * 150.0f
                                                   + static_cast<float>(rng.range(-40, 40)),
                                               b.bounds.left() + 40.0f, b.bounds.right() - 40.0f);
                    step.rain.push_back(x);
                }
                b.stateLength = 30;
                break;
            }
            case BossAction::Summon:
                step.summon = 2 + (b.phase >= 1 ? 1 : 0);
                b.stateLength = 20;
                break;
            case BossAction::Throw: {
                const int count = 3 + b.phase;
                for (int i = 0; i < count; ++i) {
                    const Vec2 from = c + Vec2{static_cast<float>(b.facing) * 40.0f, -60.0f};
                    const Vec2 to{h.x + (static_cast<float>(i) - static_cast<float>(count - 1) * 0.5f) * 110.0f, hero.bottom() - 10.0f};
                    step.shots.push_back({.at = from, .velocity = gamelib::ai::lobVelocity(from, to, 0.4f, 50 + i * 6, {12.0f, 16.0f}),
                                          .gravity = 0.4f, .size = {40.0f, 40.0f}, .lifetime = 200, .look = info.shot});
                }
                b.stateLength = 24;
                break;
            }
            case BossAction::Charge:
                b.velocity.x = static_cast<float>(b.facing) * (9.0f + static_cast<float>(b.phase) * 1.5f);
                b.stateLength = 120;
                break;
            case BossAction::Spin:
                b.guarded = true;
                b.vulnerable.setGuarded(true);
                b.velocity.x = static_cast<float>(b.facing) * (8.0f + static_cast<float>(b.phase) * 1.5f);
                b.repeats = 2;
                b.stateLength = 240;
                break;
            case BossAction::Volley: {
                const int count = 5 + b.phase * 2;
                for (int i = 0; i < count; ++i) {
                    const Vec2 from = c + Vec2{0.0f, -50.0f};
                    const Vec2 to{h.x + (static_cast<float>(i) - static_cast<float>(count - 1) * 0.5f) * 90.0f, hero.bottom()};
                    step.shots.push_back({.at = from, .velocity = gamelib::ai::lobVelocity(from, to, 0.3f, 60 + i * 4, {10.0f, 14.0f}),
                                          .gravity = 0.3f, .size = {34.0f, 34.0f}, .lifetime = 240, .look = info.shot});
                }
                b.stateLength = 30;
                break;
            }
            case BossAction::Fan: {
                const int count = 5 + b.phase * 2;
                const Vec2 from = c;
                for (int i = 0; i < count; ++i) {
                    const Vec2 dir = gamelib::core::fanDirection(h - from, i, count, 0.22f);
                    step.shots.push_back({.at = from, .velocity = dir * (4.6f + static_cast<float>(b.phase) * 0.6f), .size = {30.0f, 30.0f},
                                          .lifetime = 260, .look = info.shot});
                }
                b.stateLength = 34;
                break;
            }
            case BossAction::Teleport: {
                const float side = rng.chance(0.5f) ? -1.0f : 1.0f;
                const float x = std::clamp(h.x + side * 300.0f, b.bounds.left() + 100.0f, b.bounds.right() - 100.0f);
                b.box.x = x - b.box.width * 0.5f;
                b.box.y = hoverY(b) - b.box.height * 0.5f;
                b.previous = b.box;
                b.velocity = {};
                b.stateLength = 24;
                break;
            }
            case BossAction::Dive: {
                b.diveTarget = h;
                const Vec2 d = h - c;
                const float len = std::max(std::sqrt(d.x * d.x + d.y * d.y), 1.0f);
                b.velocity = d * ((10.0f + static_cast<float>(b.phase)) / len);
                b.stateLength = 50;
                break;
            }
            case BossAction::None: break;
            }
        }
        break;
    case BossState::Act:
        switch (b.action) {
        case BossAction::Stomp:
            if (b.grounded && b.stateTicks > 4) {
                const Vec2 feet = b.box.bottomCenter();
                for (const float dir : {-1.0f, 1.0f}) {
                    step.shots.push_back({.at = feet + Vec2{dir * 60.0f, -22.0f}, .velocity = {dir * (6.0f + b.phase), 0.0f},
                                          .size = {44.0f, 40.0f}, .lifetime = 90, .look = ShotLook::Wave, .groundWave = true});
                }
                b.velocity.x = 0.0f;
                if (b.repeats > 0) {
                    --b.repeats;
                    enter(b, BossState::Windup, paced(b, 26));
                } else {
                    enter(b, BossState::Recover, paced(b, 50));
                }
            }
            break;
        case BossAction::Charge:
            if ((b.velocity.x < 0.0f && b.box.left() <= b.bounds.left() + 2.0f)
                || (b.velocity.x > 0.0f && b.box.right() >= b.bounds.right() - 2.0f) || b.stateTicks >= b.stateLength) {
                b.velocity.x = 0.0f;
                step.landed = true;
                enter(b, BossState::Recover, paced(b, 70));
            }
            break;
        case BossAction::Spin:
            if ((b.velocity.x < 0.0f && b.box.left() <= b.bounds.left() + 2.0f)
                || (b.velocity.x > 0.0f && b.box.right() >= b.bounds.right() - 2.0f)) {
                b.velocity.x = -b.velocity.x;
                step.landed = true;
                if (--b.repeats < 0) {
                    b.velocity.x = 0.0f;
                }
            }
            if (b.repeats < 0 || b.stateTicks >= b.stateLength) {
                b.velocity.x = 0.0f;
                b.guarded = false;
                b.vulnerable.setGuarded(false);
                enter(b, BossState::Recover, paced(b, 70));
            }
            break;
        case BossAction::Dive:
            b.velocity = b.velocity * 0.985f;
            if (b.box.bottom() > b.bounds.bottom() - 4.0f) {
                b.box.y = b.bounds.bottom() - 4.0f - b.box.height;
                b.velocity.y = 0.0f;
            }
            if (b.stateTicks >= b.stateLength) {
                if (b.repeats > 0) {
                    --b.repeats;
                    enter(b, BossState::Windup, paced(b, 24));
                } else {
                    enter(b, BossState::Recover, paced(b, 50));
                }
            }
            break;
        case BossAction::Teleport:
            b.fade = b.progress();
            if (b.stateTicks >= b.stateLength) {
                b.fade = 1.0f;
                enter(b, BossState::Recover, paced(b, 20));
            }
            break;
        default:
            if (b.stateTicks >= b.stateLength) {
                if (b.action == BossAction::Fan && b.repeats > 0) {
                    --b.repeats;
                    enter(b, BossState::Windup, paced(b, 22));
                } else {
                    enter(b, BossState::Recover, paced(b, 40));
                }
            }
            break;
        }
        break;
    case BossState::Recover:
        b.velocity.x *= 0.85f;
        if (info.flying) {
            b.velocity.y += (hoverY(b) - c.y) * 0.004f;
            b.velocity.y *= 0.9f;
        }
        if (b.stateTicks >= b.stateLength) {
            b.action = BossAction::None;
            enter(b, BossState::Idle, paced(b, 50 + rng.range(0, 30)));
        }
        break;
    default: break;
    }

    if (info.flying) {
        b.box.x += b.velocity.x;
        b.box.y += b.velocity.y;
        b.box.x = std::clamp(b.box.x, b.bounds.left(), b.bounds.right() - b.box.width);
        b.box.y = std::clamp(b.box.y, b.bounds.top() + 40.0f, b.bounds.bottom() - b.box.height);
    } else {
        moveGround(b, world, step);
    }
    b.vulnerable.setHurtbox(b.box);
    return step;
}

} // namespace wayfarer::game
