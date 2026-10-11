#pragma once

// The conversation box: the speaker's chibi portrait leaning in over the
// box's left end, a name tag, lowercase text typed out with soft pauses after
// punctuation (GameLib's vector font, wrapped to the box), a bobbing "more"
// arrow, and choices in a little list. One node at a time; the game walks
// the core::DialogueRunner. (GameLib's ui::DialogueBox wraps the pixel font
// only, so this one does its own wrapping for the vector font.)

#include "shapes.hpp"

#include "maps.hpp"

#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/ui/menu.hpp>

#include <optional>
#include <string>
#include <vector>

namespace wayfarer::app {

enum class DialogueStep { None, Completed, Next, Chose };

class DialogueView {
public:
    void open(std::string speaker, std::string role, std::string_view text, std::vector<std::string> choices,
              std::optional<game::NpcId> portrait);
    void close() { m_open = false; }
    [[nodiscard]] bool isOpen() const { return m_open; }
    void update(float seconds);
    // Confirm completes the page, then turns it; on the last page it ends
    // (Next) or picks the selected choice (Chose).
    DialogueStep handle(gamelib::ui::MenuInput input);
    [[nodiscard]] std::size_t choice() const { return m_selected; }
    [[nodiscard]] int typedThisFrame() const { return m_typedNow; }
    void draw(gamelib::draw::DrawList& list, const Shapes& shapes, gamelib::core::Size canvas, float time) const;

private:
    [[nodiscard]] std::size_t pageChars() const;
    [[nodiscard]] bool lastPage() const { return m_page + 1 >= m_pages.size(); }
    [[nodiscard]] bool complete() const { return m_shown >= static_cast<float>(pageChars()); }

    bool m_open = false;
    std::string m_speaker;
    std::string m_role;
    std::vector<std::vector<std::string>> m_pages;
    std::vector<std::string> m_choices;
    std::optional<game::NpcId> m_portrait;
    std::size_t m_page = 0;
    float m_shown = 0.0f;  // characters revealed on the page
    float m_pause = 0.0f;
    float m_age = 0.0f;    // since the box opened (the pop-in)
    float m_sinceComplete = 0.0f;
    std::size_t m_selected = 0;
    int m_typedNow = 0;
};

} // namespace wayfarer::app
