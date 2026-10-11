#include "shapes.hpp"

#include "avatar_frag.spv.hpp"
#include "backdrop_frag.spv.hpp"
#include "boss_frag.spv.hpp"
#include "building_frag.spv.hpp"
#include "deco_frag.spv.hpp"
#include "fx_frag.spv.hpp"
#include "monster_frag.spv.hpp"
#include "terrain_frag.spv.hpp"
#include "ui_frag.spv.hpp"

namespace wayfarer::app {

Shapes Shapes::registerWith(gamelib::gfx::Renderer& renderer)
{
    return {
        .avatar = renderer.addShape(shaders::avatar_frag),
        .monster = renderer.addShape(shaders::monster_frag),
        .boss = renderer.addShape(shaders::boss_frag),
        .building = renderer.addShape(shaders::building_frag),
        .deco = renderer.addShape(shaders::deco_frag),
        .terrain = renderer.addShape(shaders::terrain_frag),
        .backdrop = renderer.addShape(shaders::backdrop_frag),
        .fx = renderer.addShape(shaders::fx_frag),
        .ui = renderer.addShape(shaders::ui_frag),
    };
}

void addFx(gamelib::draw::DrawList& list, const Shapes& shapes, const Fx kind, const gamelib::core::Vec2 at, const float size,
           const gamelib::core::Color core, const gamelib::core::Color edge, const float progress, const float variant,
           const float w, const float rotation)
{
    list.add({.rect = {at.x - size * 0.5f, at.y - size * 0.5f, size, size},
              .color = core,
              .color2 = edge,
              .shape = shapes.fx,
              .rotation = rotation,
              .params = {static_cast<float>(kind), progress, variant, w}});
}

void addUi(gamelib::draw::DrawList& list, const Shapes& shapes, const Ui kind, const gamelib::core::Rect rect,
           const gamelib::core::Color a, const gamelib::core::Color b, const float y, const float z, const float w)
{
    list.add({.rect = rect, .color = a, .color2 = b, .shape = shapes.ui, .params = {static_cast<float>(kind), y, z, w}});
}

void addIcon(gamelib::draw::DrawList& list, const Shapes& shapes, const Ui kind, const gamelib::core::Vec2 at, const float size,
             const float id, const float style, const gamelib::core::Color a, const gamelib::core::Color b, const float w)
{
    list.add({.rect = {at.x - size * 0.5f, at.y - size * 0.5f, size, size},
              .color = a,
              .color2 = b,
              .shape = shapes.ui,
              .params = {static_cast<float>(kind), id, style, w}});
}

void addDeco(gamelib::draw::DrawList& list, const Shapes& shapes, const int kind, const gamelib::core::Vec2 ground,
             const float width, const float height, const gamelib::core::Color a, const gamelib::core::Color b, const float lit,
             const float variant, const float seed)
{
    // The deco shader's ground line is 6/128 above the quad's bottom.
    const float bottom = ground.y + height * (6.0f / 128.0f);
    list.add({.rect = {ground.x - width * 0.5f, bottom - height, width, height},
              .color = a,
              .color2 = b,
              .shape = shapes.deco,
              .params = {static_cast<float>(kind), lit, variant, seed}});
}

} // namespace wayfarer::app
