#include "dialogue_view.hpp"

#include "look.hpp"
#include "ui_kit.hpp"

#include <gamelib/core/easing.hpp>

#include <algorithm>
#include <cmath>

namespace wayfarer::app {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::draw::Font;

constexpr Rect kBox{330.0f, 780.0f, 1300.0f, 250.0f};

gamelib::ui::DialogueBoxStyle boxStyle(const Shapes& shapes, const std::shared_ptr<float>& time)
{
    const gamelib::draw::VectorTextShadow outline{.color = srgb(0x2A1E30, 0.9f), .offset = {0.0f, 0.0f}, .weight = 4.3f};
    return {
        .panel = {.painter = [&shapes](gamelib::draw::DrawList& list, const Rect& rect) {
            addUi(list, shapes, Ui::Window, rect.expanded(10.0f), ui_color::kWindow, ui_color::kTitle, 0.0f, 0.0f);
        }},
        .font = Font::vector(34.0f, 1.3f),
        .speakerFont = Font::vector(30.0f, 1.7f).withShadow(outline),
        .padding = 32.0f,
        .portraitSize = 304.0f, // the text starts past the portrait leaning in
        .portraitGap = 24.0f,
        .text = ui_color::kInk,
        .speaker = srgb(0xFFFFFF),
        .marker = ui_color::kGold,
        .choiceDim = ui_color::kSoftInk,
        .charsPerSecond = 52.0f,
        .sentencePause = 0.22f,
        .commaPause = 0.08f,
        .confirmGuard = 0.08f,
        .tagAt = gamelib::core::Vec2{344.0f, -34.0f},
        .tagPadding = {30.0f, 16.0f},
        .choiceHeight = 58.0f,
        .tagPainter = [&shapes](gamelib::draw::DrawList& list,
                                const Rect& tag) { addUi(list, shapes, Ui::Button, tag, srgb(0x8CCAD6), ui_color::kGold, 0.0f); },
        .choicePainter =
            [&shapes, time](gamelib::draw::DrawList& list, const Rect& row, const std::size_t, const bool selected) {
                if (!selected) {
                    return;
                }
                const Rect pill{row.x - 4.0f, row.y + 3.0f, row.width + 8.0f, row.height - 6.0f};
                addUi(list, shapes, Ui::Button, pill, ui_color::kButton, ui_color::kGold, 1.0f);
                addUi(list, shapes, Ui::Arrow, {pill.x + 4.0f + std::sin(*time * 8.0f) * 2.0f, pill.center().y - 9.0f, 18.0f, 18.0f},
                      ui_color::kGold, ui_color::kGold);
            },
        .markerPainter =
            [&shapes, time](gamelib::draw::DrawList& list, const gamelib::core::Vec2 at, const bool last) {
                const float bob = std::sin(*time * 6.0f) * (last ? 4.0f : 5.0f);
                const Color c = last ? ui_color::kTitle : ui_color::kGold;
                addUi(list, shapes, Ui::Arrow, {at.x - 24.0f, at.y - 28.0f + bob, 30.0f, 30.0f}, c, c);
            }};
}

} // namespace

DialogueView::DialogueView(const Shapes& shapes)
    : m_box{kBox, boxStyle(shapes, m_time)}
{
}

void DialogueView::open(std::string speaker, std::string role, const std::string_view text, std::vector<std::string> choices,
                        const std::optional<game::NpcId> portrait)
{
    if (!m_box.isOpen()) {
        m_age = 0.0f;
    }
    m_role = std::move(role);
    m_portrait = portrait;
    m_sinceComplete = 0.0f;
    m_box.open(std::move(speaker), text, std::move(choices));
}

void DialogueView::update(const float seconds)
{
    if (!m_box.isOpen()) {
        return;
    }
    m_age += seconds;
    m_box.update(seconds);
    m_sinceComplete = m_box.pageComplete() ? m_sinceComplete + seconds : 0.0f;
}

DialogueStep DialogueView::handle(const gamelib::ui::MenuInput input)
{
    using gamelib::ui::DialogueEvent;
    const gamelib::ui::DialogueResult r = m_box.handle(input);
    switch (r.event) {
    case DialogueEvent::Completed:
    case DialogueEvent::NextPage: m_sinceComplete = 0.0f; return DialogueStep::Completed;
    case DialogueEvent::Finished: return DialogueStep::Next;
    case DialogueEvent::Chose: m_choice = r.choice; return DialogueStep::Chose;
    case DialogueEvent::None: break;
    }
    return DialogueStep::None;
}

void DialogueView::draw(gamelib::draw::DrawList& list, const Shapes& shapes, const gamelib::core::Size canvas, const float time) const
{
    (void)canvas;
    if (!m_box.isOpen()) {
        return;
    }
    *m_time = time;
    const float pop = gamelib::core::easeOutBack(std::min(m_age / 0.25f, 1.0f));
    const float slide = (1.0f - pop) * 60.0f;
    list.setLayer(gamelib::draw::layers::Hud + 40);
    // The portrait: the speaker's avatar, big, leaning in over the box.
    if (m_portrait) {
        anime::Avatar a = npcAvatar(*m_portrait);
        a.feet = {kBox.x + 160.0f, kBox.y + 200.0f + slide};
        a.height = 560.0f;
        a.facing = 1.0f;
        a.pose = anime::AvatarPose::Idle;
        a.expression = anime::Expression::Neutral;
        // Eyes stay open; a page finishing gets a brief happy smile as a reaction.
        a.nextExpression = *m_portrait == game::NpcId::Brann ? anime::Expression::Determined : anime::Expression::Happy;
        a.blend = m_box.pageComplete()
                      ? std::min(m_sinceComplete * 6.0f, 1.0f) * (1.0f - std::clamp((m_sinceComplete - 0.55f) * 5.0f, 0.0f, 1.0f))
                      : 0.0f;
        list.add(anime::avatarSprite(a, shapes.avatar));
    }
    list.setLayer(gamelib::draw::layers::Hud + 41);
    gamelib::ui::drawDialogueBox(list, m_box, time, {0.0f, slide});
    if (!m_role.empty() && !m_box.speaker().empty()) {
        // The role sits inside the box beside the tag, never across the frame.
        const Rect tag = gamelib::ui::dialogueTagRect(m_box);
        ui::text(list, m_role, tag.right() + 14.0f, kBox.y + slide + 6.0f, {.size = 22.0f, .color = ui_color::kSoftInk, .weight = 1.3f});
    }
}

} // namespace wayfarer::app
