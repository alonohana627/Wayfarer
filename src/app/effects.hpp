#pragma once

// The game's juice, driven by the rules' events and drawn over the world:
// sword arcs and the crescent sweep that follow the hero, hit sparks and
// stacked damage numbers in chunky outlined digits (criticals bigger and
// golden, the hero's hurts red, heals green), the hyper jump's afterimages,
// petals, rings and shockwaves, the level-up pillar, loot and coin popups,
// the rain warnings, dust, ambient motes per region, screen shake and
// flashes. Presentation only, on real frame time.

#include "shapes.hpp"

#include "session.hpp"

#include <gamelib/core/color.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/fx/ambient.hpp>
#include <gamelib/fx/damage_numbers.hpp>
#include <gamelib/fx/juice.hpp>
#include <gamelib/fx/particles.hpp>
#include <gamelib/fx/transients.hpp>

#include <string>

namespace wayfarer::app {

class Effects {
public:
    Effects();

    // A rules event (positions in world units).
    void react(const game::Event& e, const game::Session& session);
    void update(float seconds, const game::Session& session, gamelib::core::Rect visible, gamelib::core::Vec2 heroFeet);
    void clear();

    // Under the world view: behind the characters, then in front of them.
    void drawBehind(gamelib::draw::DrawList& list, const Shapes& shapes, gamelib::core::Vec2 heroFeet, float time) const;
    void drawFront(gamelib::draw::DrawList& list, const Shapes& shapes, gamelib::core::Vec2 heroFeet, int heroFacing,
                   float time) const;
    // In canvas units: flashes.
    void drawScreen(gamelib::draw::DrawList& list, gamelib::core::Size canvas) const;

    [[nodiscard]] gamelib::core::Vec2 shake(float time) const { return m_shake.offset(time); }
    void setRegion(game::Region region, bool dungeon);
    void setShakeScale(float scale) { m_shakeScale = scale; }

    // The hyper jump's afterimages (feet, facing, age 0..1).
    struct Ghost {
        gamelib::core::Vec2 feet;
        int facing = 1;
        float pose = 2.0f;
    };
    [[nodiscard]] const gamelib::fx::Transients<Ghost>& ghosts() const { return m_ghosts; }

private:
    struct Arc {
        gamelib::core::Vec2 offset; // from the hero's feet (attached) or the world point
        int facing = 1;
        int variant = 0;
        bool attached = true;
        gamelib::core::Color core;
        gamelib::core::Color edge;
        float size = 220.0f;
        Fx kind = Fx::Slash;
    };
    struct Spot {
        gamelib::core::Vec2 at;
        Fx kind = Fx::Spark;
        float size = 140.0f;
        gamelib::core::Color core;
        gamelib::core::Color edge;
        float variant = 0.0f;
        float seed = 0.0f;
        bool attached = false;
    };
    struct Popup {
        gamelib::core::Vec2 at;
        std::string text;
        gamelib::core::Color color;
        int icon = 0; // an item id (0: none)
        int rarity = 0;
    };

    void addArc(const Arc& arc, float seconds) { m_arcs.add(arc, seconds); }
    void spot(const Spot& spot, float seconds) { m_spots.add(spot, seconds); }

    gamelib::fx::DamageNumbers m_numbers;
    gamelib::fx::Transients<Arc> m_arcs;
    gamelib::fx::Transients<Spot> m_spots;
    gamelib::fx::Transients<Popup> m_popups;
    gamelib::fx::Transients<Ghost> m_ghosts;
    gamelib::fx::Particles m_sparks;
    gamelib::fx::Particles m_petals;
    gamelib::fx::Ambient m_motes;
    gamelib::fx::AmbientLook m_moteLook;
    gamelib::fx::Shake m_shake{0.4f, 16.0f};
    gamelib::fx::Fade m_flash{0.35f};
    gamelib::core::Color m_flashColor{1.0f, 1.0f, 1.0f, 0.5f};
    float m_shakeScale = 1.0f;
    float m_ghostClock = 0.0f;
    int m_popupRow = 0;
    float m_popupClock = 0.0f;
};

} // namespace wayfarer::app
