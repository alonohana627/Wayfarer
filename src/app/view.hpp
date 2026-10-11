#pragma once

// Drawing the world: the camera (GameLib's Camera2D following the hero,
// held inside the map), the region's parallax sky and silhouettes, the
// terrain, footholds, ropes and ladders, houses and decorations, portals
// and waystones, the townsfolk with quest markers and name tags, monsters
// with their levels and health bars, the guardian, loot, shots and the
// hero - every character an anime avatar or creature shader - then lights
// and the color grade of the region and the hour. Presentation only.

#include "effects.hpp"
#include "shapes.hpp"

#include "session.hpp"

#include <gamelib/core/camera.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/fx/sprite_motion.hpp>

#include <vector>

namespace wayfarer::app {

class WorldView {
public:
    explicit WorldView(gamelib::core::Size canvas);

    void snap(const game::Session& session);
    // One frame: camera, character animation state (call before draw).
    void follow(const game::Session& session, float alpha, float seconds, float time);
    void draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Effects& effects,
              float alpha, float time, gamelib::core::Vec2 shake) const;

    [[nodiscard]] gamelib::core::View2D view(gamelib::core::Vec2 shake = {}) const { return m_camera.view(shake); }
    [[nodiscard]] gamelib::core::Rect visible() const;
    [[nodiscard]] gamelib::core::Vec2 heroFeet(const game::Session& session, float alpha) const;

    // The hero as drawn (also for the character window and the title).
    [[nodiscard]] static anime::Avatar heroLook(const game::Session& session, float alpha, float time);
    void drawHero(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float alpha,
                  float time) const;

private:
    struct Run {
        gamelib::core::Rect rect;
        float worldLeft = 0.0f;
        int kind = 1; // 1 foothold, 2 rope, 3 ladder
    };
    void cacheMap(const game::MapDef& def) const;
    void drawBackdrop(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawTerrain(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session) const;
    void drawDecos(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawPortals(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawNpcs(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawMonsters(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float alpha,
                      float time) const;
    void drawBoss(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float alpha,
                  float time) const;
    void drawLoot(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float alpha,
                  float time) const;
    void drawShots(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawLights(gamelib::draw::DrawList& list, const game::Session& session, float alpha, float time) const;
    void drawLabels(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float alpha,
                    float time) const;

    gamelib::core::Size m_canvas;
    gamelib::core::Camera2D m_camera;
    game::MapId m_map = game::MapId::Count;
    mutable game::MapId m_cached = game::MapId::Count;
    mutable std::vector<Run> m_runs;
    gamelib::fx::SquashStretch m_squash;
    int m_lastLandTicks = 0;
    float m_expressionBlend = 1.0f;
    int m_expressionFrom = 0;
    int m_expressionTo = 0;
};

} // namespace wayfarer::app
