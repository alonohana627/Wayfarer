#pragma once

// The three save slots (GameLib's storage::SaveSlots: a versioned file per
// slot with a checksum and a backup) holding a game::SaveData: the whole
// character (level, stats, skills, bag, gear, coins, quick slots), the quests,
// the world flags, where the hero stands, the clock and the run's numbers.
// The slot list shows each slot's summary line.

#include "session.hpp"

#include <gamelib/storage/save_slots.hpp>
#include <gamelib/ui/save_slot_list.hpp>

#include <optional>
#include <vector>

namespace wayfarer::app {

inline constexpr int kSlotCount = 3;

class Saves {
public:
    Saves();

    [[nodiscard]] std::vector<gamelib::ui::SlotRow> rows() const;
    // nullopt for an empty or unreadable slot.
    [[nodiscard]] std::optional<game::SaveData> load(int slot);
    void save(int slot, const game::SaveData& data);
    void erase(int slot);

private:
    gamelib::storage::SaveSlots m_slots;
};

} // namespace wayfarer::app
