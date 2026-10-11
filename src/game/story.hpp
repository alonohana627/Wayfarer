#pragma once

// Hearthvale's people and the story: the six NPCs, the five chapter quests
// and ten side quests (GameLib's core::QuestBook), and every conversation
// as a core::DialogueTree whose conditions and effects are short strings the
// session interprets ("available 11", "accept 11", "turnin 11", "chapter 3",
// "night", "shop", "smith", "sleep morning").

#include "maps.hpp"
#include "rules.hpp"

#include <gamelib/core/dialogue.hpp>
#include <gamelib/core/quests.hpp>

#include <span>
#include <string>
#include <string_view>

namespace wayfarer::game {

[[nodiscard]] std::string_view npcName(NpcId npc);
[[nodiscard]] std::string_view npcRole(NpcId npc);
// The dialogue node a conversation with an NPC starts at.
[[nodiscard]] std::string npcRoot(NpcId npc);

// Quest ids: chapters 1..6 (1 is the arrival), side quests 11..20.
inline constexpr int kQuestArrival = 1;
inline constexpr int kQuestChapter1 = 2; // Grandcap
inline constexpr int kQuestChapter2 = 3; // Rumblerock
inline constexpr int kQuestChapter3 = 4; // Murkshell
inline constexpr int kQuestChapter4 = 5; // the Warden
inline constexpr int kQuestChapter5 = 6; // the Hush
inline constexpr int kQuestLetter = 13;
inline constexpr int kQuestStarCharm = 17;

// Objective subjects: species ids for monsters, kBossSubject + boss for guardians.
inline constexpr int kBossSubject = 100;

struct QuestText {
    int id = 0;
    std::string_view title;
    std::string_view summary; // one line for the quest log
};

[[nodiscard]] std::span<const gamelib::core::QuestDef> questDefs();
[[nodiscard]] const QuestText& questText(int id);
[[nodiscard]] const gamelib::core::DialogueTree& dialogueTree();
// The chapter quest whose guardian is `boss`.
[[nodiscard]] int chapterQuestFor(BossId boss);

} // namespace wayfarer::game
