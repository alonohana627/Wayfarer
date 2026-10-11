#pragma once

// One playthrough from a save: the hero (GameLib's platformer controller
// tuned by Speed and Jump, a three-hit combo with frame data, the hyper jump,
// the dodge roll and four hotkey skills), the current map with its monsters,
// spawns, drops and shots, a guardian in its arena, the town's NPCs by day
// and night, the clock, the quests and the story flags. Deterministic: the
// same save and per-tick input give the same run. No window, audio or clock.
//
// The app drives it: tick() with the input, events() for sounds and
// effects, and commands from its windows (spend a point, learn a skill,
// equip, buy, upgrade) and from dialogue (conditions and effects).

#include "bosses.hpp"
#include "character.hpp"
#include "maps.hpp"
#include "monsters.hpp"
#include "rules.hpp"
#include "story.hpp"

#include <gamelib/core/combat.hpp>
#include <gamelib/core/event_queue.hpp>
#include <gamelib/core/game_clock.hpp>
#include <gamelib/core/items.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/core/platformer.hpp>
#include <gamelib/core/projectiles.hpp>
#include <gamelib/core/quests.hpp>
#include <gamelib/core/rng.hpp>
#include <gamelib/core/spawner.hpp>
#include <gamelib/core/telegraphs.hpp>
#include <gamelib/core/world_flags.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace wayfarer::game {

inline constexpr std::int64_t kTicksPerDay = 14LL * 60 * kTickRate; // a day is fourteen minutes
inline constexpr int kRespawnTicks = 150;

enum class EventKind : std::uint8_t {
    Jump, HyperJump, Land, Climb, Swing, Roll, Hit, Guarded, MonsterDown, Hurt, Heal, Mana, Pickup, Coins, LevelUp,
    SkillCast, SkillLearned, NoMana, Cooldown, Potion, QuestUpdate, QuestAccepted, QuestComplete, QuestDone, Travel,
    PortalClosed, Death, BossWake, BossPhase, BossRoar, BossDown, BossLand, EnemyShot, RainWarn, Shockwave, Saved, Sleep,
    ChapterUp, Ending, Spawned, Equip, Upgrade, Bought, Sold, Full,
};

struct Event {
    EventKind kind = EventKind::Jump;
    int value = 0;   // damage, amount, level, skill, quest, item, map...
    int extra = 0;   // rarity, combo step, objective progress...
    gamelib::core::Vec2 at;
    bool crit = false;
    bool onHero = false;
};

enum class Phase : std::uint8_t { Playing, Dead, Ending };

// A hero shot (Starbolt) or a guardian's.
struct ShotData {
    bool hero = false;
    ShotLook look = ShotLook::Star;
    int hitsLeft = 1;
    bool groundWave = false;
};
using Shots = gamelib::core::Projectiles<ShotData>;

struct Drop {
    gamelib::core::ItemStack stack; // empty for coins
    std::int64_t coins = 0;
    gamelib::core::Rect box;
    gamelib::core::Rect previous;
    gamelib::core::Vec2 velocity;
    int age = 0;
    bool grounded = false;
};

struct Hero {
    gamelib::core::PlatformerBody body;
    gamelib::core::Rect previous;
    gamelib::core::Attacker combo{1, 1'000'000};
    gamelib::core::Attacker skill{1, 2'000'000};
    gamelib::core::Vulnerable vulnerable;
    int rollTicks = 0;       // > 0 while rolling
    int rollLength = 1;
    int castTicks = 0;       // > 0 while casting a skill (no movement)
    int castLength = 1;
    SkillId casting = SkillId::Combo;
    int hurtTicks = 0;       // knocked back, no control
    int hyperTicks = 0;      // > 0 just after a hyper jump (trail, no air control)
    bool hyperUp = false;
    int landTicks = 0;       // since landing (squash)
    int airTicks = 0;
    int idleTicks = 0;
    bool alive = true;
};

struct NpcHere {
    NpcId npc = NpcId::Elder;
    gamelib::core::Vec2 feet;
    int facing = 1;
};

struct RunStats {
    std::int64_t ticks = 0;
    int defeated = 0;
    int elites = 0;
    int deaths = 0;
    std::int64_t coinsEarned = 0;
    int potions = 0;
    int bestHit = 0;
};

// Everything a save keeps.
struct SaveData {
    Character character;
    std::vector<gamelib::core::QuestRecord> quests;
    gamelib::core::WorldFlags flags;
    MapId map = MapId::Town;
    gamelib::core::Vec2 feet;
    std::int64_t clock = 0;
    RunStats stats;
    bool finished = false;
};

class Session {
public:
    // A new game, or a loaded one.
    explicit Session(std::uint64_t seed = 1);
    Session(const SaveData& save, std::uint64_t seed);

    void tick(const Input& input);

    // ---- state
    [[nodiscard]] Phase phase() const { return m_phase; }
    [[nodiscard]] const Hero& hero() const { return m_hero; }
    [[nodiscard]] const Character& character() const { return m_char; }
    [[nodiscard]] const Derived& derived() const { return m_derived; }
    [[nodiscard]] MapId mapId() const { return m_map; }
    [[nodiscard]] const MapDef& map() const { return mapDef(m_map); }
    [[nodiscard]] const std::vector<Monster>& monsters() const { return m_monsters; }
    [[nodiscard]] const std::optional<Boss>& boss() const { return m_boss; }
    [[nodiscard]] const std::vector<Drop>& drops() const { return m_drops; }
    [[nodiscard]] const Shots& shots() const { return m_shots; }
    [[nodiscard]] const gamelib::core::Telegraphs<float>& rain() const { return m_rain; }
    [[nodiscard]] const std::vector<NpcHere>& npcs() const { return m_npcs; }
    [[nodiscard]] const gamelib::core::GameClock& clock() const { return m_clock; }
    [[nodiscard]] const gamelib::core::QuestBook& quests() const { return m_quests; }
    [[nodiscard]] const gamelib::core::WorldFlags& flags() const { return m_flags; }
    [[nodiscard]] const RunStats& stats() const { return m_stats; }
    [[nodiscard]] std::int64_t ticks() const { return m_ticks; }
    [[nodiscard]] gamelib::core::EventQueue<Event>& events() { return m_events; }
    [[nodiscard]] int chapter() const; // 1..6: guardians calmed + 1
    [[nodiscard]] bool night() const;
    // 0 day .. 1 deep night, eased through dusk and dawn.
    [[nodiscard]] float nightAmount() const;
    [[nodiscard]] bool waystoneOpen(int region) const;
    [[nodiscard]] bool portalOpen(int index) const;
    [[nodiscard]] bool bossBeaten(BossId boss) const;
    [[nodiscard]] bool finished() const { return m_finished; }

    // What the hero could interact with now (for prompts): an NPC, a portal, the inn.
    [[nodiscard]] std::optional<NpcId> npcInReach() const;
    [[nodiscard]] int portalInReach() const; // -1: none
    [[nodiscard]] bool innInReach() const;
    // Set by a tick when the hero pressed up at an NPC or the inn (the app opens the conversation).
    [[nodiscard]] std::optional<std::string> takeTalkRequest();
    // A map change waits for the app's transition: travel() does it.
    [[nodiscard]] bool travelPending() const { return m_travel.has_value(); }
    [[nodiscard]] MapId travelTarget() const { return m_travel ? m_travel->first : m_map; }
    void travel();
    [[nodiscard]] bool wantsSave();
    // After death: back in town.
    void respawn();

    // ---- dialogue
    void startTalk(NpcId npc); // quest talk/deliver events before the conversation
    [[nodiscard]] bool condition(std::string_view condition) const;
    void effect(std::string_view effect);
    [[nodiscard]] std::optional<std::string> takeServiceRequest(); // "shop", "smith"

    // ---- windows
    bool spendPoint(Core stat);
    int autoAssign();
    gamelib::core::LearnResult learnSkill(SkillId skill);
    [[nodiscard]] bool canLearn(SkillId skill) const;
    bool equipFromBag(int bagSlot);
    bool unequip(Slot slot);
    bool useFromBag(int bagSlot);
    bool moveInBag(int from, int to);
    bool assignQuick(int bagSlot);
    [[nodiscard]] std::vector<gamelib::core::ShopEntry> shopEntries() const;
    gamelib::core::TradeResult buy(int entry, int count);
    bool sellFromBag(int bagSlot);
    // The blacksmith: slot < 0 means a bag slot (-1 - bag index), else an equipment slot.
    [[nodiscard]] std::optional<UpgradeCost> upgradeCostOf(int target) const;
    bool upgrade(int target);
    [[nodiscard]] const gamelib::core::ItemStack& upgradeTarget(int target) const;

    // ---- saves
    [[nodiscard]] SaveData save() const;

    // ---- developer aids
    void debugTeleport(MapId map, int portal);
    void debugLevel(int level);
    void debugGrantChapter(int chapter);
    void debugBossLow();
    void debugTime(int hour);
    void debugGear();
    void debugMove(float x);
    void debugLearnAll();

private:
    void enterMap(MapId map, std::optional<gamelib::core::Vec2> feet, int facing);
    void spawnInto(int point, bool immediate);
    void tickHero(const Input& input);
    void tickMonsters();
    void tickBoss();
    void tickShots();
    void tickDrops();
    void tickSpawns();
    void placeNpcs();
    void heroHits(const gamelib::core::Hit& hit, int percent, bool magic, bool combo);
    void damageHero(int damage, float fromX);
    void defeatMonster(Monster& m);
    void defeatBoss();
    void dropLoot(const Monster& m);
    void spawnDrop(gamelib::core::ItemStack stack, std::int64_t coins, gamelib::core::Vec2 at, int index);
    void collect(Drop& drop);
    void addItem(const gamelib::core::ItemStack& stack);
    void refreshCollectQuests();
    void applyReward(const gamelib::core::QuestReward& reward);
    void gainXp(std::int64_t xp, gamelib::core::Vec2 at);
    void useSkill(SkillId skill);
    void drinkQuick(int which);
    void refreshDerived();
    [[nodiscard]] int damageRoll(int percent, bool magic, int defense, bool& crit);
    [[nodiscard]] gamelib::core::TileWorld world() const;
    void push(const Event& e) { m_events.push(e); }

    gamelib::core::Rng m_rng;
    Phase m_phase = Phase::Playing;
    Character m_char;
    Derived m_derived;
    Hero m_hero;
    MapId m_map = MapId::Town;
    std::vector<Monster> m_monsters;
    gamelib::core::Spawner m_spawner; // a point per monster (SpawnPoint::kind = the map's spawn index)
    std::optional<Boss> m_boss;
    std::vector<Drop> m_drops;
    Shots m_shots{96};
    gamelib::core::Telegraphs<float> m_rain;
    std::vector<NpcHere> m_npcs;
    gamelib::core::GameClock m_clock{kTicksPerDay};
    gamelib::core::QuestBook m_quests;
    gamelib::core::WorldFlags m_flags;
    gamelib::core::EventQueue<Event> m_events;
    RunStats m_stats;
    std::int64_t m_ticks = 0;
    std::optional<std::pair<MapId, int>> m_travel;
    std::optional<std::string> m_talk;
    std::optional<std::string> m_service;
    bool m_saveWanted = false;
    bool m_finished = false;
    int m_deadTicks = 0;
    int m_endingTicks = 0;
    int m_nightCheck = -1;
    int m_mapTicks = 0;
};

} // namespace wayfarer::game
