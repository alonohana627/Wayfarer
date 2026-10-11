#include "saves.hpp"

#include "profile.hpp"

#include <gamelib/app/rpg_file.hpp>
#include <gamelib/app/world_file.hpp>
#include <gamelib/core/timers.hpp>
#include <gamelib/storage/files.hpp>

#include <algorithm>
#include <charconv>
#include <format>
#include <sstream>
#include <string>

namespace wayfarer::app {

namespace {

// helpers

using gamelib::core::ItemStack;
using gamelib::storage::IniFile;

constexpr int kSaveVersion = 1;

std::string slotName(const int slot)
{
    return std::to_string(slot + 1);
}

IniFile write(const game::SaveData& d)
{
    IniFile ini;
    const game::Character& c = d.character;
    ini.setInt("hero", "level", c.level);
    ini.setInt("hero", "xp", c.xp);
    for (int i = 0; i < game::kCoreCount; ++i) {
        ini.setInt("hero", game::coreShort(static_cast<game::Core>(i)), c.points.base().values[static_cast<std::size_t>(i)]);
    }
    ini.setInt("hero", "ability_points", c.points.available());
    ini.setInt("hero", "hp", c.hp.current);
    ini.setInt("hero", "mp", c.mp.current);
    ini.setInt("hero", "coins", c.coins);
    ini.setInt("hero", "quick_hp", c.quick[0]);
    ini.setInt("hero", "quick_mp", c.quick[1]);
    gamelib::app::writeSkillBook(c.skills, ini, "skills");
    gamelib::app::writeInventory(c.bag, ini, "bag");
    gamelib::app::writeEquipment(c.gear, ini, "gear");
    for (const auto& q : d.quests) {
        std::string line = std::to_string(static_cast<int>(q.state));
        for (const int p : q.progress) {
            line += " " + std::to_string(p);
        }
        ini.set("quests", std::to_string(q.quest), line);
    }
    ini.setInt("world", "map", static_cast<int>(d.map));
    ini.setInt("world", "x", static_cast<long long>(d.feet.x));
    ini.setInt("world", "y", static_cast<long long>(d.feet.y));
    ini.setInt("world", "clock", d.clock);
    ini.setBool("world", "finished", d.finished);
    ini.setInt("stats", "ticks", d.stats.ticks);
    ini.setInt("stats", "defeated", d.stats.defeated);
    ini.setInt("stats", "elites", d.stats.elites);
    ini.setInt("stats", "deaths", d.stats.deaths);
    ini.setInt("stats", "coins", d.stats.coinsEarned);
    ini.setInt("stats", "potions", d.stats.potions);
    ini.setInt("stats", "best_hit", d.stats.bestHit);
    gamelib::app::writeWorldFlags(d.flags, ini, "flags");
    return ini;
}

game::SaveData read(const IniFile& ini)
{
    game::SaveData d;
    game::Character& c = d.character;
    c.level = ini.getInt("hero", "level", 1, game::kMaxLevel).value_or(1);
    c.xp = std::max<long long>(0, ini.getInt("hero", "xp").value_or(0));
    game::CoreStats base;
    for (int i = 0; i < game::kCoreCount; ++i) {
        base.values[static_cast<std::size_t>(i)] =
            ini.getInt("hero", game::coreShort(static_cast<game::Core>(i)), 1, 999).value_or(game::kStartCore);
    }
    c.points.restore(base, ini.getInt("hero", "ability_points", 0, 999).value_or(0));
    gamelib::app::readSkillBook(ini, "skills", c.skills);
    c.coins = std::max<long long>(0, ini.getInt("hero", "coins").value_or(0));
    c.quick[0] = ini.getInt("hero", "quick_hp", 0, 9999).value_or(game::kRedPotion);
    c.quick[1] = ini.getInt("hero", "quick_mp", 0, 9999).value_or(game::kBluePotion);
    (void)gamelib::app::readInventory(ini, "bag", c.bag);
    for (const ItemStack& misfit : gamelib::app::readEquipment(ini, "gear", c.gear)) {
        (void)c.bag.add(misfit); // a piece that no longer fits its slot goes to the bag
    }
    if (c.gear.at(static_cast<int>(game::Slot::Weapon)).empty()) {
        c.gear.setAt(static_cast<int>(game::Slot::Weapon), {.item = game::kStarterSword, .count = 1});
    }
    c.refreshResources();
    c.hp.current = std::clamp(ini.getInt("hero", "hp", 1, 99999).value_or(c.hp.max), 1, c.hp.max);
    c.mp.current = std::clamp(ini.getInt("hero", "mp", 0, 99999).value_or(c.mp.max), 0, c.mp.max);
    for (const auto& [key, value] : ini.entries("quests")) {
        gamelib::core::QuestRecord r;
        std::from_chars(key.data(), key.data() + key.size(), r.quest);
        std::istringstream in{std::string{value}};
        int state = 0;
        in >> state;
        r.state = static_cast<gamelib::core::QuestState>(std::clamp(state, 0, 4));
        int p = 0;
        while (in >> p) {
            r.progress.push_back(p);
        }
        d.quests.push_back(r);
    }
    d.map = static_cast<game::MapId>(ini.getInt("world", "map", 0, game::kMapCount - 1).value_or(0));
    d.feet = {static_cast<float>(ini.getInt("world", "x").value_or(800)), static_cast<float>(ini.getInt("world", "y").value_or(1000))};
    d.clock = std::max<long long>(0, ini.getInt("world", "clock").value_or(0));
    d.finished = ini.getBool("world", "finished").value_or(false);
    d.stats.ticks = std::max<long long>(0, ini.getInt("stats", "ticks").value_or(0));
    d.stats.defeated = ini.getInt("stats", "defeated", 0, 9'999'999).value_or(0);
    d.stats.elites = ini.getInt("stats", "elites", 0, 9'999'999).value_or(0);
    d.stats.deaths = ini.getInt("stats", "deaths", 0, 9'999'999).value_or(0);
    d.stats.coinsEarned = std::max<long long>(0, ini.getInt("stats", "coins").value_or(0));
    d.stats.potions = ini.getInt("stats", "potions", 0, 9'999'999).value_or(0);
    d.stats.bestHit = ini.getInt("stats", "best_hit", 0, 9'999'999).value_or(0);
    d.flags = gamelib::app::readWorldFlags(ini, "flags");
    return d;
}

std::string summary(const game::SaveData& d)
{
    const std::int64_t ticks = std::min<std::int64_t>(d.stats.ticks, 359'999LL * game::kTickRate);
    return std::format("LV {}  {}  {}{}", d.character.level, game::mapDef(d.map).name,
                       gamelib::core::formatClock(static_cast<int>(ticks), game::kTickRate, 0), d.finished ? "  *" : "");
}

} // namespace

Saves::Saves()
    : m_slots{gamelib::storage::userDataDir(kAppName, kDataDirVariable) / "saves", kSaveVersion, "wayfarer"}
{
}

std::vector<gamelib::ui::SlotRow> Saves::rows() const
{
    std::vector<gamelib::ui::SlotRow> rows;
    for (int slot = 0; slot < kSlotCount; ++slot) {
        const gamelib::storage::SaveSlotInfo info = m_slots.info(slotName(slot));
        rows.push_back({.exists = info.exists, .readable = info.readable, .summary = info.summary});
    }
    return rows;
}

std::optional<game::SaveData> Saves::load(const int slot)
{
    auto loaded = m_slots.load(slotName(slot));
    if (!loaded) {
        return std::nullopt;
    }
    return read(loaded->data);
}

void Saves::save(const int slot, const game::SaveData& data)
{
    (void)m_slots.save(slotName(slot), write(data), summary(data));
}

void Saves::erase(const int slot)
{
    (void)m_slots.remove(slotName(slot));
}

} // namespace wayfarer::app
