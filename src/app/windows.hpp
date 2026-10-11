#pragma once

// The MapleStory-like windows, opened over a paused world: the character's
// stat window (core stats with ability points, a preview of every change a
// point makes, auto-assign), the skill window (icons, levels, what the next
// point does), the bag and equipment (a 24-slot grid, five gear slots on a
// portrait, item cards with comparisons, quick-slot potions), the quest
// log, Pippa's shop (buy and sell) and Brann's forge (upgrades for coins and
// glimmer ore). Keyboard and gamepad through ui::MenuInput.

#include "shapes.hpp"

#include "session.hpp"

#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/fx/transients.hpp>
#include <gamelib/platform/actions.hpp>
#include <gamelib/ui/menu.hpp>

#include <string>

namespace wayfarer::app {

enum class WindowId { None, Stats, Skills, Inventory, Quests, Shop, Smith };

// What a key press did in a window (for sounds).
enum class WindowFeedback { None, Moved, Done, Failed, Closed, Tab, Big };

// Extra keys the windows understand besides the menu directions.
enum class WindowKey { None, QuickHp, QuickMp, Auto, Tab };

class Windows {
public:
    void open(WindowId id);
    void close() { m_open = WindowId::None; }
    [[nodiscard]] WindowId current() const { return m_open; }
    [[nodiscard]] bool isOpen() const { return m_open != WindowId::None; }
    WindowFeedback handle(gamelib::ui::MenuInput input, game::Session& session);
    WindowFeedback handleKey(WindowKey key, game::Session& session);
    void update(float seconds) { m_age += seconds; m_flash.update(seconds); }
    void draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
              const gamelib::platform::ActionMap& actions, gamelib::core::Size canvas, float time) const;

private:
    void drawStats(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawSkills(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                    const gamelib::platform::ActionMap& actions, float time) const;
    void drawInventory(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawQuests(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawShop(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawSmith(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    // The forge's list: equipped slots first (>= 0), then bag slots (-1 - index).
    [[nodiscard]] std::vector<int> smithTargets(const game::Session& session) const;

    struct Flash {
        int row = 0;
    };
    WindowId m_open = WindowId::None;
    int m_cursor = 0;
    int m_tab = 0;      // shop: 0 buy, 1 sell
    int m_scroll = 0;   // first visible row of long lists
    float m_age = 0.0f;
    gamelib::fx::Transients<Flash> m_flash;
};

} // namespace wayfarer::app
