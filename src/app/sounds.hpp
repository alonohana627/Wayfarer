#pragma once

// Wayfarer's sound effects and music, all synthesized with GameLib's 8-bit
// synth at startup: no audio files. Soft region themes with a night take
// each (GameLib's MusicBank variants), a cozy town waltz, the dungeons, the
// guardians, the title and the ending; combat sounds, pickup chimes by
// rarity, a level-up fanfare and UI clicks.

#include "rules.hpp"

#include <gamelib/audio/music.hpp>
#include <gamelib/audio/player.hpp>
#include <gamelib/audio/sound_bank.hpp>
#include <gamelib/audio/synth.hpp>

#include <cstddef>
#include <cstdint>

namespace wayfarer::app {

enum class Sfx : std::uint8_t {
    Jump, HyperJump, Land, Swing, Hit, Crit, MonsterDown, Hurt, Death, LevelUp,
    Pickup, PickupRare, PickupEpic, PickupLegend, Coin, Potion,
    Sweep, Star, Wind, Bloom, Roll, NoMana,
    Portal, Sealed, QuestAccept, QuestComplete, QuestDone, Learn, Point, Equip, Upgrade, Buy, Sell,
    MenuMove, MenuSelect, MenuBack, Error, Type, Open, Close,
    BossRoar, BossLand, BossDown, Lunge, Shot, RainWarn, Sleep, Spawn, Chapter, Guard,
    Count
};
inline constexpr std::size_t kSfxCount = static_cast<std::size_t>(Sfx::Count);

[[nodiscard]] gamelib::audio::SoundRecipe sfxRecipe(Sfx sfx);

enum class Tune : std::uint8_t { Title, Town, Forest, Mines, Marsh, Ruins, Peak, Dungeon, Boss, FinalBoss, Ending, Count };
inline constexpr std::size_t kTuneCount = static_cast<std::size_t>(Tune::Count);

// Variant 0 by day, 1 at night (MusicBank falls back to 0 where there is none).
[[nodiscard]] gamelib::audio::Song tune(Tune tune, int variant);
[[nodiscard]] Tune regionTune(game::Region region, bool dungeon);

using Music = gamelib::audio::MusicBank<Tune, kTuneCount>;
using Sounds = gamelib::audio::SoundBank<Sfx, kSfxCount>;

} // namespace wayfarer::app
