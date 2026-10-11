#pragma once

// The conversation box: GameLib's ui::DialogueBox (paging, the typewriter
// reveal with pauses after punctuation, choices) in Wayfarer's frames - the
// shader window, a pill name tag, the gold arrows - with the speaker's chibi
// portrait leaning in over the box's left end, the speaker's role beside the
// tag and a pop-in. One node at a time; the game walks the
// core::DialogueRunner.

#include "shapes.hpp"

#include "maps.hpp"

#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/ui/dialogue_box.hpp>
#include <gamelib/ui/menu.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace wayfarer::app {

enum class DialogueStep { None, Completed, Next, Chose };

class DialogueView {
public:
    explicit DialogueView(const Shapes& shapes);

    void open(std::string speaker, std::string role, std::string_view text, std::vector<std::string> choices,
              std::optional<game::NpcId> portrait);
    void close() { m_box.close(); }
    [[nodiscard]] bool isOpen() const { return m_box.isOpen(); }
    void update(float seconds);
    // Confirm completes the page, then turns it; on the last page it ends
    // (Next) or picks the selected choice (Chose).
    DialogueStep handle(gamelib::ui::MenuInput input);
    [[nodiscard]] std::size_t choice() const { return m_choice; }
    [[nodiscard]] int typedThisFrame() const { return static_cast<int>(m_box.revealedLastUpdate()); }
    void draw(gamelib::draw::DrawList& list, const Shapes& shapes, gamelib::core::Size canvas, float time) const;

private:
    // The frame's time, for the painters' bobbing arrows (shared with the
    // painters, so it survives the view being moved).
    std::shared_ptr<float> m_time = std::make_shared<float>(0.0f);
    gamelib::ui::DialogueBox m_box;
    std::string m_role;
    std::optional<game::NpcId> m_portrait;
    std::size_t m_choice = 0;
    float m_age = 0.0f;           // since the box opened (the pop-in)
    float m_sinceComplete = 0.0f; // since the page was typed out (the portrait's smile)
};

} // namespace wayfarer::app
