#include "demo.hpp"

#include <cmath>

namespace wayfarer::game {

Session demoSession(const std::uint64_t seed)
{
    Session s{seed};
    s.debugGrantChapter(1);
    s.debugLevel(5);
    // Learn the early skills the way a player would.
    for (int i = 0; i < 3; ++i) {
        (void)s.learnSkill(SkillId::Sweep);
        (void)s.learnSkill(SkillId::Roll);
    }
    s.debugTeleport(MapId::Forest1, 0);
    (void)s.events().take();
    return s;
}

bool demoDone(const Session& s)
{
    return s.mapId() != MapId::Forest1 || s.phase() != Phase::Playing || s.travelPending();
}

Input demoInput(const Session& s, DemoBot& bot)
{
    Input in;
    ++bot.ticks;
    const Hero& h = s.hero();
    const gamelib::core::Rect box = h.body.box;
    const float x = box.center().x;
    const float feetY = box.bottom();

    // The nearest monster at the hero's height, ahead or close behind.
    const Monster* target = nullptr;
    float best = 1e9f;
    int around = 0;
    for (const Monster& m : s.monsters()) {
        if (!m.alive() || m.state == MonsterState::Spawn) {
            continue;
        }
        const float dx = m.box.center().x - x;
        const float dy = m.box.bottom() - feetY;
        if (std::abs(dy) > 70.0f || dx < -150.0f || dx > 420.0f) {
            continue;
        }
        around += std::abs(dx) < 200.0f ? 1 : 0;
        if (std::abs(dx) < best) {
            best = std::abs(dx);
            target = &m;
        }
    }

    in.move = 1;
    if (target != nullptr) {
        const float dx = target->box.center().x - x;
        const int dir = dx < 0.0f ? -1 : 1;
        if (std::abs(dx) < 105.0f) {
            in.move = (h.body.facing != dir) ? dir : 0;
            in.attackPressed = bot.ticks % 6 == 0;
            in.attackHeld = true;
            if (around >= 2 && bot.ticks - bot.lastSkill > 150) {
                in.skillPressed[0] = true; // Crescent Sweep
                bot.lastSkill = bot.ticks;
            }
        } else {
            in.move = dir;
        }
    } else if (h.body.grounded && bot.hyperIn < 0 && bot.ticks % 300 == 150) {
        // Show off the hyper jump on open ground.
        in.jumpPressed = true;
        in.jumpHeld = true;
        bot.jumpHold = 10;
        bot.hyperIn = 14;
    }

    // Stuck against a step or a wall: jump over it.
    if (bot.stuck.update(x) && h.body.grounded) {
        bot.jumpHold = 16;
        in.jumpPressed = true;
    }
    if (bot.jumpHold > 0) {
        --bot.jumpHold;
        in.jumpHeld = true;
    }
    if (bot.hyperIn >= 0 && --bot.hyperIn == 0) {
        in.jumpPressed = true;
        in.jumpHeld = true;
        bot.hyperIn = -1;
    }
    if (s.character().hp.current * 100 < s.character().hp.max * 40 && bot.ticks % 30 == 0) {
        in.hpPotion = true;
    }
    return in;
}

} // namespace wayfarer::game
