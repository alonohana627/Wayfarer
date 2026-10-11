#include "wayfarer_game.hpp"

#include "look.hpp"

#include <gamelib/app/settings.hpp>
#include <gamelib/core/rng.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <sstream>
#include <string_view>
#include <utility>

namespace wayfarer::app {

namespace {

// helpers

using gamelib::app::ArcadeAction;
using gamelib::app::ArcadeScreen;
using gamelib::core::Vec2;
using gamelib::platform::Key;
using gamelib::ui::MenuInput;

constexpr int kControlsCommand = 7;
constexpr int kQuitToTitleCommand = 8;

constexpr float kFaintSeconds = 2.4f;

std::optional<game::NpcId> npcBySpeaker(const std::string_view speaker)
{
    for (int i = 0; i < game::kNpcCount; ++i) {
        if (game::npcName(static_cast<game::NpcId>(i)) == speaker) {
            return static_cast<game::NpcId>(i);
        }
    }
    return std::nullopt;
}

int toInt(const std::string_view text)
{
    int v = 0;
    (void)std::from_chars(text.data(), text.data() + text.size(), v);
    return v;
}

} // namespace

WayfarerGame::WayfarerGame(gamelib::app::App& app, ProfileFile profile, const bool demo)
    : m_profile{std::move(profile)}
    , m_menus{"WAYFARER", m_profile.value().settings, {}, {.music = true, .vsync = true, .controls = kControlsCommand}}
    , m_sfx{app.audio(), sfxRecipe}
    , m_music{app.audio(), 2, [](const Tune t, const int variant) -> std::optional<gamelib::audio::Song> {
                  const bool hasNight = t == Tune::Town || t == Tune::Forest || t == Tune::Marsh || t == Tune::Ruins || t == Tune::Peak;
                  if (variant == 1 && !hasNight) {
                      return std::nullopt;
                  }
                  return tune(t, variant);
              }}
    , m_shapes{Shapes::registerWith(app.renderer())}
    , m_canvas{app.canvas()}
    , m_view{app.canvas()}
    , m_dev{gamelib::app::readDevStart("WAYFARER")}
    , m_attract{[](const std::uint64_t seed) { return DemoRun{.session = game::demoSession(seed), .bot = {}}; },
                [](DemoRun& run, float) {
                    run.session.tick(game::demoInput(run.session, run.bot));
                    (void)run.session.events().take();
                },
                [](const DemoRun& run) { return game::demoDone(run.session) || run.session.ticks() > 60 * 90; }, 11}
    , m_demoView{app.canvas()}
    , m_demoMode{demo}
{
    m_menus.setSounds({.moved = gamelib::app::MenuSound{m_sfx.id(Sfx::MenuMove)},
                       .changed = gamelib::app::MenuSound{m_sfx.id(Sfx::MenuMove)},
                       .selected = gamelib::app::MenuSound{m_sfx.id(Sfx::MenuSelect)},
                       .back = gamelib::app::MenuSound{m_sfx.id(Sfx::MenuBack)}});
    m_menus.hostControls(prefs().actions);
    m_profile.save();
    gamelib::app::applySettings(app, m_profile.value().settings);
    m_demoView.snap(m_attract.sim().session);
    m_demoEffects.setRegion(game::Region::Forest, false);
    m_music.play(app.audio(), Tune::Title, 0.6f);
    if (m_dev.scene && *m_dev.scene == "sheet") {
        m_mode = Mode::Sheet;
    } else if (m_dev.scene || m_dev.level) {
        startSlot(app, prefs().lastSlot);
    }
}

WayfarerGame::~WayfarerGame()
{
    if (m_session && m_mode == Mode::Play && !m_demoMode) {
        m_saves.save(m_slot, m_session->save());
    }
    save();
}

void WayfarerGame::save()
{
    m_profile.save();
}

// ---------------------------------------------------------------------------
// Input

void WayfarerGame::onFocus(gamelib::app::App& app, const bool focused)
{
    if (!m_demoMode && m_mode == Mode::Play && !m_windows.isOpen() && !m_dialogue.isOpen()) {
        m_menus.onFocus(app, focused, m_time);
    }
}

void WayfarerGame::onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event)
{
    if (m_demoMode) {
        if (event.pressed && event.key != Key::F11) {
            m_demoMode = false;
            app.input().releaseAll();
        }
        return;
    }
    const auto input = gamelib::ui::menuInput(prefs().actions, event, menuBindings());
    const gamelib::platform::ActionId action = prefs().actions.actionOn(event.key);
    if (m_mode == Mode::Sheet) {
        if (event.pressed && input) {
            if (*input == MenuInput::Back) {
                toTitle(app);
            } else if (*input == MenuInput::Right || *input == MenuInput::Confirm) {
                m_sheet = (m_sheet + 1) % 4;
            } else if (*input == MenuInput::Left) {
                m_sheet = (m_sheet + 3) % 4;
            }
        }
        return;
    }
    if (m_mode == Mode::Ending) {
        if (event.pressed && m_endingTime > kEndingInputDelay && input && (*input == MenuInput::Confirm || *input == MenuInput::Back)) {
            toTitle(app);
        }
        return;
    }
    if (m_mode == Mode::Slots) {
        if (event.pressed && input) {
            slotsCommand(app, *input);
        } else if (event.pressed && event.key == Key::Delete) {
            slotsCommand(app, MenuInput::Left);
        }
        return;
    }
    if (m_mode == Mode::Play && m_dialogue.isOpen()) {
        if (event.pressed && input) {
            dialogueInput(app, *input);
        }
        return;
    }
    if (m_mode == Mode::Play && m_windows.isOpen()) {
        if (!event.pressed || !m_session) {
            return;
        }
        const WindowId current = m_windows.current();
        const auto windowFor = [](const gamelib::platform::ActionId a) {
            if (a == actionId(Action::Stats)) {
                return WindowId::Stats;
            }
            if (a == actionId(Action::Skills)) {
                return WindowId::Skills;
            }
            if (a == actionId(Action::Inventory)) {
                return WindowId::Inventory;
            }
            if (a == actionId(Action::Quests)) {
                return WindowId::Quests;
            }
            return WindowId::None;
        };
        const WindowId target = windowFor(action);
        if (target == current) {
            m_windows.close();
            windowFeedback(app, WindowFeedback::Closed);
        } else if (target != WindowId::None) {
            openWindow(app, target);
        } else if (action == actionId(Action::PotionHp)) {
            windowFeedback(app, m_windows.handleKey(WindowKey::QuickHp, *m_session));
        } else if (action == actionId(Action::PotionMp)) {
            windowFeedback(app, m_windows.handleKey(WindowKey::QuickMp, *m_session));
        } else if (event.key == Key::Tab) {
            windowFeedback(app, m_windows.handleKey(WindowKey::Tab, *m_session));
        } else if (action == actionId(Action::Skill1) && current == WindowId::Stats) {
            windowFeedback(app, m_windows.handleKey(WindowKey::Auto, *m_session));
        } else if (input) {
            windowFeedback(app, m_windows.handle(*input, *m_session));
        }
        if (!m_windows.isOpen()) {
            app.input().releaseAll();
            if (m_pendingService) {
                m_pendingService.reset();
            }
        }
        return;
    }
    if (m_mode == Mode::Play && m_session && (m_session->phase() != game::Phase::Playing || m_transition.active())) {
        return;
    }
    const gamelib::app::ArcadeResult result = m_menus.onKey(app, event, m_time);
    handleResult(app, result);
    if (event.pressed && m_mode == Mode::Play && result.from == ArcadeScreen::Playing && m_menus.is(ArcadeScreen::Playing)) {
        (void)playKey(app, action);
    }
}

void WayfarerGame::onGamepad(gamelib::app::App& app, const gamelib::platform::GamepadEvent& event)
{
    if (m_demoMode) {
        if (event.pressed) {
            m_demoMode = false;
            app.input().releaseAll();
        }
        return;
    }
    const auto input = gamelib::ui::menuInput(prefs().actions, event, menuBindings());
    const gamelib::platform::ActionId action = prefs().actions.actionOn(event.button);
    if (m_mode == Mode::Sheet) {
        if (event.pressed && input && *input == MenuInput::Back) {
            toTitle(app);
        }
        return;
    }
    if (m_mode == Mode::Ending) {
        if (event.pressed && m_endingTime > kEndingInputDelay && input) {
            toTitle(app);
        }
        return;
    }
    if (m_mode == Mode::Slots) {
        if (event.pressed && input) {
            slotsCommand(app, *input);
        }
        return;
    }
    if (m_mode == Mode::Play && m_dialogue.isOpen()) {
        if (event.pressed && input) {
            dialogueInput(app, *input);
        }
        return;
    }
    if (m_mode == Mode::Play && m_windows.isOpen()) {
        if (event.pressed && m_session) {
            if (action == actionId(Action::Inventory) || event.button == gamelib::platform::GamepadButton::Start) {
                m_windows.close();
                windowFeedback(app, WindowFeedback::Closed);
            } else if (event.button == gamelib::platform::GamepadButton::LeftBumper) {
                windowFeedback(app, m_windows.handleKey(WindowKey::Tab, *m_session));
            } else if (input) {
                windowFeedback(app, m_windows.handle(*input, *m_session));
            }
            if (!m_windows.isOpen()) {
                app.input().releaseAll();
            }
        }
        return;
    }
    if (m_mode == Mode::Play && m_session && (m_session->phase() != game::Phase::Playing || m_transition.active())) {
        return;
    }
    const gamelib::app::ArcadeResult result = m_menus.onGamepad(app, event, m_time);
    handleResult(app, result);
    if (event.pressed && m_mode == Mode::Play && result.from == ArcadeScreen::Playing && m_menus.is(ArcadeScreen::Playing)) {
        (void)playKey(app, action);
    }
}

bool WayfarerGame::playKey(gamelib::app::App& app, const gamelib::platform::ActionId action)
{
    if (!m_session || m_session->phase() != game::Phase::Playing) {
        return false;
    }
    if (action == actionId(Action::Stats)) {
        openWindow(app, WindowId::Stats);
    } else if (action == actionId(Action::Skills)) {
        openWindow(app, WindowId::Skills);
    } else if (action == actionId(Action::Inventory)) {
        openWindow(app, WindowId::Inventory);
    } else if (action == actionId(Action::Quests)) {
        openWindow(app, WindowId::Quests);
    } else if (action == actionId(Action::Minimap)) {
        prefs().bigMinimap = !prefs().bigMinimap;
        save();
        m_sfx.queue(app.sounds(), Sfx::MenuMove);
    } else {
        return false;
    }
    return true;
}

void WayfarerGame::openWindow(gamelib::app::App& app, const WindowId id)
{
    m_windows.open(id);
    m_sfx.queue(app.sounds(), Sfx::Open);
    app.input().releaseAll();
}

void WayfarerGame::windowFeedback(gamelib::app::App& app, const WindowFeedback feedback)
{
    auto& q = app.sounds();
    switch (feedback) {
    case WindowFeedback::Moved: m_sfx.queue(q, Sfx::MenuMove); break;
    case WindowFeedback::Done:
        m_sfx.queue(q, m_windows.current() == WindowId::Stats ? Sfx::Point : Sfx::MenuSelect);
        break;
    case WindowFeedback::Failed: m_sfx.queue(q, Sfx::Error); break;
    case WindowFeedback::Closed: m_sfx.queue(q, Sfx::Close); break;
    case WindowFeedback::Tab: m_sfx.queue(q, Sfx::MenuMove); break;
    case WindowFeedback::Big: m_sfx.queue(q, m_windows.current() == WindowId::Smith ? Sfx::Upgrade : Sfx::Learn); break;
    case WindowFeedback::None: break;
    }
    // Purchases, upgrades and equips push events: play their sounds now.
    if (m_session) {
        for (const game::Event& e : m_session->events().take()) {
            react(app, e);
        }
    }
}

void WayfarerGame::handleResult(gamelib::app::App& app, const gamelib::app::ArcadeResult& result)
{
    if (result.changed) {
        save();
        gamelib::app::applySettings(app, m_profile.value().settings);
    }
    switch (result.action) {
    case ArcadeAction::Start:
        if (result.from == ArcadeScreen::Paused) {
            // RESTART: back to the last save.
            startSlot(app, m_slot);
            break;
        }
        m_mode = Mode::Slots;
        m_slotList.setRows(m_saves.rows());
        m_slotList.setCursor(prefs().lastSlot);
        app.input().releaseAll();
        break;
    case ArcadeAction::ToTitle: toTitle(app); break;
    case ArcadeAction::Quit:
        if (m_session && m_mode == Mode::Play) {
            m_saves.save(m_slot, m_session->save());
        }
        save();
        app.requestQuit();
        break;
    case ArcadeAction::Game:
        if (result.command == kQuitToTitleCommand) {
            toTitle(app);
        }
        break;
    case ArcadeAction::Resume:
    case ArcadeAction::Controls:
    case ArcadeAction::None: break;
    }
}

void WayfarerGame::slotsCommand(gamelib::app::App& app, const MenuInput input)
{
    const gamelib::ui::SlotResult r = m_slotList.handle(input);
    switch (r.action) {
    case gamelib::ui::SlotAction::Moved: m_sfx.queue(app.sounds(), Sfx::MenuMove); break;
    case gamelib::ui::SlotAction::EraseAsked: m_sfx.queue(app.sounds(), Sfx::Error); break;
    case gamelib::ui::SlotAction::Erase:
        m_saves.erase(r.slot);
        m_slotList.setRows(m_saves.rows());
        m_sfx.queue(app.sounds(), Sfx::Sell);
        break;
    case gamelib::ui::SlotAction::Start:
        m_sfx.queue(app.sounds(), Sfx::MenuSelect);
        startSlot(app, r.slot);
        break;
    case gamelib::ui::SlotAction::Back:
        m_sfx.queue(app.sounds(), Sfx::MenuBack);
        m_mode = Mode::Title;
        m_menus.go(ArcadeScreen::Title, m_time);
        break;
    case gamelib::ui::SlotAction::None: break;
    }
}

void WayfarerGame::dialogueInput(gamelib::app::App& app, const MenuInput input)
{
    if (!m_runner) {
        m_dialogue.close();
        return;
    }
    const DialogueStep step = m_dialogue.handle(input);
    switch (step) {
    case DialogueStep::Completed: m_sfx.queue(app.sounds(), Sfx::MenuMove, 0.5f); break;
    case DialogueStep::Next:
        m_sfx.queue(app.sounds(), Sfx::MenuMove, 0.6f);
        m_runner->advance();
        showNode(app);
        break;
    case DialogueStep::Chose:
        m_sfx.queue(app.sounds(), Sfx::MenuSelect);
        m_runner->choose(m_dialogue.choice());
        showNode(app);
        break;
    case DialogueStep::None:
        if (input == MenuInput::Up || input == MenuInput::Down) {
            m_sfx.queue(app.sounds(), Sfx::MenuMove, 0.6f);
        }
        break;
    }
}

// ---------------------------------------------------------------------------
// Flow

void WayfarerGame::startSlot(gamelib::app::App& app, const int slot)
{
    m_slot = slot;
    prefs().lastSlot = slot;
    save();
    const std::uint64_t seed = gamelib::core::randomSeed();
    if (std::optional<game::SaveData> data = m_saves.load(slot)) {
        m_session.emplace(*data, seed);
    } else {
        m_session.emplace(seed);
    }
    (void)m_session->events().take();
    m_view.snap(*m_session);
    m_effects.clear();
    m_effects.setRegion(m_session->map().region, m_session->map().dungeon);
    m_effects.setShakeScale(m_profile.value().settings.screenShake ? 1.0f : 0.0f);
    m_hud.reset(*m_session);
    m_hud.mapTitle(*m_session);
    m_windows.close();
    m_dialogue.close();
    m_runner.reset();
    m_pendingService.reset();
    m_transition.cancel();
    m_mode = Mode::Play;
    m_menus.go(ArcadeScreen::Playing, m_time);
    app.input().releaseAll();
    m_deadTime = 0.0f;
    m_hintTime = 0.0f;
    if (m_session->quests().stateOf(game::kQuestArrival) == gamelib::core::QuestState::Available) {
        m_hud.toast("welcome to hearthvale! talk to elder oswin", srgb(0xFFE08A));
    }
    applyDevScene(app);
    updateMusic(app);
}

void WayfarerGame::applyDevScene(gamelib::app::App& app)
{
    if (!m_session) {
        return;
    }
    game::Session& s = *m_session;
    if (m_dev.level) {
        s.debugLevel(*m_dev.level);
    }
    if (!m_dev.scene) {
        return;
    }
    std::istringstream tokens{*m_dev.scene};
    std::string token;
    while (std::getline(tokens, token, ',')) {
        if (token.starts_with("chapter")) {
            s.debugGrantChapter(toInt(std::string_view{token}.substr(7)));
        } else if (token.starts_with("map")) {
            s.debugTeleport(static_cast<game::MapId>(std::clamp(toInt(std::string_view{token}.substr(3)), 0, game::kMapCount - 1)), 0);
            m_view.snap(s);
            m_effects.setRegion(s.map().region, s.map().dungeon);
            m_hud.mapTitle(s);
        } else if (token.starts_with("level")) {
            s.debugLevel(toInt(std::string_view{token}.substr(5)));
        } else if (token == "night") {
            s.debugTime(22);
        } else if (token == "dusk") {
            s.debugTime(18);
        } else if (token == "gear") {
            s.debugGear();
        } else if (token == "learn") {
            s.debugLearnAll();
        } else if (token == "stats") {
            openWindow(app, WindowId::Stats);
        } else if (token == "skills") {
            openWindow(app, WindowId::Skills);
        } else if (token == "inventory") {
            openWindow(app, WindowId::Inventory);
        } else if (token == "quests") {
            openWindow(app, WindowId::Quests);
        } else if (token == "shop") {
            openWindow(app, WindowId::Shop);
        } else if (token == "smith") {
            openWindow(app, WindowId::Smith);
        } else if (token.starts_with("talk")) {
            openTalk(app, game::npcRoot(static_cast<game::NpcId>(toInt(std::string_view{token}.substr(4)))));
        } else if (token == "bosslow") {
            s.debugBossLow();
        } else if (token == "ending") {
            startEnding(app);
        } else if (token.starts_with("x")) {
            s.debugMove(static_cast<float>(toInt(std::string_view{token}.substr(1))) * game::kTile);
            m_view.snap(s);
        }
    }
    (void)s.events().take();
}

void WayfarerGame::toTitle(gamelib::app::App& app)
{
    if (m_session && m_mode == Mode::Play) {
        m_saves.save(m_slot, m_session->save());
    }
    m_session.reset();
    m_transition.cancel();
    m_windows.close();
    m_dialogue.close();
    m_runner.reset();
    m_mode = Mode::Title;
    m_menus.go(ArcadeScreen::Title, m_time);
    save();
    m_music.play(app.audio(), Tune::Title, 0.8f);
    app.input().releaseAll();
}

void WayfarerGame::openTalk(gamelib::app::App& app, const std::string& node)
{
    game::Session& s = *m_session;
    m_runner = std::make_unique<gamelib::core::DialogueRunner>(
        game::dialogueTree(), [&s](const std::string_view c) { return s.condition(c); }, [&s](const std::string_view e) { s.effect(e); });
    m_runner->start(node);
    for (const game::Event& e : s.events().take()) {
        react(app, e);
    }
    m_sfx.queue(app.sounds(), Sfx::Open);
    app.input().releaseAll();
    showNode(app);
}

void WayfarerGame::showNode(gamelib::app::App& app)
{
    game::Session& s = *m_session;
    for (const game::Event& e : s.events().take()) {
        react(app, e);
    }
    if (const std::optional<std::string> service = s.takeServiceRequest()) {
        m_pendingService = service;
    }
    if (!m_runner || m_runner->done() || m_runner->node() == nullptr) {
        m_dialogue.close();
        m_runner.reset();
        app.input().releaseAll();
        if (m_pendingService) {
            openWindow(app, *m_pendingService == "shop" ? WindowId::Shop : WindowId::Smith);
            m_pendingService.reset();
        }
        return;
    }
    const gamelib::core::DialogueNode* node = m_runner->node();
    const std::optional<game::NpcId> npc = npcBySpeaker(node->speaker);
    m_dialogue.open(node->speaker, npc ? std::string{game::npcRole(*npc)} : std::string{}, node->text, m_runner->choiceLabels(), npc);
}

void WayfarerGame::startEnding(gamelib::app::App& app)
{
    if (!m_session) {
        return;
    }
    m_endStats = m_session->stats();
    m_endLevel = m_session->character().level;
    m_saves.save(m_slot, m_session->save());
    m_newBest = m_profile.value().bests.offer(kBestTime, m_endStats.ticks, gamelib::app::Bests::Better::Lower);
    save();
    m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = srgb(0xFFF4E0), .coverSeconds = 1.6f, .holdSeconds = 0.6f,
                        .uncoverSeconds = 1.6f},
                       [this, &app] {
                           m_mode = Mode::Ending;
                           m_endingTime = 0.0f;
                           m_windows.close();
                           m_dialogue.close();
                           m_music.play(app.audio(), Tune::Ending, 1.5f);
                       });
}

void WayfarerGame::tick(gamelib::app::App& app)
{
    if (m_demoMode || m_mode == Mode::Title || m_mode == Mode::Slots) {
        m_attract.tick(app.tickSeconds());
        return;
    }
    if (m_mode != Mode::Play || !m_session || !m_menus.is(ArcadeScreen::Playing) || m_windows.isOpen() || m_dialogue.isOpen()) {
        return;
    }
    if (m_transition.active() && !m_session->travelPending()) {
        return;
    }
    playTick(app);
}

void WayfarerGame::playTick(gamelib::app::App& app)
{
    game::Session& s = *m_session;
    if (s.travelPending()) {
        return; // the transition is running
    }
    game::Input input = readInput(prefs().actions, app.input());
    s.tick(input);
    for (const game::Event& e : s.events().take()) {
        react(app, e);
    }
    if (const std::optional<std::string> talk = s.takeTalkRequest()) {
        openTalk(app, *talk);
        return;
    }
    if (s.travelPending() && !m_transition.active()) {
        m_sfx.queue(app.sounds(), Sfx::Portal);
        m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = srgb(0x1E1830), .coverSeconds = 0.32f,
                            .holdSeconds = 0.12f, .uncoverSeconds = 0.45f},
                           [this] {
                               m_session->travel();
                               for (const game::Event& e : m_session->events().take()) {
                                   (void)e;
                               }
                               m_view.snap(*m_session);
                               m_effects.clear();
                               m_effects.setRegion(m_session->map().region, m_session->map().dungeon);
                               m_hud.mapTitle(*m_session);
                               if (m_session->boss()) {
                                   m_hud.reset(*m_session);
                               }
                           });
    }
    if (s.wantsSave()) {
        m_saves.save(m_slot, s.save());
    }
    if (s.phase() == game::Phase::Ending && !m_transition.active() && m_mode == Mode::Play) {
        startEnding(app);
    }
}

void WayfarerGame::react(gamelib::app::App& app, const game::Event& e)
{
    using game::EventKind;
    if (!m_session) {
        return;
    }
    m_effects.react(e, *m_session);
    m_hud.react(e, *m_session);
    auto& q = app.sounds();
    switch (e.kind) {
    case EventKind::Jump: m_sfx.queue(q, Sfx::Jump); break;
    case EventKind::HyperJump: m_sfx.queue(q, Sfx::HyperJump); break;
    case EventKind::Land:
        if (e.value > 12) {
            m_sfx.queue(q, Sfx::Land);
        }
        break;
    case EventKind::Swing: m_sfx.queue(q, Sfx::Swing, 1.0f, e.value == 0 ? 1.0f : e.value == 1 ? 1.12f : 0.85f); break;
    case EventKind::Hit:
        m_sfx.queue(q, e.crit ? Sfx::Crit : Sfx::Hit);
        if (e.extra > 0) {
            app.hitStop(static_cast<float>(e.extra) / 60.0f * (e.crit ? 1.4f : 1.0f));
        }
        break;
    case EventKind::Guarded: m_sfx.queue(q, Sfx::Guard); break;
    case EventKind::MonsterDown: m_sfx.queue(q, Sfx::MonsterDown, e.extra != 0 ? 1.0f : 0.8f); break;
    case EventKind::Hurt:
        m_sfx.queue(q, Sfx::Hurt);
        app.hitStop(0.06f);
        break;
    case EventKind::Death:
        m_sfx.queue(q, Sfx::Death);
        m_deadTime = 0.0f;
        break;
    case EventKind::Heal: m_sfx.queue(q, Sfx::Bloom); break;
    case EventKind::Pickup: {
        const auto r = static_cast<game::Rarity>(std::clamp(e.extra, 0, 3));
        m_sfx.queue(q, r == game::Rarity::Legendary ? Sfx::PickupLegend
                       : r == game::Rarity::Epic    ? Sfx::PickupEpic
                       : r == game::Rarity::Rare    ? Sfx::PickupRare
                                                    : Sfx::Pickup);
        break;
    }
    case EventKind::Coins: m_sfx.queue(q, Sfx::Coin, 0.8f); break;
    case EventKind::LevelUp: m_sfx.queue(q, Sfx::LevelUp); break;
    case EventKind::SkillCast:
        switch (static_cast<game::SkillId>(e.value)) {
        case game::SkillId::Sweep: m_sfx.queue(q, Sfx::Sweep); break;
        case game::SkillId::Starbolt: m_sfx.queue(q, Sfx::Star); break;
        case game::SkillId::Tailwind: m_sfx.queue(q, Sfx::Wind); break;
        case game::SkillId::Roll: m_sfx.queue(q, Sfx::Roll); break;
        default: break;
        }
        break;
    case EventKind::SkillLearned: m_sfx.queue(q, Sfx::Learn); break;
    case EventKind::NoMana:
    case EventKind::Cooldown: m_sfx.queue(q, Sfx::NoMana, 0.7f); break;
    case EventKind::Potion: m_sfx.queue(q, Sfx::Potion); break;
    case EventKind::QuestAccepted: m_sfx.queue(q, Sfx::QuestAccept); break;
    case EventKind::QuestComplete: m_sfx.queue(q, Sfx::QuestComplete); break;
    case EventKind::QuestDone: m_sfx.queue(q, Sfx::QuestDone); break;
    case EventKind::PortalClosed: m_sfx.queue(q, Sfx::Sealed); break;
    case EventKind::BossWake: m_sfx.queue(q, Sfx::BossRoar); break;
    case EventKind::BossRoar: m_sfx.queue(q, Sfx::BossRoar, 0.6f); break;
    case EventKind::BossPhase: m_sfx.queue(q, Sfx::BossRoar, 1.0f, 1.2f); break;
    case EventKind::BossLand: m_sfx.queue(q, Sfx::BossLand); break;
    case EventKind::BossDown:
        m_sfx.queue(q, Sfx::BossDown);
        app.hitStop(0.25f);
        m_music.stop(app.audio(), 1.0f);
        break;
    case EventKind::EnemyShot: m_sfx.queue(q, e.value < 0 ? Sfx::Shot : Sfx::Lunge, e.value < 0 ? 0.8f : 0.5f); break;
    case EventKind::RainWarn: m_sfx.queue(q, Sfx::RainWarn); break;
    case EventKind::Sleep:
        m_sfx.queue(q, Sfx::Sleep);
        m_hud.toast(e.value == 0 ? "a new morning in hearthvale" : "the stars come out", srgb(0xFFE08A));
        m_saves.save(m_slot, m_session->save());
        break;
    case EventKind::ChapterUp:
        m_sfx.queue(q, Sfx::Chapter);
        m_hud.toast(std::format("chapter {} - return to hearthvale", e.value), srgb(0xFFE08A));
        break;
    case EventKind::Spawned:
        if (e.extra < 100) {
            m_sfx.queue(q, Sfx::Spawn, 0.4f);
        }
        break;
    case EventKind::Equip: m_sfx.queue(q, Sfx::Equip); break;
    case EventKind::Upgrade: m_sfx.queue(q, Sfx::Upgrade); break;
    case EventKind::Bought: m_sfx.queue(q, Sfx::Buy); break;
    case EventKind::Sold: m_sfx.queue(q, Sfx::Sell); break;
    case EventKind::Full: m_sfx.queue(q, Sfx::Error); break;
    case EventKind::Roll:
    case EventKind::Climb:
    case EventKind::Mana:
    case EventKind::QuestUpdate:
    case EventKind::Travel:
    case EventKind::Ending:
    case EventKind::Saved:
    case EventKind::Shockwave: break;
    }
}

void WayfarerGame::updateMusic(gamelib::app::App& app)
{
    if (m_mode == Mode::Title || m_mode == Mode::Slots || m_mode == Mode::Sheet) {
        m_music.play(app.audio(), Tune::Title, 0.8f);
        return;
    }
    if (m_mode == Mode::Ending) {
        m_music.play(app.audio(), Tune::Ending, 1.5f);
        return;
    }
    if (!m_session) {
        return;
    }
    const game::Session& s = *m_session;
    if (s.boss()) {
        if (s.boss()->alive()) {
            m_music.play(app.audio(), s.boss()->id == game::BossId::Hush ? Tune::FinalBoss : Tune::Boss, 0.6f);
        }
        return;
    }
    if (s.phase() == game::Phase::Dead) {
        return;
    }
    const game::MapDef& def = s.map();
    const bool nightTake = !def.dungeon && def.region != game::Region::Mines && s.nightAmount() > 0.6f;
    m_music.play(app.audio(), regionTune(def.region, def.dungeon), 2.5f, nightTake ? 1 : 0);
}

void WayfarerGame::frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info)
{
    m_time = info.time;
    m_seconds = info.seconds;
    m_transition.update(info.seconds);
    m_windows.update(info.seconds);
    if (m_mode == Mode::Ending) {
        m_endingTime += info.seconds;
    }
    if (m_mode == Mode::Play && m_session) {
        game::Session& s = *m_session;
        const bool frozen = !m_menus.is(ArcadeScreen::Playing) || m_windows.isOpen() || m_dialogue.isOpen() || s.travelPending();
        m_alpha = frozen ? 1.0f : info.alpha;
        m_view.follow(s, m_alpha, info.seconds, m_time);
        m_effects.update(info.seconds, s, m_view.visible(), m_view.heroFeet(s, m_alpha));
        m_hud.update(info.seconds, s);
        m_dialogue.update(info.seconds);
        m_typed += m_dialogue.typedThisFrame();
        if (m_typed >= 3) {
            m_typed = 0;
            m_sfx.queue(app.sounds(), Sfx::Type, 0.5f);
        }
        // Fainted: a moment, then wake up at the inn.
        if (s.phase() == game::Phase::Dead) {
            m_deadTime += info.seconds;
            if (m_deadTime > kFaintSeconds && !m_transition.active()) {
                m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = srgb(0xFFF4E0), .coverSeconds = 0.8f,
                                    .holdSeconds = 0.4f, .uncoverSeconds = 0.8f},
                                   [this] {
                                       m_session->respawn();
                                       (void)m_session->events().take();
                                       m_view.snap(*m_session);
                                       m_effects.clear();
                                       m_effects.setRegion(m_session->map().region, m_session->map().dungeon);
                                       m_hud.reset(*m_session);
                                       m_hud.mapTitle(*m_session);
                                       m_hud.toast("you wake at the sleepy kettle inn", srgb(0xFFE08A));
                                       m_deadTime = 0.0f;
                                   });
            }
        }
    } else {
        const game::Session& demo = m_attract.sim().session;
        if (demo.ticks() <= 1) {
            m_demoView.snap(demo);
            m_demoEffects.clear();
        }
        m_demoView.follow(demo, info.alpha, info.seconds, m_time);
        m_demoEffects.update(info.seconds, demo, m_demoView.visible(), m_demoView.heroFeet(demo, info.alpha));
    }
    updateMusic(app);
}

} // namespace wayfarer::app
