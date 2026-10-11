#include "dialogue_view.hpp"

#include "look.hpp"
#include "ui_kit.hpp"

#include "story.hpp"

#include <gamelib/core/easing.hpp>

#include <algorithm>
#include <cmath>

namespace wayfarer::app {

namespace {

// helpers

constexpr float kTextSize = 34.0f;
constexpr float kLineGap = 1.38f;
constexpr int kLinesPerPage = 3;
constexpr float kCharsPerSecond = 52.0f;
constexpr gamelib::core::Rect kBox{330.0f, 780.0f, 1300.0f, 250.0f};
constexpr float kTextLeft = 360.0f; // past the portrait

} // namespace

void DialogueView::open(std::string speaker, std::string role, const std::string_view text, std::vector<std::string> choices,
                        const std::optional<game::NpcId> portrait)
{
    const bool wasOpen = m_open;
    m_open = true;
    m_speaker = std::move(speaker);
    m_role = std::move(role);
    m_choices = std::move(choices);
    m_portrait = portrait;
    m_pages.clear();
    const float width = kBox.right() - (kBox.x + kTextLeft) - 50.0f;
    const std::vector<std::string> lines = ui::wrap(text, kTextSize, width);
    for (std::size_t i = 0; i < lines.size(); i += kLinesPerPage) {
        std::vector<std::string> page(lines.begin() + static_cast<std::ptrdiff_t>(i),
                                      lines.begin() + static_cast<std::ptrdiff_t>(std::min(lines.size(), i + kLinesPerPage)));
        m_pages.push_back(std::move(page));
    }
    if (m_pages.empty()) {
        m_pages.push_back({});
    }
    m_page = 0;
    m_shown = 0.0f;
    m_pause = 0.0f;
    m_selected = 0;
    m_sinceComplete = 0.0f;
    if (!wasOpen) {
        m_age = 0.0f;
    }
}

std::size_t DialogueView::pageChars() const
{
    std::size_t n = 0;
    for (const std::string& line : m_pages[m_page]) {
        n += line.size();
    }
    return n;
}

void DialogueView::update(const float seconds)
{
    m_typedNow = 0;
    if (!m_open) {
        return;
    }
    m_age += seconds;
    if (complete()) {
        m_sinceComplete += seconds;
        return;
    }
    if (m_pause > 0.0f) {
        m_pause -= seconds;
        return;
    }
    const float before = m_shown;
    m_shown = std::min(m_shown + seconds * kCharsPerSecond, static_cast<float>(pageChars()));
    m_typedNow = static_cast<int>(m_shown) - static_cast<int>(before);
    // A breath after sentences and commas.
    std::size_t index = static_cast<std::size_t>(m_shown);
    std::size_t n = 0;
    for (const std::string& line : m_pages[m_page]) {
        if (index > n && index <= n + line.size()) {
            const char c = line[index - n - 1];
            if (static_cast<int>(before) != static_cast<int>(m_shown)) {
                if (c == '.' || c == '!' || c == '?') {
                    m_pause = 0.22f;
                } else if (c == ',') {
                    m_pause = 0.08f;
                }
            }
        }
        n += line.size();
    }
}

DialogueStep DialogueView::handle(const gamelib::ui::MenuInput input)
{
    using gamelib::ui::MenuInput;
    if (!m_open) {
        return DialogueStep::None;
    }
    const bool choosing = lastPage() && complete() && !m_choices.empty();
    if (choosing && (input == MenuInput::Up || input == MenuInput::Down)) {
        const std::size_t n = m_choices.size();
        m_selected = input == MenuInput::Up ? (m_selected + n - 1) % n : (m_selected + 1) % n;
        return DialogueStep::None;
    }
    if (input != MenuInput::Confirm) {
        return DialogueStep::None;
    }
    if (!complete()) {
        m_shown = static_cast<float>(pageChars());
        m_sinceComplete = 0.0f;
        return DialogueStep::Completed;
    }
    if (m_sinceComplete < 0.08f) {
        return DialogueStep::None; // a double tap never skips a page unread
    }
    if (!lastPage()) {
        ++m_page;
        m_shown = 0.0f;
        m_sinceComplete = 0.0f;
        return DialogueStep::Completed;
    }
    return m_choices.empty() ? DialogueStep::Next : DialogueStep::Chose;
}

void DialogueView::draw(gamelib::draw::DrawList& list, const Shapes& shapes, const gamelib::core::Size canvas, const float time) const
{
    (void)canvas;
    if (!m_open) {
        return;
    }
    const float pop = gamelib::core::easeOutBack(std::min(m_age / 0.25f, 1.0f));
    const float slide = (1.0f - pop) * 60.0f;
    list.setLayer(gamelib::draw::layers::Hud + 40);
    // The portrait: the speaker's avatar, big, leaning in over the box.
    if (m_portrait) {
        anime::Avatar a = npcAvatar(*m_portrait);
        a.feet = {kBox.x + 150.0f, kBox.bottom() + 240.0f + slide};
        a.height = 560.0f;
        a.facing = 1.0f;
        a.pose = anime::AvatarPose::Idle;
        a.expression = anime::Expression::Neutral;
        a.nextExpression = complete() ? anime::Expression::Happy : anime::Expression::Neutral;
        a.blend = std::min(m_sinceComplete * 6.0f, 1.0f);
        if (*m_portrait == game::NpcId::Brann) {
            a.nextExpression = anime::Expression::Determined;
        }
        list.add(anime::avatarSprite(a, shapes.avatar));
    }
    list.setLayer(gamelib::draw::layers::Hud + 41);
    const gamelib::core::Rect box{kBox.x, kBox.y + slide, kBox.width, kBox.height};
    addUi(list, shapes, Ui::Window, box.expanded(10.0f), ui_color::kWindow, ui_color::kTitle, 0.0f, 0.0f);
    // The name tag.
    const float nameW = ui::textWidth(m_speaker, 30.0f) + 60.0f;
    const gamelib::core::Rect tag{box.x + kTextLeft - 16.0f, box.y - 34.0f, nameW, 54.0f};
    addUi(list, shapes, Ui::Button, tag, srgb(0x8CCAD6), ui_color::kGold, 0.0f);
    ui::text(list, m_speaker, tag.center().x, tag.y + 10.0f,
             {.size = 30.0f, .color = srgb(0xFFFFFF), .weight = 1.7f, .align = ui::Align::Center, .outline = true});
    if (!m_role.empty()) {
        ui::text(list, m_role, tag.right() + 14.0f, tag.y + 18.0f, {.size = 22.0f, .color = ui_color::kSoftInk});
    }
    // The text, typed out.
    float y = box.y + 34.0f;
    std::size_t left = static_cast<std::size_t>(m_shown);
    for (const std::string& line : m_pages[m_page]) {
        const std::size_t take = std::min(left, line.size());
        ui::text(list, std::string_view{line}.substr(0, take), box.x + kTextLeft, y, {.size = kTextSize, .color = ui_color::kInk, .weight = 1.3f});
        left -= take;
        y += kTextSize * kLineGap;
    }
    if (complete() && !lastPage()) {
        const float bob = std::sin(time * 6.0f) * 5.0f;
        addUi(list, shapes, Ui::Arrow, {box.right() - 56.0f, box.bottom() - 52.0f + bob, 30.0f, 30.0f}, ui_color::kGold, ui_color::kGold);
    }
    if (complete() && lastPage() && m_choices.empty()) {
        const float bob = std::sin(time * 6.0f) * 4.0f;
        addUi(list, shapes, Ui::Arrow, {box.right() - 56.0f, box.bottom() - 52.0f + bob, 30.0f, 30.0f}, ui_color::kTitle, ui_color::kTitle);
    }
    // Choices above the box's right end.
    if (complete() && lastPage() && !m_choices.empty()) {
        float w = 0.0f;
        for (const std::string& c : m_choices) {
            w = std::max(w, ui::textWidth(c, 30.0f));
        }
        w += 110.0f;
        const float h = 58.0f;
        const float total = h * static_cast<float>(m_choices.size()) + 20.0f;
        const gamelib::core::Rect panel{box.right() - w, box.y - total - 26.0f, w, total};
        list.setLayer(gamelib::draw::layers::Hud + 42);
        addUi(list, shapes, Ui::Window, panel.expanded(10.0f), ui_color::kWindow, ui_color::kTitle, 0.0f, 0.0f);
        for (std::size_t i = 0; i < m_choices.size(); ++i) {
            const gamelib::core::Rect row{panel.x + 10.0f, panel.y + 10.0f + h * static_cast<float>(i), w - 20.0f, h - 6.0f};
            const bool sel = i == m_selected;
            if (sel) {
                addUi(list, shapes, Ui::Button, row, ui_color::kButton, ui_color::kGold, 1.0f);
                addUi(list, shapes, Ui::Arrow, {row.x + 10.0f + std::sin(time * 8.0f) * 3.0f, row.center().y - 11.0f, 22.0f, 22.0f},
                      ui_color::kGold, ui_color::kGold);
            }
            ui::text(list, m_choices[i], row.x + 44.0f, row.center().y - 17.0f,
                     {.size = 30.0f, .color = sel ? ui_color::kInk : ui_color::kSoftInk, .weight = sel ? 1.5f : 1.25f});
        }
    }
}

} // namespace wayfarer::app
