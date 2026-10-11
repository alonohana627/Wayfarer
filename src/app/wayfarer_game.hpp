#pragma once

// Wayfarer on GameLib's main loop: the title (the demo bot fighting across
// the Bellwood Path behind the menu), the three save slots, the world with
// its HUD, conversations, the windows (stats, skills, bag, quests, shop,
// forge), portals with soft transitions, fainting and waking at the inn,
// autosaves, region music by day and night, and the ending with the
// journey's numbers and the credits. The rules live in src/game; this is
// input, presentation, sound and saves.

#include "actions.hpp"
#include "dialogue_view.hpp"
#include "effects.hpp"
#include "hud.hpp"
#include "profile.hpp"
#include "saves.hpp"
#include "shapes.hpp"
#include "sounds.hpp"
#include "view.hpp"
#include "windows.hpp"

#include "demo.hpp"
#include "session.hpp"

#include <gamelib/ai/attract.hpp>
#include <gamelib/app/app.hpp>
#include <gamelib/app/arcade_menus.hpp>
#include <gamelib/app/dev_start.hpp>
#include <gamelib/core/dialogue.hpp>
#include <gamelib/ui/save_slot_list.hpp>
#include <gamelib/ui/transition.hpp>

#include <memory>
#include <optional>
#include <string>

namespace wayfarer::app {

inline constexpr float kEndingInputDelay = 6.0f;

class WayfarerGame final : public gamelib::app::Game {
public:
    WayfarerGame(gamelib::app::App& app, ProfileFile profile, bool demo);
    ~WayfarerGame() override;
    WayfarerGame(const WayfarerGame&) = delete("owns the menus that point into its profile");
    WayfarerGame& operator=(const WayfarerGame&) = delete("owns the menus that point into its profile");

    void onFocus(gamelib::app::App& app, bool focused) override;
    void onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event) override;
    void onGamepad(gamelib::app::App& app, const gamelib::platform::GamepadEvent& event) override;
    void tick(gamelib::app::App& app) override;
    void frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info) override;
    void draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info) override;

private:
    enum class Mode { Title, Slots, Play, Ending, Sheet };

    struct DemoRun {
        game::Session session;
        game::DemoBot bot;
    };

    // Input.
    void menuInput(gamelib::app::App& app, gamelib::ui::MenuInput input, bool fromKey);
    void handleResult(gamelib::app::App& app, const gamelib::app::ArcadeResult& result);
    void slotsCommand(gamelib::app::App& app, gamelib::ui::MenuInput input);
    bool playKey(gamelib::app::App& app, gamelib::platform::ActionId action);
    void dialogueInput(gamelib::app::App& app, gamelib::ui::MenuInput input);
    void windowFeedback(gamelib::app::App& app, WindowFeedback feedback);
    void openWindow(gamelib::app::App& app, WindowId id);

    // Flow.
    void startSlot(gamelib::app::App& app, int slot);
    void toTitle(gamelib::app::App& app);
    void playTick(gamelib::app::App& app);
    void react(gamelib::app::App& app, const game::Event& e);
    void openTalk(gamelib::app::App& app, const std::string& node);
    void showNode(gamelib::app::App& app);
    void startEnding(gamelib::app::App& app);
    void save();
    void updateMusic(gamelib::app::App& app);
    void applyDevScene(gamelib::app::App& app);
    [[nodiscard]] Prefs& prefs() { return m_profile.value().game; }
    [[nodiscard]] const Prefs& prefs() const { return m_profile.value().game; }

    // Screens (screens.cpp).
    void drawTitle(gamelib::draw::DrawList& list, float time) const;
    void drawMenu(gamelib::draw::DrawList& list, float time) const;
    void drawControls(gamelib::draw::DrawList& list, float time) const;
    void drawSlots(gamelib::draw::DrawList& list, float time) const;
    void drawEnding(gamelib::draw::DrawList& list, float time) const;
    void drawFainted(gamelib::draw::DrawList& list, float time) const;
    void drawSheet(gamelib::draw::DrawList& list, float time) const;
    void drawHints(gamelib::draw::DrawList& list, float time) const;

    ProfileFile m_profile; // the menus edit its settings in place: declared first
    gamelib::app::ArcadeMenus m_menus;
    Sounds m_sfx;
    Music m_music;
    Shapes m_shapes;
    Saves m_saves;
    gamelib::core::Size m_canvas;
    Mode m_mode = Mode::Title;
    gamelib::ui::SaveSlotList m_slotList{kSlotCount};
    int m_slot = 0;

    std::optional<game::Session> m_session;
    WorldView m_view;
    Effects m_effects;
    Hud m_hud;
    Windows m_windows;
    DialogueView m_dialogue;
    std::unique_ptr<gamelib::core::DialogueRunner> m_runner;
    std::optional<std::string> m_pendingService;
    gamelib::ui::Transition m_transition;
    float m_deadTime = 0.0f;
    float m_endingTime = 0.0f;
    game::RunStats m_endStats;
    int m_endLevel = 1;
    bool m_newBest = false;
    int m_sheet = 0;
    float m_hintTime = 0.0f;
    gamelib::app::DevStart m_dev;

    gamelib::ai::AttractMode<DemoRun> m_attract;
    WorldView m_demoView;
    Effects m_demoEffects;
    bool m_demoMode = false;
    float m_time = 0.0f;
    float m_seconds = 0.0f;
    float m_alpha = 1.0f;
    int m_typed = 0;
};

} // namespace wayfarer::app
