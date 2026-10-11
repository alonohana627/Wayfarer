#pragma once

// The play screen's HUD in the storybook style: the level badge with HP and
// MP gauges (a "damage taken" trail), the experience bar along the bottom,
// the hotkeys with their skill icons, cooldowns and potion counts, the
// minimap with the hero, portals and people, the clock and coins, active
// buffs, a tracker for open quests, map names and toasts, the guardian's
// health bar, and the "up" prompt near people and doors.

#include "look.hpp"
#include "shapes.hpp"

#include "session.hpp"

#include <gamelib/core/camera.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/fx/banners.hpp>
#include <gamelib/fx/transients.hpp>
#include <gamelib/platform/actions.hpp>
#include <gamelib/ui/hud.hpp>

#include <string>
#include <vector>

namespace wayfarer::app {

class Hud {
public:
    void reset(const game::Session& session);
    void react(const game::Event& e, const game::Session& session);
    void update(float seconds, const game::Session& session);
    void draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
              const gamelib::platform::ActionMap& actions, const gamelib::core::View2D& view, gamelib::core::Size canvas,
              bool bigMinimap, float time) const;
    // A toast in the corner (quest updates, "bag full").
    void toast(std::string text, Color color);
    void mapTitle(const game::Session& session);

private:
    void drawBars(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, gamelib::core::Size canvas) const;
    void drawHotkeys(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                     const gamelib::platform::ActionMap& actions, gamelib::core::Size canvas, float time) const;
    void drawMinimap(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, bool big, float time) const;
    void drawClock(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, gamelib::core::Size canvas) const;
    void drawTracker(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, gamelib::core::Size canvas) const;
    void drawBoss(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, gamelib::core::Size canvas,
                  float time) const;
    void drawPrompt(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                    const gamelib::platform::ActionMap& actions, const gamelib::core::View2D& view, float time) const;

    struct Toast {
        std::string text;
        Color color;
    };
    gamelib::ui::BarTrail m_hp{0.5f, 0.6f};
    gamelib::ui::BarTrail m_mp{0.5f, 0.6f};
    gamelib::ui::BarTrail m_boss{0.6f, 0.4f};
    gamelib::fx::Transients<Toast> m_toasts;
    gamelib::fx::Banners m_banner; // the map title
    float m_xpShown = 0.0f;
    float m_coinsShown = 0.0f;
    struct Line {
        gamelib::core::Rect rect;
        int kind = 0; // 0 floor, 1 foothold, 2 climb
    };
    mutable game::MapId m_cached = game::MapId::Count;
    mutable std::vector<Line> m_lines;
};

} // namespace wayfarer::app
