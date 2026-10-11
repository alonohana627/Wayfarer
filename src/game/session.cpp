#include "session.hpp"

#include <gamelib/core/collision.hpp>
#include <gamelib/core/tile_collision.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <string>

namespace wayfarer::game {

namespace {

// helpers

using gamelib::core::AttackClip;
using gamelib::core::ItemStack;
using gamelib::core::ObjectiveKind;
using gamelib::core::QuestEvent;
using gamelib::core::QuestState;
using gamelib::core::Rect;
using gamelib::core::Vec2;

constexpr int kRespawnDelay = 7 * kTickRate;
constexpr int kHurtStun = 18;
constexpr int kHurtInvincible = 100;
constexpr float kRollSpeed = 9.5f;
constexpr float kHyperForward = 12.0f;
constexpr float kHyperRise = 5.0f;
constexpr float kHyperUp = 15.0f;
constexpr int kHyperLock = 12;
constexpr int kDropLifetime = 90 * kTickRate;
constexpr Vec2 kTalkReach{90.0f, 110.0f};
constexpr float kInnDoorX = 47.8f * kTile; // the inn door in Hearthvale (maps_town.cpp)

int parseInt(std::string_view text)
{
    int value = 0;
    while (!text.empty() && text.front() == ' ') {
        text.remove_prefix(1);
    }
    (void)std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

// The combo's three clips for a weapon: faster with a dagger, heavier with an axe.
std::array<AttackClip, 3> comboClips(const WeaponKind weapon)
{
    const int quick = weapon == WeaponKind::Dagger ? -2 : weapon == WeaponKind::Axe ? 2 : 0;
    const float reach = weapon == WeaponKind::Axe ? 1.12f : weapon == WeaponKind::Dagger ? 0.85f : 1.0f;
    std::array<AttackClip, 3> c;
    c[0] = {.startup = 5 + quick / 2, .active = 4, .recovery = 11 + quick, .hitbox = {-4.0f, -66.0f, 104.0f * reach, 104.0f},
            .damage = 100, .knockback = {3.0f, -2.5f}, .stunTicks = 14, .hitStopTicks = 3, .chainWindow = 13,
            .cancelFrom = 9, .id = 0};
    c[1] = {.startup = 4 + quick / 2, .active = 4, .recovery = 11 + quick, .hitbox = {-4.0f, -74.0f, 110.0f * reach, 114.0f},
            .damage = 115, .knockback = {3.5f, -3.0f}, .stunTicks = 14, .hitStopTicks = 3, .chainWindow = 13,
            .cancelFrom = 8, .id = 1};
    c[2] = {.startup = 7 + quick, .active = 6, .recovery = 18 + quick * 2,
            .hitbox = {-14.0f, -96.0f, 128.0f * reach, 136.0f}, .damage = 170, .knockback = {7.0f, -5.5f}, .stunTicks = 24,
            .hitStopTicks = 6, .chainWindow = 0, .cancelFrom = 13, .id = 2};
    return c;
}

AttackClip sweepClip(const int percent)
{
    return {.startup = 8, .active = 16, .recovery = 14, .hitbox = {-170.0f, -120.0f, 340.0f, 170.0f}, .damage = percent,
            .knockback = {5.5f, -5.0f}, .stunTicks = 22, .hitStopTicks = 4, .cancelFrom = -1, .rehitTicks = 8, .id = 10};
}

bool overlapsRect(const Rect& a, const Rect& b)
{
    return gamelib::core::overlaps(a, b);
}

} // namespace

// ---------------------------------------------------------------------------
// Construction and saves

Session::Session(const std::uint64_t seed)
    : m_rng{seed}
{
    for (const auto& q : questDefs()) {
        m_quests.define(q);
    }
    m_quests.refresh(1, [](int) { return false; });
    // Starting gear.
    m_char.gear.setAt(static_cast<int>(Slot::Weapon), {.item = kStarterSword, .count = 1});
    m_char.gear.setAt(static_cast<int>(Slot::Armor), {.item = kStarterTunic, .count = 1});
    m_char.gear.setAt(static_cast<int>(Slot::Shoes), {.item = kStarterShoes, .count = 1});
    m_char.bag.add({.item = kRedPotion, .count = 10});
    m_char.bag.add({.item = kBluePotion, .count = 5});
    m_char.coins = 120;
    m_char.refreshResources();
    m_char.hp.refill();
    m_char.mp.refill();
    m_clock.setTime(9, 0);
    refreshDerived();
    m_hero.body.box = Rect::fromBottomCenter(feetOn(28.0f, 21), kHeroSize);
    m_hero.body.facing = 1;
    m_hero.vulnerable = gamelib::core::Vulnerable{m_hero.body.box, 1};
    enterMap(MapId::Town, feetOn(28.0f, 21), 1);
    m_saveWanted = false;
}

Session::Session(const SaveData& save, const std::uint64_t seed)
    : m_rng{seed}
{
    for (const auto& q : questDefs()) {
        m_quests.define(q);
    }
    m_char = save.character;
    m_quests.load(save.quests);
    m_flags = save.flags;
    m_clock.setElapsed(save.clock);
    m_stats = save.stats;
    m_finished = save.finished;
    m_quests.refresh(m_char.level, [](int) { return false; });
    m_char.refreshResources();
    refreshDerived();
    m_hero.vulnerable = gamelib::core::Vulnerable{m_hero.body.box, 1};
    MapId map = save.map;
    // Never wake inside an arena: start at its door.
    std::optional<Vec2> feet = save.feet;
    if (mapDef(map).arena && !mapDef(map).portals.empty()) {
        const Portal back = mapDef(map).portals.front();
        feet = back.at + Vec2{kTile * 2.0f, 0.0f};
        if (!bossBeaten(mapDef(map).arena->boss)) {
            map = back.to;
            feet = mapDef(map).portals[static_cast<std::size_t>(back.toPortal)].at;
        }
    }
    enterMap(map, feet, 1);
    m_saveWanted = false;
}

SaveData Session::save() const
{
    SaveData s;
    s.character = m_char;
    s.quests = m_quests.save();
    s.flags = m_flags;
    s.map = m_map;
    s.feet = m_hero.body.box.bottomCenter();
    s.clock = m_clock.elapsed();
    s.stats = m_stats;
    s.finished = m_finished;
    return s;
}

gamelib::core::TileWorld Session::world() const
{
    gamelib::core::TileWorld w{map().tiles, 0, tileShapes()};
    w.setSideWalls(true);
    return w;
}

void Session::refreshDerived()
{
    m_derived = m_char.derived();
}

// ---------------------------------------------------------------------------
// Story state

int Session::chapter() const
{
    int beaten = 0;
    for (int b = 0; b < kBossCount; ++b) {
        beaten += bossBeaten(static_cast<BossId>(b)) ? 1 : 0;
    }
    return beaten + 1;
}

bool Session::bossBeaten(const BossId boss) const
{
    return m_flags.has("boss" + std::to_string(static_cast<int>(boss)));
}

bool Session::night() const
{
    const int h = m_clock.hour();
    return h >= 19 || h < 6;
}

float Session::nightAmount() const
{
    const float hour = static_cast<float>(m_clock.minuteOfDay()) / 60.0f;
    // Dusk 17:30-20:00, dawn 4:30-6:30.
    if (hour >= 17.5f && hour < 20.0f) {
        return (hour - 17.5f) / 2.5f;
    }
    if (hour >= 20.0f || hour < 4.5f) {
        return 1.0f;
    }
    if (hour >= 4.5f && hour < 6.5f) {
        return 1.0f - (hour - 4.5f) / 2.0f;
    }
    return 0.0f;
}

bool Session::waystoneOpen(const int region) const
{
    if (region == 1) {
        return m_quests.stateOf(kQuestArrival) == QuestState::TurnedIn;
    }
    const QuestState s = m_quests.stateOf(kQuestChapter1 + region - 1);
    return s == QuestState::Active || s == QuestState::Complete || s == QuestState::TurnedIn;
}

bool Session::portalOpen(const int index) const
{
    const MapDef& m = map();
    if (index < 0 || index >= static_cast<int>(m.portals.size())) {
        return false;
    }
    const Portal& p = m.portals[static_cast<std::size_t>(index)];
    if (p.kind == PortalKind::Waystone) {
        return waystoneOpen(p.chapter);
    }
    if (p.afterBoss) {
        return m.arena && bossBeaten(m.arena->boss);
    }
    // An arena's way back is shut while its guardian fights.
    if (m.arena && m_boss && m_boss->alive() && m_boss->state != BossState::Intro) {
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Maps

void Session::enterMap(const MapId id, const std::optional<Vec2> feet, const int facing)
{
    m_map = id;
    m_mapTicks = 0;
    m_monsters.clear();
    m_drops.clear();
    m_shots.clear();
    m_rain.clear();
    m_boss.reset();
    m_slots.clear();
    const MapDef& def = map();
    for (int s = 0; s < static_cast<int>(def.spawns.size()); ++s) {
        for (int i = 0; i < def.spawns[static_cast<std::size_t>(s)].count; ++i) {
            m_slots.push_back({.def = s});
        }
    }
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i) {
        spawnInto(i, true);
    }
    if (def.arena && !bossBeaten(def.arena->boss)) {
        m_boss = makeBoss(def.arena->boss, def.arena->bossFeet, def.arena->bounds);
        push({.kind = EventKind::BossWake, .value = static_cast<int>(def.arena->boss)});
    }
    const Vec2 at = feet.value_or(def.portals.empty() ? Vec2{tx(3.0f), def.bounds().bottom() * 0.5f} : def.portals.front().at);
    m_hero.body.box = Rect::fromBottomCenter(at, kHeroSize);
    m_hero.body.velocity = {};
    m_hero.body.facing = facing;
    m_hero.body.climbing = false;
    m_hero.body.grounded = false;
    m_hero.previous = m_hero.body.box;
    m_hero.combo.cancel();
    m_hero.skill.cancel();
    m_hero.rollTicks = 0;
    m_hero.castTicks = 0;
    m_hero.hurtTicks = 0;
    m_nightCheck = -1;
    placeNpcs();
    push({.kind = EventKind::Travel, .value = static_cast<int>(id)});
    if (id == MapId::Town) {
        m_saveWanted = true;
    }
}

void Session::spawnInto(const int slotIndex, const bool immediate)
{
    SpawnSlot& slot = m_slots[static_cast<std::size_t>(slotIndex)];
    const SpawnDef& def = map().spawns[static_cast<std::size_t>(slot.def)];
    const Species& s = species(def.species);
    const float left = def.run.left();
    const float right = def.run.right();
    const float span = std::max(right - left - s.size.x, 1.0f);
    const float x = left + s.size.x * 0.5f + span * m_rng.unit();
    const float floorY = def.run.bottom();
    const bool elite = m_rng.range(0, 99) < map().eliteChance;
    const int level = s.level + m_rng.range(0, 1);
    Vec2 feet{x, floorY};
    if (s.behavior == Behavior::Flyer) {
        feet.y -= 70.0f + m_rng.unit() * 90.0f;
    }
    Monster m = makeMonster(def.species, level, elite, feet, left, right, static_cast<std::uint8_t>(m_rng.range(0, 255)));
    m.spawn = slotIndex;
    if (immediate) {
        m.stateTicks = m.stateLength - 1; // already standing there when the map opens
    }
    m_monsters.push_back(m);
    slot.occupied = true;
    slot.timer = 0;
    if (!immediate) {
        push({.kind = EventKind::Spawned, .at = m.box.bottomCenter()});
    }
}

void Session::placeNpcs()
{
    m_npcs.clear();
    const bool isNight = night();
    for (const NpcPlace& n : map().npcs) {
        const Vec2 at = isNight ? n.night : n.day;
        if (at.x < 0.0f) {
            continue;
        }
        m_npcs.push_back({.npc = n.npc, .feet = at, .facing = at.x < map().bounds().width * 0.5f ? 1 : -1});
    }
    m_nightCheck = isNight ? 1 : 0;
}

void Session::travel()
{
    if (!m_travel) {
        return;
    }
    const auto [to, portal] = *m_travel;
    m_travel.reset();
    const MapDef& target = mapDef(to);
    Vec2 feet = target.portals.empty() ? Vec2{tx(4.0f), target.bounds().bottom() * 0.5f}
                                       : target.portals[static_cast<std::size_t>(std::clamp(portal, 0, static_cast<int>(target.portals.size()) - 1))].at;
    int facing = 1;
    if (!target.portals.empty()) {
        const Portal& p = target.portals[static_cast<std::size_t>(std::clamp(portal, 0, static_cast<int>(target.portals.size()) - 1))];
        if (p.kind == PortalKind::Edge) {
            const bool left = p.at.x < target.bounds().width * 0.5f;
            feet.x += left ? kTile * 1.2f : -kTile * 1.2f;
            facing = left ? 1 : -1;
        } else {
            facing = p.at.x < target.bounds().width * 0.5f ? 1 : -1;
        }
    }
    enterMap(to, feet, facing);
}

bool Session::wantsSave()
{
    return std::exchange(m_saveWanted, false);
}

void Session::respawn()
{
    m_phase = Phase::Playing;
    m_hero.alive = true;
    m_char.refreshResources();
    m_char.hp.refill();
    m_char.mp.current = std::max(m_char.mp.current, m_char.mp.max / 2);
    m_char.coins -= m_char.coins / 10;
    m_hero.vulnerable.makeInvincible(120);
    enterMap(MapId::Town, Vec2{kInnDoorX + kTile * 2.0f, 21.0f * kTile}, 1);
}

// ---------------------------------------------------------------------------
// Interaction

std::optional<NpcId> Session::npcInReach() const
{
    const Vec2 feet = m_hero.body.box.bottomCenter();
    std::optional<NpcId> best;
    float bestDist = 1e9f;
    for (const NpcHere& n : m_npcs) {
        const float dx = std::abs(n.feet.x - feet.x);
        const float dy = std::abs(n.feet.y - feet.y);
        if (dx < kTalkReach.x && dy < kTalkReach.y && dx < bestDist) {
            best = n.npc;
            bestDist = dx;
        }
    }
    return best;
}

int Session::portalInReach() const
{
    const Vec2 feet = m_hero.body.box.bottomCenter();
    const MapDef& m = map();
    for (int i = 0; i < static_cast<int>(m.portals.size()); ++i) {
        const Portal& p = m.portals[static_cast<std::size_t>(i)];
        if (p.kind == PortalKind::Edge) {
            continue;
        }
        if (std::abs(p.at.x - feet.x) < 56.0f && std::abs(p.at.y - feet.y) < 80.0f) {
            return i;
        }
    }
    return -1;
}

bool Session::innInReach() const
{
    const Vec2 feet = m_hero.body.box.bottomCenter();
    return m_map == MapId::Town && std::abs(feet.x - kInnDoorX) < 60.0f && std::abs(feet.y - 21.0f * kTile) < 60.0f;
}

std::optional<std::string> Session::takeTalkRequest()
{
    return std::exchange(m_talk, std::nullopt);
}

std::optional<std::string> Session::takeServiceRequest()
{
    return std::exchange(m_service, std::nullopt);
}

void Session::startTalk(const NpcId npc)
{
    for (const auto& u : m_quests.onEvent({.kind = ObjectiveKind::Talk, .subject = static_cast<int>(npc)})) {
        push({.kind = EventKind::QuestUpdate, .value = u.quest, .extra = u.progress});
    }
    // Deliveries: the letter goes to Brann.
    if (npc == NpcId::Brann && m_quests.stateOf(kQuestLetter) == QuestState::Active && m_char.bag.countOf(kElderLetter) > 0) {
        m_char.bag.remove(kElderLetter, 1);
        (void)m_quests.onEvent({.kind = ObjectiveKind::Deliver, .subject = static_cast<int>(NpcId::Brann)});
    }
}

bool Session::condition(const std::string_view c) const
{
    if (c.empty()) {
        return true;
    }
    const auto space = c.find(' ');
    const std::string_view verb = c.substr(0, space);
    const int n = space == std::string_view::npos ? 0 : parseInt(c.substr(space + 1));
    if (verb == "available") {
        return m_quests.stateOf(n) == QuestState::Available;
    }
    if (verb == "active") {
        return m_quests.stateOf(n) == QuestState::Active;
    }
    if (verb == "complete") {
        return m_quests.stateOf(n) == QuestState::Complete;
    }
    if (verb == "done") {
        return m_quests.stateOf(n) == QuestState::TurnedIn;
    }
    if (verb == "chapter") {
        return chapter() == n;
    }
    if (verb == "night") {
        return night();
    }
    if (verb == "day") {
        return !night();
    }
    if (verb == "has") {
        return m_char.bag.countOf(n) > 0;
    }
    return false;
}

void Session::effect(const std::string_view e)
{
    const auto space = e.find(' ');
    const std::string_view verb = e.substr(0, space);
    const std::string_view arg = space == std::string_view::npos ? std::string_view{} : e.substr(space + 1);
    const int n = parseInt(arg);
    if (verb == "accept") {
        if (m_quests.accept(n)) {
            push({.kind = EventKind::QuestAccepted, .value = n});
            if (n == kQuestLetter) {
                addItem({.item = kElderLetter, .count = 1});
            }
            refreshCollectQuests();
        }
    } else if (verb == "turnin") {
        // Collect quests take their items.
        if (const gamelib::core::QuestDef* def = m_quests.find(n)) {
            if (m_quests.stateOf(n) == QuestState::Complete) {
                for (const auto& o : def->objectives) {
                    if (o.kind == ObjectiveKind::Collect) {
                        m_char.bag.remove(o.subject, o.required);
                    }
                }
            }
        }
        if (const std::optional<gamelib::core::QuestReward> reward = m_quests.turnIn(n)) {
            push({.kind = EventKind::QuestDone, .value = n});
            if (n == kQuestArrival) {
                m_char.skills.setLevel(static_cast<int>(SkillId::HyperJump), 1);
                push({.kind = EventKind::SkillLearned, .value = static_cast<int>(SkillId::HyperJump)});
            }
            applyReward(*reward);
            m_quests.refresh(m_char.level, [](int) { return false; });
            refreshCollectQuests();
        }
    } else if (verb == "shop" || verb == "smith") {
        m_service = std::string{verb};
    } else if (verb == "sleep") {
        const bool morning = arg == "morning";
        m_clock.advanceTo(m_clock.timeOf(morning ? 7 : 20, 0));
        m_char.hp.refill();
        m_char.mp.refill();
        placeNpcs();
        m_saveWanted = true;
        push({.kind = EventKind::Sleep, .value = morning ? 0 : 1});
    }
}

void Session::applyReward(const gamelib::core::QuestReward& reward)
{
    m_char.coins += reward.coins;
    m_stats.coinsEarned += reward.coins;
    if (reward.coins > 0) {
        push({.kind = EventKind::Coins, .value = static_cast<int>(reward.coins), .onHero = true});
    }
    for (const auto& item : reward.items) {
        const ItemInfo& info = itemInfo(item.item);
        ItemStack stack{.item = item.item, .count = item.count};
        if (info.kind == ItemKind::Equip) {
            stack.rarity = 1; // quest gear is at least rare
        }
        addItem(stack);
    }
    gainXp(reward.experience, m_hero.body.box.center());
}

void Session::addItem(const ItemStack& stack)
{
    const int left = m_char.bag.add(stack);
    if (left < stack.count) {
        push({.kind = EventKind::Pickup, .value = stack.item, .extra = stack.rarity, .at = m_hero.body.box.center(),
              .onHero = true});
    }
    if (left > 0) {
        // No room: it falls at the hero's feet.
        ItemStack rest = stack;
        rest.count = left;
        spawnDrop(rest, 0, m_hero.body.box.center(), 0);
        push({.kind = EventKind::Full});
    }
    refreshCollectQuests();
}

void Session::refreshCollectQuests()
{
    for (const auto& def : questDefs()) {
        for (const auto& o : def.objectives) {
            if (o.kind == ObjectiveKind::Collect) {
                for (const auto& u : m_quests.setCount(ObjectiveKind::Collect, o.subject, m_char.bag.countOf(o.subject))) {
                    if (u.questComplete) {
                        push({.kind = EventKind::QuestComplete, .value = u.quest});
                    } else {
                        push({.kind = EventKind::QuestUpdate, .value = u.quest, .extra = u.progress});
                    }
                }
            }
        }
    }
}

void Session::gainXp(const std::int64_t xp, const Vec2 at)
{
    if (xp <= 0) {
        return;
    }
    const gamelib::core::LevelUp up = m_char.gainXp(xp);
    if (up.happened()) {
        push({.kind = EventKind::LevelUp, .value = up.to, .at = at, .onHero = true});
        m_quests.refresh(m_char.level, [](int) { return false; });
        refreshDerived();
    }
}

// ---------------------------------------------------------------------------
// Windows

bool Session::spendPoint(const Core stat)
{
    const bool ok = m_char.spendPoint(stat);
    refreshDerived();
    return ok;
}

int Session::autoAssign()
{
    const int spent = m_char.autoAssign();
    refreshDerived();
    return spent;
}

bool Session::canLearn(const SkillId skill) const
{
    const int id = static_cast<int>(skill);
    if (skill == SkillId::HyperJump && m_char.skills.levelOf(id) == 0) {
        return false; // taught by Captain Dorran
    }
    return m_char.skills.points() > 0 && m_char.skills.levelOf(id) < m_char.skills.maxLevelOf(id)
           && m_char.level >= skillUnlockLevel(skill);
}

gamelib::core::LearnResult Session::learnSkill(const SkillId skill)
{
    if (skill == SkillId::HyperJump && m_char.skills.levelOf(static_cast<int>(skill)) == 0) {
        return gamelib::core::LearnResult::MissingPrerequisite;
    }
    const gamelib::core::LearnResult r = m_char.skills.learn(static_cast<int>(skill), m_char.level);
    if (r == gamelib::core::LearnResult::Learned) {
        push({.kind = EventKind::SkillLearned, .value = static_cast<int>(skill)});
    }
    return r;
}

bool Session::equipFromBag(const int bagSlot)
{
    const gamelib::core::EquipResult r = m_char.gear.equipFrom(m_char.bag, bagSlot, m_char.wearer());
    if (r == gamelib::core::EquipResult::Done) {
        m_char.refreshResources();
        refreshDerived();
        push({.kind = EventKind::Equip});
        return true;
    }
    return false;
}

bool Session::unequip(const Slot slot)
{
    if (slot == Slot::Weapon) {
        return false; // the hero always carries a weapon
    }
    const gamelib::core::EquipResult r = m_char.gear.unequipTo(m_char.bag, static_cast<int>(slot));
    if (r == gamelib::core::EquipResult::Done) {
        m_char.refreshResources();
        refreshDerived();
        push({.kind = EventKind::Equip});
        return true;
    }
    if (r == gamelib::core::EquipResult::InventoryFull) {
        push({.kind = EventKind::Full});
    }
    return false;
}

bool Session::useFromBag(const int bagSlot)
{
    const ItemStack& stack = m_char.bag.at(bagSlot);
    if (stack.empty()) {
        return false;
    }
    const ItemInfo& info = itemInfo(stack.item);
    if (info.kind == ItemKind::Equip) {
        return equipFromBag(bagSlot);
    }
    if (info.kind != ItemKind::Use) {
        return false;
    }
    if (stack.item == kReturnScroll) {
        if (m_map == MapId::Town || m_boss) {
            return false;
        }
        (void)m_char.bag.takeAt(bagSlot, 1);
        m_travel = {MapId::Town, 0};
        // Arrive at the inn rather than a waystone.
        push({.kind = EventKind::Potion, .value = stack.item});
        return true;
    }
    if ((info.heal > 0 && m_char.hp.full()) || (info.mana > 0 && m_char.mp.full())) {
        return false;
    }
    m_char.hp.restore(info.heal);
    m_char.mp.restore(info.mana);
    (void)m_char.bag.takeAt(bagSlot, 1);
    ++m_stats.potions;
    push({.kind = EventKind::Potion, .value = info.heal > 0 ? info.heal : info.mana, .extra = info.heal > 0 ? 0 : 1,
          .at = m_hero.body.box.center(), .onHero = true});
    refreshCollectQuests();
    return true;
}

bool Session::moveInBag(const int from, const int to)
{
    return m_char.bag.move(from, to);
}

bool Session::assignQuick(const int bagSlot)
{
    const ItemStack& stack = m_char.bag.at(bagSlot);
    const ItemInfo& info = itemInfo(stack.item);
    if (stack.empty() || info.kind != ItemKind::Use || (info.heal == 0 && info.mana == 0)) {
        return false;
    }
    m_char.quick[info.heal > 0 ? 0 : 1] = stack.item;
    return true;
}

std::vector<gamelib::core::ShopEntry> Session::shopEntries() const
{
    std::vector<gamelib::core::ShopEntry> e{{kRedPotion}, {kBluePotion}, {kReturnScroll}};
    const int ch = chapter();
    if (ch >= 2) {
        e.push_back({kOrangePotion});
    }
    if (ch >= 3) {
        e.push_back({kManaTonic});
    }
    if (ch >= 4) {
        e.push_back({kWhitePotion});
    }
    // Plain gear of the regions opened so far.
    for (int tier = 1; tier <= std::min(ch, 5); ++tier) {
        for (const Slot s : {Slot::Weapon, Slot::Helmet, Slot::Armor, Slot::Shoes}) {
            e.push_back({equipItem(tier, s, 0)});
        }
    }
    return e;
}

gamelib::core::TradeResult Session::buy(const int entry, const int count)
{
    gamelib::core::Shop shop{itemRegistry(), shopEntries()};
    const gamelib::core::TradeResult r = shop.buy(entry, count, m_char.bag, m_char.coins);
    if (r == gamelib::core::TradeResult::Done) {
        push({.kind = EventKind::Bought});
        refreshCollectQuests();
    } else if (r == gamelib::core::TradeResult::InventoryFull) {
        push({.kind = EventKind::Full});
    }
    return r;
}

bool Session::sellFromBag(const int bagSlot)
{
    const ItemStack stack = m_char.bag.at(bagSlot);
    if (stack.empty() || itemInfo(stack.item).kind == ItemKind::Quest) {
        return false;
    }
    // Equipment sells by its rarity and upgrades (core::Shop pays the plain
    // price), so the stack is taken and paid here.
    const int each = itemInfo(stack.item).kind == ItemKind::Equip ? sellValue(stack) : itemRegistry().at(stack.item).sellPrice;
    if (each <= 0) {
        return false;
    }
    (void)m_char.bag.takeAt(bagSlot, stack.count);
    m_char.coins += static_cast<std::int64_t>(each) * stack.count;
    push({.kind = EventKind::Sold, .value = each * stack.count});
    refreshCollectQuests();
    return true;
}

const ItemStack& Session::upgradeTarget(const int target) const
{
    return target < 0 ? m_char.bag.at(-1 - target) : m_char.gear.at(target);
}

std::optional<UpgradeCost> Session::upgradeCostOf(const int target) const
{
    const ItemStack& s = upgradeTarget(target);
    if (s.empty() || itemInfo(s.item).kind != ItemKind::Equip || upgradeOf(s) >= kMaxUpgrade) {
        return std::nullopt;
    }
    return upgradeCost(s);
}

bool Session::upgrade(const int target)
{
    const std::optional<UpgradeCost> cost = upgradeCostOf(target);
    if (!cost || m_char.coins < cost->coins || m_char.bag.countOf(materialItem(Material::GlimmerOre)) < cost->ore) {
        return false;
    }
    ItemStack s = upgradeTarget(target);
    applyUpgrade(s);
    m_char.coins -= cost->coins;
    m_char.bag.remove(materialItem(Material::GlimmerOre), cost->ore);
    if (target < 0) {
        m_char.bag.setAt(-1 - target, s);
    } else {
        m_char.gear.setAt(target, s);
    }
    m_char.refreshResources();
    refreshDerived();
    refreshCollectQuests();
    push({.kind = EventKind::Upgrade, .value = upgradeOf(s)});
    return true;
}

// ---------------------------------------------------------------------------
// The tick

void Session::tick(const Input& input)
{
    ++m_ticks;
    ++m_stats.ticks;
    ++m_mapTicks;
    if (m_phase == Phase::Ending) {
        ++m_endingTicks;
        return;
    }
    if (m_travel) {
        return; // the app runs the transition, then calls travel()
    }
    m_clock.tick();
    if ((night() ? 1 : 0) != m_nightCheck) {
        placeNpcs();
    }
    refreshDerived();
    for (const int expired : m_char.buffs.tick()) {
        (void)expired;
        m_char.refreshResources();
        refreshDerived();
    }
    if (const std::optional<gamelib::core::SkillUse> done = m_char.skills.tick()) {
        (void)done;
    }
    if (m_phase == Phase::Dead) {
        ++m_deadTicks;
        m_hero.previous = m_hero.body.box;
        tickMonsters();
        return;
    }
    tickHero(input);
    tickMonsters();
    tickBoss();
    tickShots();
    tickDrops();
    tickSpawns();
    // Slow regeneration out of combat: MapleStory's resting recovery.
    if (m_ticks % (4 * kTickRate) == 0 && m_hero.hurtTicks == 0) {
        m_char.hp.restore(std::max(1, m_char.hp.max / 50));
        m_char.mp.restore(std::max(1, m_char.mp.max / 40));
    }
}

void Session::drinkQuick(const int which)
{
    const int item = m_char.quick[static_cast<std::size_t>(which)];
    if (const std::optional<int> slot = m_char.bag.firstSlotOf(item)) {
        if (!useFromBag(*slot)) {
            push({.kind = EventKind::Cooldown, .value = -1});
        }
    } else {
        // Fall back to any potion of the right kind.
        for (int s = 0; s < m_char.bag.size(); ++s) {
            const ItemInfo& info = itemInfo(m_char.bag.at(s).item);
            if (!m_char.bag.at(s).empty() && info.kind == ItemKind::Use && (which == 0 ? info.heal > 0 : info.mana > 0)) {
                (void)useFromBag(s);
                return;
            }
        }
        push({.kind = EventKind::NoMana, .value = -1});
    }
}

void Session::useSkill(const SkillId skill)
{
    const int id = static_cast<int>(skill);
    if (m_char.skills.levelOf(id) == 0) {
        return;
    }
    const gamelib::core::UseCheck check = m_char.skills.check(id, m_char.mp);
    if (check == gamelib::core::UseCheck::NotEnoughResource) {
        push({.kind = EventKind::NoMana, .value = id, .at = m_hero.body.box.center()});
        return;
    }
    if (check != gamelib::core::UseCheck::Ready) {
        push({.kind = EventKind::Cooldown, .value = id});
        return;
    }
    const std::optional<gamelib::core::SkillUse> use = m_char.skills.use(id, m_char.mp);
    if (!use) {
        return;
    }
    Hero& h = m_hero;
    h.combo.cancel();
    push({.kind = EventKind::SkillCast, .value = id, .at = h.body.box.center()});
    switch (skill) {
    case SkillId::Roll:
        h.rollLength = 20;
        h.rollTicks = h.rollLength;
        h.vulnerable.makeInvincible(use->values.durationTicks);
        push({.kind = EventKind::Roll, .at = h.body.box.bottomCenter()});
        break;
    case SkillId::Sweep:
        h.casting = skill;
        h.castLength = sweepClip(0).totalTicks();
        h.castTicks = h.castLength;
        (void)h.skill.start(sweepClip(use->values.value), h.body.facing);
        break;
    case SkillId::Starbolt: {
        h.casting = skill;
        h.castLength = 16;
        h.castTicks = h.castLength;
        const Vec2 from = h.body.box.center() + Vec2{static_cast<float>(h.body.facing) * 40.0f, -14.0f};
        ShotData data{.hero = true, .look = ShotLook::Star, .hitsLeft = starboltTargets(use->level)};
        gamelib::core::Projectile<ShotData> shot{.position = from, .velocity = {static_cast<float>(h.body.facing) * 12.0f, 0.0f},
                                                 .size = {36.0f, 36.0f}, .lifetimeTicks = 70, .team = 1, .owner = 1,
                                                 .damage = use->values.value, .pierce = true,
                                                 .motion = gamelib::core::ProjectileMotion::Homing,
                                                 .tileContact = gamelib::core::TileContact::Ignore, .homingTurn = 0.09f,
                                                 .target = from + Vec2{static_cast<float>(h.body.facing) * 400.0f, 0.0f}, .extra = data};
        (void)m_shots.spawn(shot);
        break;
    }
    case SkillId::Tailwind:
        h.casting = skill;
        h.castLength = 20;
        h.castTicks = h.castLength;
        m_char.buffs.apply(kTailwindBuff, {.speed = use->values.value, .jump = use->values.value / 2}, use->values.durationTicks);
        refreshDerived();
        break;
    case SkillId::Bloom: {
        h.casting = skill;
        h.castLength = 26;
        h.castTicks = h.castLength;
        const int amount = m_char.hp.max * use->values.value / 100 + m_derived.magic * 2;
        m_char.hp.restore(amount);
        push({.kind = EventKind::Heal, .value = amount, .at = h.body.box.center(), .onHero = true});
        break;
    }
    default: break;
    }
}

void Session::tickHero(const Input& input)
{
    Hero& h = m_hero;
    gamelib::core::PlatformerBody& b = h.body;
    h.previous = b.box;
    h.vulnerable.tick();
    h.hurtTicks = std::max(0, h.hurtTicks - 1);
    h.hyperTicks = std::max(0, h.hyperTicks - 1);
    const gamelib::core::TileWorld w = world();

    // Up: talk, the inn, a door - before it reaches the ladders.
    bool upUsed = false;
    if (input.upPressed && b.grounded && h.castTicks == 0 && h.rollTicks == 0) {
        if (const std::optional<NpcId> npc = npcInReach()) {
            m_talk = npcRoot(*npc);
            startTalk(*npc);
            upUsed = true;
        } else if (innInReach()) {
            m_talk = "inn";
            upUsed = true;
        } else if (const int p = portalInReach(); p >= 0) {
            if (portalOpen(p)) {
                const Portal& portal = map().portals[static_cast<std::size_t>(p)];
                m_travel = {portal.to, portal.toPortal};
                if (portal.afterBoss) {
                    m_travel = {portal.to, portal.toPortal};
                }
                return;
            }
            push({.kind = EventKind::PortalClosed, .at = map().portals[static_cast<std::size_t>(p)].at});
            upUsed = true;
        }
    }
    if (m_talk) {
        return;
    }

    if (input.hpPotion) {
        drinkQuick(0);
    }
    if (input.mpPotion) {
        drinkQuick(1);
    }
    const bool free = h.hurtTicks == 0 && h.castTicks == 0 && h.rollTicks == 0;
    if (free) {
        for (int i = 0; i < kHotkeyCount; ++i) {
            if (input.skillPressed[i]) {
                useSkill(hotkeySkill(i));
                break;
            }
        }
    }
    if (input.rollPressed && h.hurtTicks == 0 && h.castTicks == 0 && h.rollTicks == 0 && !b.climbing
        && (!h.combo.busy() || h.combo.cancellable())) {
        if (input.move != 0) {
            b.facing = input.move;
        }
        useSkill(SkillId::Roll);
    }
    // The combo.
    const WeaponKind weapon = static_cast<WeaponKind>([&] {
        const int style = itemInfo(m_char.gear.at(static_cast<int>(Slot::Weapon)).item).style;
        return style == 2 ? 1 : style == 4 ? 2 : style == 5 ? 3 : 0;
    }());
    if (input.attackPressed && h.hurtTicks == 0 && h.castTicks == 0 && h.rollTicks == 0 && !b.climbing) {
        const auto clips = comboClips(weapon);
        const int step = h.combo.nextComboStep() % 3;
        if (input.move != 0 && !h.combo.busy()) {
            b.facing = input.move;
        }
        if (h.combo.start(clips[static_cast<std::size_t>(step)], b.facing) == gamelib::core::AttackPress::Started) {
            push({.kind = EventKind::Swing, .value = step, .at = b.box.center()});
        }
    }

    // Movement intent.
    gamelib::core::PlatformerIntent intent;
    intent.move = static_cast<float>(input.move);
    intent.climb = upUsed ? 0.0f : static_cast<float>(input.climb);
    intent.jumpPressed = input.jumpPressed;
    intent.jumpHeld = input.jumpHeld;
    const bool attacking = h.combo.busy() && h.combo.phase() != gamelib::core::AttackPhase::Recovery;
    if (h.hurtTicks > 0 || h.castTicks > 0 || h.rollTicks > 0 || (attacking && b.grounded)) {
        intent.move = 0.0f;
        intent.climb = 0.0f;
        intent.jumpPressed = h.rollTicks > 0 ? false : (h.castTicks > 0 ? false : intent.jumpPressed && h.hurtTicks == 0);
    }
    if (h.castTicks > 0 || h.hurtTicks > 0) {
        intent.jumpPressed = false;
    }
    // A jump cancels the combo's recovery.
    if (intent.jumpPressed && h.combo.busy() && h.combo.cancellable()) {
        h.combo.cancel();
    }
    if (intent.jumpPressed && attacking && b.grounded) {
        intent.jumpPressed = false;
    }

    // The hyper jump: a second jump in the air.
    const int hyperLevel = m_char.skillLevel(SkillId::HyperJump);
    if (input.jumpPressed && !b.grounded && !b.climbing && b.coyote == 0 && hyperLevel > 0 && h.hyperLeft > 0 && h.hurtTicks == 0
        && h.castTicks == 0 && h.rollTicks == 0) {
        const int id = static_cast<int>(SkillId::HyperJump);
        if (m_char.skills.use(id, m_char.mp)) {
            const float power = static_cast<float>(skillDef(SkillId::HyperJump).levels[static_cast<std::size_t>(hyperLevel - 1)].value) / 100.0f;
            const float jumpPct = static_cast<float>(m_derived.jump) / 100.0f;
            if (input.move != 0) {
                b.facing = input.move;
            }
            h.hyperUp = input.climb < 0;
            const Vec2 v = h.hyperUp ? Vec2{b.velocity.x * 0.4f, -kHyperUp * power * std::sqrt(jumpPct)}
                                     : Vec2{static_cast<float>(b.facing) * kHyperForward * power * (static_cast<float>(m_derived.speed) / 100.0f),
                                            -kHyperRise * std::sqrt(jumpPct)};
            gamelib::core::launch(b, v, false);
            b.lockTicks = h.hyperUp ? 4 : kHyperLock;
            h.hyperLeft = 0;
            h.hyperTicks = 26;
            h.combo.cancel();
            push({.kind = EventKind::HyperJump, .value = h.hyperUp ? 1 : 0, .at = b.box.center()});
        } else {
            push({.kind = EventKind::NoMana, .value = id, .at = b.box.center()});
        }
        intent.jumpPressed = false;
    }

    gamelib::core::PlatformerTuning tuning = m_char.tuning();
    if (h.rollTicks > 0) {
        --h.rollTicks;
        b.velocity.x = static_cast<float>(b.facing) * kRollSpeed * (static_cast<float>(m_derived.speed) / 100.0f)
                       * (0.4f + 0.6f * static_cast<float>(h.rollTicks) / static_cast<float>(h.rollLength));
        b.lockTicks = std::max(b.lockTicks, 1);
    }
    if (h.hyperTicks > 0 && !h.hyperUp) {
        // Keep the burst's speed: no air drag for a moment.
        tuning.airDecel = 0.0f;
        tuning.maxSpeed = std::max(tuning.maxSpeed, std::abs(b.velocity.x));
    }
    const bool wasGrounded = b.grounded;
    const bool wasClimbing = b.climbing;
    const gamelib::core::PlatformerStep step = gamelib::core::stepPlatformer(b, intent, tuning, w, {});
    if (step.events.jumped || step.events.climbJumped) {
        push({.kind = EventKind::Jump, .at = b.box.bottomCenter()});
    }
    if (step.events.landed) {
        push({.kind = EventKind::Land, .value = static_cast<int>(h.airTicks), .at = b.box.bottomCenter()});
        h.landTicks = 0;
    }
    if (b.grounded || b.climbing) {
        h.hyperLeft = 1;
        h.airTicks = 0;
    } else {
        ++h.airTicks;
    }
    if (b.climbing && !wasClimbing) {
        push({.kind = EventKind::Climb, .at = b.box.center()});
        h.combo.cancel();
    }
    (void)wasGrounded;
    ++h.landTicks;
    h.idleTicks = (std::abs(b.velocity.x) < 0.1f && b.grounded && !h.combo.busy()) ? h.idleTicks + 1 : 0;

    // Edge portals: walking into the map's end.
    const MapDef& m = map();
    if (b.grounded && input.move != 0) {
        const bool atLeft = b.box.left() <= 1.0f && input.move < 0;
        const bool atRight = b.box.right() >= m.bounds().right() - 1.0f && input.move > 0;
        if (atLeft || atRight) {
            for (int i = 0; i < static_cast<int>(m.portals.size()); ++i) {
                const Portal& p = m.portals[static_cast<std::size_t>(i)];
                const bool sideMatches = (p.at.x < m.bounds().width * 0.5f) == atLeft;
                if (p.kind == PortalKind::Edge && sideMatches && std::abs(p.at.y - b.box.bottom()) < kTile * 4.0f) {
                    m_travel = {p.to, p.toPortal};
                    return;
                }
            }
        }
    }

    // Attacks.
    const Vec2 anchor = b.box.center();
    const gamelib::core::AttackStep combo = h.combo.tick(anchor);
    if (combo.chained) {
        push({.kind = EventKind::Swing, .value = h.combo.comboStep() % 3, .at = anchor});
    }
    if (combo.hit) {
        heroHits(*combo.hit, combo.hit->damage, false, true);
    }
    if (h.castTicks > 0) {
        --h.castTicks;
        if (h.casting == SkillId::Sweep) {
            const gamelib::core::AttackStep sweep = h.skill.tick(anchor);
            if (sweep.hit) {
                heroHits(*sweep.hit, sweep.hit->damage, false, false);
            }
        }
    }

    // Hazards: monster bodies and lunges.
    h.vulnerable.setHurtbox(b.box.expanded(-4.0f));
    if (h.vulnerable.invincible()) {
        return;
    }
    for (Monster& mon : m_monsters) {
        if (!mon.alive() || mon.state == MonsterState::Spawn) {
            continue;
        }
        if (overlapsRect(mon.box.expanded(-6.0f), h.vulnerable.hurtbox())) {
            const int atk = monsterAttack(mon.level, mon.elite) * (mon.state == MonsterState::Attack ? 13 : 10) / 10;
            const int dmg = std::max(1, atk * m_rng.range(90, 110) / 100 - m_derived.defense / 2);
            damageHero(dmg, mon.box.center().x);
            return;
        }
    }
}

void Session::heroHits(const gamelib::core::Hit& hit, const int percent, const bool magic, const bool combo)
{
    const int mastery = combo ? skillDef(SkillId::Combo).levels[static_cast<std::size_t>(std::max(m_char.skillLevel(SkillId::Combo), 1) - 1)].value : 0;
    for (Monster& m : m_monsters) {
        if (!m.alive() || m.state == MonsterState::Spawn) {
            continue;
        }
        if (const std::optional<gamelib::core::Landed> landed = m.vulnerable.tryHit(hit)) {
            bool crit = false;
            const int dmg = damageRoll(percent * (100 + mastery) / 100, magic, monsterDefense(m.level), crit);
            const int dir = m.box.center().x >= m_hero.body.box.center().x ? 1 : -1;
            hurtMonster(m, dmg, dir, static_cast<int>(m_ticks));
            m_stats.bestHit = std::max(m_stats.bestHit, dmg);
            push({.kind = EventKind::Hit, .value = dmg, .extra = landed->hitStopTicks,
                  .at = {m.box.center().x, m.box.top()}, .crit = crit});
            if (!m.alive()) {
                defeatMonster(m);
            }
        }
    }
    if (m_boss && m_boss->alive() && m_boss->state != BossState::Intro && m_boss->fade > 0.5f) {
        Boss& boss = *m_boss;
        if (boss.guarded) {
            if (boss.vulnerable.guardedHit(hit)) {
                push({.kind = EventKind::Guarded, .at = boss.box.center()});
            }
        } else if (const std::optional<gamelib::core::Landed> landed = boss.vulnerable.tryHit(hit)) {
            bool crit = false;
            const int dmg = damageRoll(percent * (100 + mastery) / 100, magic, monsterDefense(boss.info().level), crit);
            m_stats.bestHit = std::max(m_stats.bestHit, dmg);
            push({.kind = EventKind::Hit, .value = dmg, .extra = landed->hitStopTicks, .at = {boss.box.center().x, boss.box.top()},
                  .crit = crit});
            if (hurtBoss(boss, dmg, static_cast<int>(m_ticks))) {
                push({.kind = EventKind::BossDown, .value = static_cast<int>(boss.id), .at = boss.box.center()});
            }
        }
    }
}

int Session::damageRoll(const int percent, const bool magic, const int defense, bool& crit)
{
    const int power = magic ? m_derived.magic : m_derived.attack;
    int raw = power * percent / 100 * m_rng.range(90, 110) / 100;
    crit = m_rng.range(0, 99) < m_derived.crit;
    if (crit) {
        raw = raw * 160 / 100;
    }
    return std::max(1, raw - defense / 2);
}

void Session::damageHero(const int damage, const float fromX)
{
    Hero& h = m_hero;
    if (!h.alive || h.vulnerable.invincible()) {
        return;
    }
    m_char.hp.restore(-damage);
    h.vulnerable.makeInvincible(kHurtInvincible);
    h.hurtTicks = kHurtStun;
    h.combo.cancel();
    h.skill.cancel();
    h.castTicks = 0;
    h.rollTicks = 0;
    const int dir = h.body.box.center().x >= fromX ? 1 : -1;
    h.body.climbing = false;
    gamelib::core::launch(h.body, {static_cast<float>(dir) * 4.5f, -5.5f}, false);
    h.body.lockTicks = kHurtStun;
    push({.kind = EventKind::Hurt, .value = damage, .at = {h.body.box.center().x, h.body.box.top()}, .onHero = true});
    if (m_char.hp.current <= 0) {
        h.alive = false;
        m_phase = Phase::Dead;
        m_deadTicks = 0;
        ++m_stats.deaths;
        push({.kind = EventKind::Death, .at = h.body.box.center()});
    }
}

void Session::defeatMonster(Monster& m)
{
    ++m_stats.defeated;
    if (m.elite) {
        ++m_stats.elites;
    }
    push({.kind = EventKind::MonsterDown, .value = static_cast<int>(m.species), .extra = m.elite ? 1 : 0, .at = m.box.center()});
    for (const auto& u : m_quests.onEvent({.kind = ObjectiveKind::Defeat, .subject = static_cast<int>(m.species)})) {
        push({.kind = u.questComplete ? EventKind::QuestComplete : EventKind::QuestUpdate, .value = u.quest, .extra = u.progress});
    }
    dropLoot(m);
    gainXp(monsterXp(m.level, m.elite), m.box.center());
}

void Session::dropLoot(const Monster& m)
{
    const Species& s = m.kind();
    const Vec2 at = m.box.center();
    int index = 0;
    const std::int64_t coins = static_cast<std::int64_t>(m.level * 3 + m_rng.range(0, m.level * 2 + 2)) * (m.elite ? 3 : 1);
    spawnDrop({}, coins, at, index++);
    if (m.elite || m_rng.chance(0.45f)) {
        spawnDrop({.item = materialItem(s.material), .count = 1}, 0, at, index++);
    }
    if (m_rng.chance(m.elite ? 0.25f : 0.06f)) {
        const bool hp = m_rng.chance(0.6f);
        const int item = hp ? (m.level < 10 ? kRedPotion : m.level < 20 ? kOrangePotion : kWhitePotion)
                            : (m.level < 15 ? kBluePotion : kManaTonic);
        spawnDrop({.item = item, .count = 1}, 0, at, index++);
    }
    if (m_rng.chance(m.elite ? 0.35f : 0.035f)) {
        spawnDrop(rollRandomEquipment(static_cast<int>(s.region), m_rng, m.elite), 0, at, index++);
    }
    if (m.species == SpeciesId::BogMochi && m_quests.stateOf(kQuestStarCharm) == QuestState::Active
        && m_char.bag.countOf(kStarCharm) == 0 && m_rng.chance(0.25f)) {
        spawnDrop({.item = kStarCharm, .count = 1}, 0, at, index++);
    }
}

void Session::spawnDrop(ItemStack stack, const std::int64_t coins, const Vec2 at, const int index)
{
    Drop d;
    d.stack = std::move(stack);
    d.coins = coins;
    d.box = Rect::fromCenter(at, {30.0f, 30.0f});
    d.previous = d.box;
    const float spread = (static_cast<float>(index) - 1.0f) * 1.6f + m_rng.uniform(-0.6f, 0.6f);
    d.velocity = {spread, -7.5f - m_rng.uniform(0.0f, 1.5f)};
    m_drops.push_back(std::move(d));
    if (!m_drops.back().stack.empty()) {
        push({.kind = EventKind::Spawned, .value = m_drops.back().stack.item, .extra = m_drops.back().stack.rarity + 100, .at = at});
    }
}

void Session::collect(Drop& d)
{
    if (d.coins > 0) {
        m_char.coins += d.coins;
        m_stats.coinsEarned += d.coins;
        push({.kind = EventKind::Coins, .value = static_cast<int>(d.coins), .at = d.box.center()});
        d.coins = 0;
        return;
    }
    const int before = d.stack.count;
    const int left = m_char.bag.add(d.stack);
    if (left == before) {
        push({.kind = EventKind::Full});
        return;
    }
    push({.kind = EventKind::Pickup, .value = d.stack.item, .extra = d.stack.rarity, .at = d.box.center()});
    d.stack.count = left;
    refreshCollectQuests();
}

void Session::tickDrops()
{
    const gamelib::core::TileWorld w = world();
    const Rect hero = m_hero.body.box;
    for (Drop& d : m_drops) {
        d.previous = d.box;
        ++d.age;
        if (!d.grounded) {
            d.velocity.y = std::min(d.velocity.y + 0.45f, 10.0f);
            const gamelib::core::BoxMove move = gamelib::core::moveBox(d.box, d.velocity, w);
            d.box = move.box;
            if (move.contacts.blockedX) {
                d.velocity.x = -d.velocity.x * 0.5f;
            }
            if (move.contacts.ground) {
                if (d.velocity.y > 3.0f) {
                    d.velocity.y = -d.velocity.y * 0.35f;
                    d.velocity.x *= 0.6f;
                } else {
                    d.grounded = true;
                    d.velocity = {};
                }
            }
        }
        if (d.age > 22 && m_hero.alive && overlapsRect(d.box.expanded(10.0f), hero)) {
            collect(d);
        }
    }
    std::erase_if(m_drops, [](const Drop& d) { return (d.coins == 0 && d.stack.empty()) || d.age > kDropLifetime; });
}

void Session::tickMonsters()
{
    const gamelib::core::TileWorld w = world();
    const HeroView hero{.box = m_hero.body.box, .alive = m_hero.alive && m_phase == Phase::Playing};
    for (Monster& m : m_monsters) {
        const MonsterStep step = stepMonster(m, hero, w, static_cast<int>(m_ticks));
        if (step.lunged) {
            push({.kind = EventKind::EnemyShot, .value = static_cast<int>(m.species), .at = m.box.center()});
        }
        if (step.finished && m.spawn >= 0 && m.spawn < static_cast<int>(m_slots.size())) {
            SpawnSlot& slot = m_slots[static_cast<std::size_t>(m.spawn)];
            slot.occupied = false;
            slot.timer = kRespawnDelay + m_rng.range(0, 3 * kTickRate);
        }
        // Fell out of the world: gone, and it comes back.
        if (m.box.top() > map().bounds().bottom() + 200.0f && m.alive()) {
            m.state = MonsterState::Defeat;
            m.stateTicks = m.stateLength = 1;
        }
    }
    std::erase_if(m_monsters, [](const Monster& m) { return m.state == MonsterState::Defeat && m.stateTicks >= m.stateLength; });
}

void Session::tickSpawns()
{
    for (int i = 0; i < static_cast<int>(m_slots.size()); ++i) {
        SpawnSlot& slot = m_slots[static_cast<std::size_t>(i)];
        if (slot.occupied) {
            continue;
        }
        if (slot.timer > 0 && --slot.timer == 0) {
            // Not right on top of the hero.
            const SpawnDef& def = map().spawns[static_cast<std::size_t>(slot.def)];
            if (overlapsRect(def.run.expanded(60.0f), m_hero.body.box) && def.run.width < 6.0f * kTile) {
                slot.timer = kTickRate;
                continue;
            }
            spawnInto(i, false);
        }
    }
}

void Session::tickBoss()
{
    if (!m_boss) {
        return;
    }
    Boss& b = *m_boss;
    const BossStep step = stepBoss(b, m_hero.body.box, world(), m_rng, static_cast<int>(m_ticks));
    const BossInfo& info = b.info();
    for (const BossShotRequest& s : step.shots) {
        gamelib::core::Projectile<ShotData> shot{.position = s.at, .velocity = s.velocity, .size = s.size,
                                                 .lifetimeTicks = s.lifetime, .team = 2, .owner = 2,
                                                 .damage = bossAttack(b.id),
                                                 .tileContact = s.groundWave ? gamelib::core::TileContact::Ignore
                                                                             : gamelib::core::TileContact::Remove,
                                                 .gravity = s.gravity, .extra = {.look = s.look, .groundWave = s.groundWave}};
        (void)m_shots.spawn(shot);
    }
    if (!step.shots.empty()) {
        push({.kind = EventKind::EnemyShot, .value = -1, .at = b.box.center()});
    }
    for (const float x : step.rain) {
        m_rain.add(x, 64);
    }
    if (!step.rain.empty()) {
        push({.kind = EventKind::RainWarn, .at = b.box.center()});
    }
    if (step.summon > 0) {
        const int alive = static_cast<int>(std::ranges::count_if(m_monsters, [](const Monster& m) { return m.alive(); }));
        for (int i = 0; i < std::min(step.summon, 5 - alive); ++i) {
            const float x = std::clamp(b.box.center().x + (i % 2 == 0 ? -1.0f : 1.0f) * (120.0f + 60.0f * static_cast<float>(i)), b.bounds.left() + 60.0f,
                                       b.bounds.right() - 60.0f);
            const Vec2 feet{x, species(info.minion).behavior == Behavior::Flyer ? b.bounds.bottom() - 200.0f : b.bounds.bottom()};
            Monster m = makeMonster(info.minion, std::max(1, info.level - 3), false, feet, b.bounds.left(), b.bounds.right(),
                                    static_cast<std::uint8_t>(m_rng.range(0, 255)));
            m.aggro = 600;
            m_monsters.push_back(m);
            push({.kind = EventKind::Spawned, .at = feet});
        }
    }
    if (step.landed) {
        push({.kind = EventKind::BossLand, .at = b.box.bottomCenter()});
    }
    if (step.roared) {
        push({.kind = EventKind::BossRoar, .value = static_cast<int>(b.action), .at = b.box.center()});
    }
    if (step.phaseChanged) {
        push({.kind = EventKind::BossPhase, .value = b.phase, .at = b.box.center()});
    }
    if (step.contact && m_hero.alive && !m_hero.vulnerable.invincible()
        && overlapsRect(b.box.expanded(-14.0f), m_hero.vulnerable.hurtbox())) {
        const int dmg = std::max(1, bossAttack(b.id) * m_rng.range(90, 110) / 100 - m_derived.defense / 2);
        damageHero(dmg, b.box.center().x);
    }
    if (step.finished) {
        defeatBoss();
    }
}

void Session::defeatBoss()
{
    const Boss b = *m_boss;
    m_boss.reset();
    m_flags.set("boss" + std::to_string(static_cast<int>(b.id)));
    ++m_stats.defeated;
    for (const auto& u : m_quests.onEvent({.kind = ObjectiveKind::Defeat, .subject = kBossSubject + static_cast<int>(b.id)})) {
        push({.kind = u.questComplete ? EventKind::QuestComplete : EventKind::QuestUpdate, .value = u.quest, .extra = u.progress});
    }
    // Minions scatter.
    for (Monster& m : m_monsters) {
        if (m.alive()) {
            hurtMonster(m, m.hp, 1, static_cast<int>(m_ticks));
        }
    }
    const Vec2 at = b.box.center();
    const int tier = std::min(static_cast<int>(b.id) + 1, 5);
    int index = 0;
    spawnDrop({}, static_cast<std::int64_t>(b.info().level) * 60, at, index++);
    for (int i = 0; i < 2; ++i) {
        ItemStack gear = rollRandomEquipment(tier, m_rng, true);
        gear.rarity = std::max(gear.rarity, 1 + i);
        spawnDrop(gear, 0, at, index++);
    }
    spawnDrop({.item = b.id == BossId::Rumblerock ? materialItem(Material::GlimmerOre) : kOrangePotion, .count = 5}, 0, at, index++);
    gainXp(bossXp(b.id), at);
    push({.kind = EventKind::ChapterUp, .value = chapter()});
    m_saveWanted = true;
    if (b.id == BossId::Hush) {
        // The last guardian: the story ends.
        (void)m_quests.onEvent({.kind = ObjectiveKind::Defeat, .subject = kBossSubject + static_cast<int>(b.id)});
        if (m_quests.stateOf(kQuestChapter5) == QuestState::Complete) {
            if (const std::optional<gamelib::core::QuestReward> reward = m_quests.turnIn(kQuestChapter5)) {
                applyReward(*reward);
            }
        }
        m_finished = true;
        m_phase = Phase::Ending;
        m_endingTicks = 0;
        push({.kind = EventKind::Ending});
    }
}

void Session::tickShots()
{
    const gamelib::core::TileWorld w = world();
    // Starbolts home on the nearest monster ahead.
    for (auto& s : m_shots.items()) {
        if (!s.extra.hero) {
            if (s.extra.groundWave) {
                s.velocity.y = 0.0f;
            }
            continue;
        }
        float best = 760.0f;
        for (const Monster& m : m_monsters) {
            if (!m.alive() || m.state == MonsterState::Spawn) {
                continue;
            }
            const Vec2 d = m.box.center() - s.position;
            const float dist = std::sqrt(d.x * d.x + d.y * d.y);
            if (dist < best && d.x * s.velocity.x > -40.0f) {
                best = dist;
                s.target = m.box.center();
            }
        }
        if (m_boss && m_boss->alive()) {
            const Vec2 d = m_boss->box.center() - s.position;
            if (std::sqrt(d.x * d.x + d.y * d.y) < best) {
                s.target = m_boss->box.center();
            }
        }
    }
    m_shots.step(1.0f, gamelib::core::shotSolid(w), map().bounds().expanded(100.0f));
    for (auto& s : m_shots.items()) {
        if (s.extra.hero) {
            const gamelib::core::Hit hit = s.hit();
            for (Monster& m : m_monsters) {
                if (s.extra.hitsLeft <= 0 || !m.alive() || m.state == MonsterState::Spawn) {
                    continue;
                }
                if (m.vulnerable.tryHit(hit)) {
                    bool crit = false;
                    const int dmg = damageRoll(s.damage, true, monsterDefense(m.level), crit);
                    hurtMonster(m, dmg, s.velocity.x >= 0.0f ? 1 : -1, static_cast<int>(m_ticks));
                    m_stats.bestHit = std::max(m_stats.bestHit, dmg);
                    push({.kind = EventKind::Hit, .value = dmg, .extra = 2, .at = {m.box.center().x, m.box.top()}, .crit = crit});
                    --s.extra.hitsLeft;
                    if (!m.alive()) {
                        defeatMonster(m);
                    }
                }
            }
            if (s.extra.hitsLeft > 0 && m_boss && m_boss->alive() && m_boss->state != BossState::Intro) {
                Boss& b = *m_boss;
                if (b.guarded) {
                    if (b.vulnerable.guardedHit(hit)) {
                        push({.kind = EventKind::Guarded, .at = s.position});
                        s.extra.hitsLeft = 0;
                    }
                } else if (b.vulnerable.tryHit(hit)) {
                    bool crit = false;
                    const int dmg = damageRoll(s.damage, true, monsterDefense(b.info().level), crit);
                    m_stats.bestHit = std::max(m_stats.bestHit, dmg);
                    push({.kind = EventKind::Hit, .value = dmg, .extra = 2, .at = {b.box.center().x, b.box.top()}, .crit = crit});
                    s.extra.hitsLeft = 0;
                    if (hurtBoss(b, dmg, static_cast<int>(m_ticks))) {
                        push({.kind = EventKind::BossDown, .value = static_cast<int>(b.id), .at = b.box.center()});
                    }
                }
            }
        } else if (m_hero.alive && !m_hero.vulnerable.invincible() && overlapsRect(s.box(), m_hero.vulnerable.hurtbox())) {
            const int dmg = std::max(1, s.damage * m_rng.range(90, 110) / 100 - m_derived.defense / 2);
            damageHero(dmg, s.position.x);
            s.extra.hitsLeft = 0;
        }
    }
    (void)m_shots.removeIf([](const auto& s) { return s.extra.hitsLeft <= 0; });
    // The rain: warned spots, then falling hazards.
    if (m_boss || !m_rain.empty()) {
        const ShotLook look = m_boss ? m_boss->info().shot : ShotLook::Spore;
        for (const float x : m_rain.tick()) {
            gamelib::core::Projectile<ShotData> drop{.position = {x, map().bounds().top() + 40.0f}, .velocity = {0.0f, 9.0f},
                                                     .size = {38.0f, 38.0f}, .lifetimeTicks = 200, .team = 2, .owner = 3,
                                                     .damage = m_boss ? bossAttack(m_boss->id) : 10,
                                                     .tileContact = gamelib::core::TileContact::Remove, .extra = {.look = look}};
            (void)m_shots.spawn(drop);
        }
    }
}

// ---------------------------------------------------------------------------
// Developer aids

void Session::debugTeleport(const MapId target, const int portal)
{
    m_travel = {target, portal};
    travel();
}

void Session::debugLevel(const int level)
{
    while (m_char.level < level) {
        m_char.gainXp(m_char.xpToNext());
    }
    m_char.autoAssign();
    m_quests.refresh(m_char.level, [](int) { return false; });
    refreshDerived();
}

void Session::debugGrantChapter(const int target)
{
    if (m_quests.stateOf(kQuestArrival) != QuestState::TurnedIn) {
        (void)m_quests.accept(kQuestArrival);
        (void)m_quests.onEvent({.kind = ObjectiveKind::Talk, .subject = static_cast<int>(NpcId::Dorran)});
        (void)m_quests.turnIn(kQuestArrival);
        m_char.skills.setLevel(static_cast<int>(SkillId::HyperJump), 1);
    }
    for (int b = 0; b < target - 1 && b < kBossCount; ++b) {
        m_flags.set("boss" + std::to_string(b));
        const int q = kQuestChapter1 + b;
        m_quests.refresh(m_char.level, [](int) { return false; });
        (void)m_quests.accept(q);
        (void)m_quests.onEvent({.kind = ObjectiveKind::Defeat, .subject = kBossSubject + b});
        (void)m_quests.turnIn(q);
    }
    m_quests.refresh(m_char.level, [](int) { return false; });
    if (target <= kBossCount) {
        (void)m_quests.accept(kQuestChapter1 + target - 1);
    }
}

void Session::debugBossLow()
{
    if (m_boss) {
        m_boss->hp = std::max(1, m_boss->maxHp / 20);
    }
}

void Session::debugTime(const int hour)
{
    m_clock.setTime(hour, 0);
    placeNpcs();
}

void Session::debugGear()
{
    // The best plain gear of the regions opened, a few potions and some ore.
    const int tier = std::min(chapter(), 5);
    for (const Slot s : {Slot::Weapon, Slot::Helmet, Slot::Armor, Slot::Shoes, Slot::Ring}) {
        m_char.gear.setAt(static_cast<int>(s), rollEquipment(equipItem(tier, s, 0), m_rng, true));
    }
    m_char.bag.add({.item = kOrangePotion, .count = 20});
    m_char.bag.add({.item = kManaTonic, .count = 10});
    m_char.bag.add({.item = materialItem(Material::GlimmerOre), .count = 12});
    m_char.bag.add(rollRandomEquipment(tier, m_rng, true));
    m_char.bag.add(rollRandomEquipment(tier, m_rng, true));
    m_char.coins += 5000;
    m_char.refreshResources();
    m_char.hp.refill();
    m_char.mp.refill();
    refreshDerived();
}

void Session::debugMove(const float x)
{
    const std::optional<Vec2> feet = gamelib::core::topFloor(world(), static_cast<int>(x / kTile));
    if (!feet) {
        return;
    }
    m_hero.body.box = Rect::fromBottomCenter(*feet, kHeroSize);
    m_hero.previous = m_hero.body.box;
}

} // namespace wayfarer::game

namespace wayfarer::game {

void Session::debugLearnAll()
{
    for (int round = 0; round < 12; ++round) {
        for (const SkillId s : {SkillId::Roll, SkillId::Sweep, SkillId::Starbolt, SkillId::Tailwind, SkillId::Bloom, SkillId::HyperJump,
                                SkillId::Combo}) {
            (void)learnSkill(s);
        }
    }
    (void)m_events.take();
}

} // namespace wayfarer::game
