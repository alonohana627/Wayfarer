#pragma once

// The demo bot: fights its way across the first forest map, as the title
// screen's attract mode and the --demo run. It reads the session like a
// player reads the screen (the monsters ahead, its health, a wall in the
// way) and answers with one tick of input; deterministic for a seed.

#include "session.hpp"

#include <gamelib/ai/planner.hpp>

namespace wayfarer::game {

struct DemoBot {
    int ticks = 0;
    int jumpHold = 0;
    int hyperIn = -1;     // ticks until a planned hyper jump
    int lastSkill = -999;
    gamelib::ai::StuckWatch stuck{45};
    float bestX = 0.0f;
};

// A session placed on the Bellwood Path the way the demo starts.
[[nodiscard]] Session demoSession(std::uint64_t seed);

[[nodiscard]] Input demoInput(const Session& session, DemoBot& bot);

// True once the bot reached the end of the map (or fell).
[[nodiscard]] bool demoDone(const Session& session);

} // namespace wayfarer::game
