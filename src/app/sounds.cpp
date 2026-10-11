#include "sounds.hpp"

#include <array>
#include <string>
#include <vector>

namespace wayfarer::app {

namespace {

// helpers

using gamelib::audio::chord;
using gamelib::audio::Chord;
using gamelib::audio::chordPattern;
using gamelib::audio::note;
using gamelib::audio::noise;
using gamelib::audio::Note;
using gamelib::audio::Song;
using gamelib::audio::SoundRecipe;
using gamelib::audio::tone;
using gamelib::audio::Track;
using gamelib::audio::Wave;

Note rest(const float seconds)
{
    return tone(Wave::Pulse50, note("C4"), seconds, 0.0f, 0.0f);
}

// A soft song: a lead, an arpeggio of the chords, a held bass, a quiet pad and light drums.
Song song(const float bpm, const int stepsPerBeat, const std::string& lead, const std::vector<Chord>& chords,
          const std::string& arpFigure, const std::string& bassFigure, const std::string& drums, const Wave leadWave,
          const float leadVolume, const float peakDb)
{
    Song s{.bpm = bpm, .stepsPerBeat = stepsPerBeat, .tracks = {}, .peakDb = peakDb};
    s.tracks.push_back(Track{.wave = leadWave, .volume = leadVolume, .decay = 2.2f, .intro = {}, .loop = lead});
    s.tracks.push_back(Track{.wave = Wave::Pulse12, .volume = 0.2f, .decay = 4.5f, .intro = {}, .loop = chordPattern(chords, arpFigure, 1)});
    s.tracks.push_back(Track{.wave = Wave::Triangle, .volume = 0.85f, .decay = 1.0f, .intro = {}, .loop = chordPattern(chords, bassFigure)});
    if (!drums.empty()) {
        s.tracks.push_back(Track{.wave = Wave::Noise, .volume = 0.14f, .decay = 7.0f, .intro = {}, .loop = drums});
    }
    return s;
}

std::string times(const std::string& pattern, const int n)
{
    std::string out;
    for (int i = 0; i < n; ++i) {
        out += pattern + " | ";
    }
    return out;
}

// ---- Hearthvale: a gentle waltz in F (3/4, six eighths a bar).
const std::vector<Chord> kTownChords{chord("F"), chord("Dm"), chord("Bb"), chord("C"), chord("F"), chord("Am"), chord("Bb"), chord("C"),
                                     chord("Dm"), chord("Am"), chord("Bb"), chord("F"), chord("Gm"), chord("C"), chord("F"), chord("F")};
const std::string kTownLead = "A4 - C5 - F5 - | E5 - D5 - A4 - | D5 - - - C5 Bb4 | C5 - - - - - | "
                              "A4 - C5 - F5 - | G5 - E5 - C5 - | D5 - Bb4 - D5 - | E5 - - - - - | "
                              "F5 - E5 - D5 - | C5 - A4 - E5 - | D5 - C5 - Bb4 - | A4 - C5 - F5 - | "
                              "G5 - F5 - D5 - | E5 - G5 - Bb5 - | A5 - - - G5 - | F5 - - - . . |";

// ---- Bellwood Forest: a bouncy walk in G.
const std::vector<Chord> kForestChords{chord("G"), chord("Em"), chord("C"), chord("D"), chord("G"), chord("Em"), chord("Am"), chord("D")};
const std::string kForestLead = "G4 . B4 . D5 - B4 . | E5 . D5 . B4 - G4 . | C5 . E5 . G5 - E5 . | D5 - - . F#5 . A5 . | "
                                "G5 . F#5 . E5 - D5 . | B4 . D5 . G5 - . . | A4 . C5 . E5 - D5 C5 | D5 - - - - - . . |";

// ---- Glimmerdeep Mines: echoing and curious, A minor.
const std::vector<Chord> kMinesChords{chord("Am"), chord("F"), chord("G"), chord("Em"), chord("Am"), chord("F"), chord("Dm"), chord("E")};
const std::string kMinesLead = "A4 - - . C5 - E5 - | F5 - - - E5 - C5 - | D5 - - . B4 - G4 - | E5 - - - - - . . | "
                               "A5 - - . G5 - E5 - | F5 - - - C5 - A4 - | D5 - F5 - A5 - G5 - | G#5 - - - - - . . |";

// ---- Murkmallow Marsh: dreamy, D dorian, slow.
const std::vector<Chord> kMarshChords{chord("Dm"), chord("C"), chord("Bb"), chord("C"), chord("Dm"), chord("Am"), chord("Bb"), chord("A")};
const std::string kMarshLead = "D5 - - - F5 - - - | E5 - - - C5 - - - | D5 - - - Bb4 - A4 - | G4 - - - - - - - | "
                               "A4 - D5 - F5 - A5 - | G5 - - - E5 - - - | F5 - E5 - D5 - Bb4 - | C#5 - - - - - . . |";

// ---- Sunspire Ruins: warm and ancient, E minor with a raised seventh.
const std::vector<Chord> kRuinsChords{chord("Em"), chord("D"), chord("C"), chord("B"), chord("Em"), chord("Am"), chord("C"), chord("B")};
const std::string kRuinsLead = "E5 - - . G5 - F#5 - | D5 - - - A4 - - - | C5 - E5 - G5 - E5 - | D#5 - - - - - . . | "
                               "E5 - B4 - E5 - G5 - | A5 - - - C5 - E5 - | G5 - F#5 - E5 - C5 - | B4 - - - - - . . |";

// ---- Frostcrown Peak: airy bells, C major.
const std::vector<Chord> kPeakChords{chord("C"), chord("G"), chord("Am"), chord("F"), chord("C"), chord("Em"), chord("F"), chord("G")};
const std::string kPeakLead = "E5 - - - G5 - C6 - | B5 - - - D5 - G5 - | A5 - - - E5 - C5 - | F5 - - - - - A4 - | "
                              "G5 - - - E5 - C5 - | B4 - - - E5 - G5 - | A5 - G5 - F5 - E5 - | D5 - - - - - . . |";

// ---- Dungeons: hushed and low.
const std::vector<Chord> kDungeonChords{chord("Am"), chord("Am"), chord("F"), chord("E"), chord("Dm"), chord("Am"), chord("Bb"), chord("E")};
const std::string kDungeonLead = "A4 - - - . . E5 - | D5 - C5 - B4 - - - | A4 - - - . . F5 - | E5 - - - - - . . | "
                                 "D5 - - - . . F5 - | E5 - C5 - A4 - - - | Bb4 - D5 - F5 - - - | G#4 - - - - - . . |";

// ---- The guardians: driving.
const std::vector<Chord> kBossChords{chord("Cm"), chord("Ab"), chord("Bb"), chord("G"), chord("Cm"), chord("Ab"), chord("Fm"), chord("G")};
const std::string kBossLead = "C5 C5 Eb5 . G5 . C6 . | Ab5 - G5 . Eb5 . C5 . | Bb4 . D5 . F5 - Bb5 . | B5 - - . G5 . D5 . | "
                              "C6 . G5 . Eb5 . C5 . | Ab5 . C6 . Eb6 - C6 . | F5 . Ab5 . C6 - Ab5 . | G5 - B5 - D6 - - . |";
const std::vector<Chord> kFinalChords{chord("Dm"), chord("Bb"), chord("C"), chord("A"), chord("Dm"), chord("Gm"), chord("Bb"), chord("A")};
const std::string kFinalLead = "D5 . F5 . A5 . D6 . | Bb5 - A5 . F5 . D5 . | C5 . E5 . G5 - C6 . | C#6 - - . A5 . E5 . | "
                               "D6 . A5 . F5 . D5 . | G5 . Bb5 . D6 - Bb5 . | F5 . Bb5 . D6 - F6 . | E6 - C#6 - A5 - - . |";

// ---- The title: hopeful, C major.
const std::vector<Chord> kTitleChords{chord("C"), chord("Am"), chord("F"), chord("G"), chord("C"), chord("Em"), chord("F"), chord("G")};
const std::string kTitleLead = "E5 - - - D5 - C5 - | A4 - - - C5 - E5 - | F5 - - - E5 - D5 - | D5 - - - - - . . | "
                               "E5 - - - G5 - C6 - | B5 - - - G5 - E5 - | A5 - G5 - F5 - E5 - | D5 - - - G4 - . . |";

const std::string kWaltzBeat = "C3 . C7 . C7 .";

Song nightOf(Song s)
{
    // Night: slower, softer, the lead a music box an octave apart from the arps.
    s.bpm *= 0.82f;
    s.tracks[0].wave = Wave::Triangle;
    s.tracks[0].volume = 1.0f;
    s.tracks[0].decay = 3.5f;
    if (s.tracks.size() > 3) {
        s.tracks.pop_back(); // no drums
    }
    s.peakDb -= 1.5f;
    return s;
}

} // namespace

Tune regionTune(const game::Region region, const bool dungeon)
{
    if (dungeon) {
        return Tune::Dungeon;
    }
    switch (region) {
    case game::Region::Town: return Tune::Town;
    case game::Region::Forest: return Tune::Forest;
    case game::Region::Mines: return Tune::Mines;
    case game::Region::Marsh: return Tune::Marsh;
    case game::Region::Ruins: return Tune::Ruins;
    case game::Region::Peak: return Tune::Peak;
    case game::Region::Count: break;
    }
    return Tune::Town;
}

Song tune(const Tune t, const int variant)
{
    Song s;
    switch (t) {
    case Tune::Title:
        s = song(84.0f, 2, kTitleLead, kTitleChords, "1 3 5 8 5 3 1 3", "1 - - - 5 - - -", "", Wave::Pulse25, 0.6f, -17.0f);
        break;
    case Tune::Town:
        s = song(104.0f, 2, kTownLead, kTownChords, "1 5 8 5 3' 5", "1 - - - - -", times(kWaltzBeat, 16), Wave::Pulse25, 0.62f, -17.0f);
        break;
    case Tune::Forest:
        s = song(108.0f, 2, kForestLead, kForestChords, "1 5 8 5 1 5 8 5", "1 - 5, - 1 - 5, -", times("C3 . . . C7 . C7 .", 8),
                 Wave::Pulse25, 0.62f, -16.5f);
        break;
    case Tune::Mines:
        s = song(88.0f, 2, kMinesLead, kMinesChords, "1 5 8 3' 8 5 1 5", "1 - - - 1 - - -", times("C3 . . . . . C8 .", 8), Wave::Pulse12, 0.7f, -17.0f);
        break;
    case Tune::Marsh:
        s = song(76.0f, 2, kMarshLead, kMarshChords, "1 3 5 3 8 5 3 5", "1 - - - - - - -", "", Wave::Pulse25, 0.55f, -17.5f);
        break;
    case Tune::Ruins:
        s = song(92.0f, 2, kRuinsLead, kRuinsChords, "1 5 8 5 3 5 8 5", "1 - - 1 5, - - -", times("C3 . . C7 . . C6 .", 8), Wave::Pulse25,
                 0.6f, -17.0f);
        break;
    case Tune::Peak:
        s = song(80.0f, 2, kPeakLead, kPeakChords, "1 5 8 3' 5' 3' 8 5", "1 - - - 5, - - -", "", Wave::Pulse12, 0.72f, -17.5f);
        break;
    case Tune::Dungeon:
        s = song(72.0f, 2, kDungeonLead, kDungeonChords, "1 . 5 . 8 . 5 .", "1 - - - - - - -", times("C2 . . . . . . .", 8), Wave::Pulse12,
                 0.6f, -18.0f);
        break;
    case Tune::Boss:
        s = song(148.0f, 2, kBossLead, kBossChords, "1 5 8 5 1 5 8 5", "1 1 8, 1 1 1 8, 5,", times("C3 . C8 . C6 . C8 C8", 8), Wave::Pulse25,
                 0.75f, -15.5f);
        break;
    case Tune::FinalBoss:
        s = song(156.0f, 2, kFinalLead, kFinalChords, "1 5 8 5 3 5 8 5", "1 1 8, 1 1 1 8, 5,", times("C3 . C8 . C6 . C8 C8", 8),
                 Wave::Pulse25, 0.78f, -15.0f);
        break;
    case Tune::Ending:
        s = song(96.0f, 2, kTownLead, kTownChords, "1 5 8 5 3' 5", "1 - - 5, - -", times(kWaltzBeat, 16), Wave::Pulse25, 0.66f, -16.5f);
        break;
    case Tune::Count: break;
    }
    if (variant == 1) {
        return nightOf(s);
    }
    return s;
}

SoundRecipe sfxRecipe(const Sfx sfx)
{
    switch (sfx) {
    case Sfx::Jump: return {{{tone(Wave::Pulse25, note("D5"), 0.08f, note("A5"), 0.8f)}}, -24.0f};
    case Sfx::HyperJump:
        return {{{tone(Wave::Pulse12, note("A5"), 0.05f, note("E6")), tone(Wave::Pulse12, note("E6"), 0.1f, note("B6"))},
                 {noise(9000.0f, 0.18f, 3000.0f, 0.6f)}},
                -18.0f};
    case Sfx::Land: return {{{tone(Wave::Triangle, note("D3"), 0.05f, note("A2"))}, {noise(1600.0f, 0.04f, 600.0f, 0.35f)}}, -26.0f};
    case Sfx::Swing: return {{{noise(12000.0f, 0.11f, 3500.0f, 0.9f)}, {tone(Wave::Pulse12, note("C6"), 0.05f, note("G5"), 0.3f)}}, -21.0f};
    case Sfx::Hit:
        return {{{tone(Wave::Pulse25, note("G5"), 0.04f, note("C5"))}, {noise(6000.0f, 0.06f, 1800.0f, 0.8f)}}, -18.0f};
    case Sfx::Crit:
        return {{{tone(Wave::Pulse25, note("C6"), 0.04f, note("G5")), tone(Wave::Pulse12, note("G6"), 0.08f, note("C6"))},
                 {noise(8000.0f, 0.1f, 1500.0f, 0.9f)}},
                -16.0f};
    case Sfx::MonsterDown:
        return {{{tone(Wave::Pulse25, note("E5"), 0.06f), tone(Wave::Pulse25, note("C5"), 0.06f), tone(Wave::Pulse25, note("G4"), 0.12f, note("D4"))},
                 {noise(3000.0f, 0.2f, 300.0f, 0.5f)}},
                -18.0f};
    case Sfx::Hurt:
        return {{{tone(Wave::Pulse25, note("A5"), 0.05f, note("E5")), tone(Wave::Pulse25, note("D5"), 0.12f, note("A4"))},
                 {noise(2600.0f, 0.12f, 600.0f, 0.5f)}},
                -16.0f};
    case Sfx::Death:
        return {{{tone(Wave::Pulse25, note("E5"), 0.14f), tone(Wave::Pulse25, note("C5"), 0.14f), tone(Wave::Pulse25, note("A4"), 0.14f),
                  tone(Wave::Triangle, note("F4"), 0.6f, note("C3"))}},
                -15.0f};
    case Sfx::LevelUp:
        return {{{tone(Wave::Pulse25, note("C5"), 0.09f), tone(Wave::Pulse25, note("E5"), 0.09f), tone(Wave::Pulse25, note("G5"), 0.09f),
                  tone(Wave::Pulse25, note("C6"), 0.18f), tone(Wave::Pulse25, note("G5"), 0.09f), tone(Wave::Pulse25, note("C6"), 0.45f)},
                 {tone(Wave::Pulse12, note("E4"), 0.27f), tone(Wave::Pulse12, note("G4"), 0.27f), tone(Wave::Pulse12, note("E5"), 0.54f)},
                 {tone(Wave::Triangle, note("C3"), 0.36f), tone(Wave::Triangle, note("G3"), 0.36f), tone(Wave::Triangle, note("C4"), 0.5f)}},
                -12.0f};
    case Sfx::Pickup: return {{{tone(Wave::Pulse25, note("A5"), 0.05f), tone(Wave::Pulse25, note("E6"), 0.08f)}}, -22.0f};
    case Sfx::PickupRare:
        return {{{tone(Wave::Pulse25, note("C6"), 0.06f), tone(Wave::Pulse25, note("E6"), 0.06f), tone(Wave::Pulse25, note("G6"), 0.12f)}}, -19.0f};
    case Sfx::PickupEpic:
        return {{{tone(Wave::Pulse25, note("C6"), 0.06f), tone(Wave::Pulse25, note("E6"), 0.06f), tone(Wave::Pulse25, note("G6"), 0.06f),
                  tone(Wave::Pulse25, note("B6"), 0.2f)},
                 {rest(0.06f), tone(Wave::Pulse12, note("G5"), 0.32f)}},
                -17.0f};
    case Sfx::PickupLegend:
        return {{{tone(Wave::Pulse25, note("G5"), 0.07f), tone(Wave::Pulse25, note("C6"), 0.07f), tone(Wave::Pulse25, note("E6"), 0.07f),
                  tone(Wave::Pulse25, note("G6"), 0.07f), tone(Wave::Pulse25, note("C7"), 0.35f)},
                 {tone(Wave::Pulse12, note("C5"), 0.28f), tone(Wave::Pulse12, note("E5"), 0.35f)},
                 {tone(Wave::Triangle, note("C4"), 0.63f)}},
                -14.0f};
    case Sfx::Coin: return {{{tone(Wave::Pulse12, note("B5"), 0.04f), tone(Wave::Pulse12, note("E6"), 0.1f)}}, -23.0f};
    case Sfx::Potion: return {{{tone(Wave::Triangle, note("C5"), 0.08f, note("G5")), tone(Wave::Pulse12, note("C6"), 0.12f, note("E6"))}}, -19.0f};
    case Sfx::Sweep:
        return {{{noise(10000.0f, 0.35f, 2000.0f, 1.0f)}, {tone(Wave::Pulse25, note("C5"), 0.3f, note("C6"), 0.5f)}}, -15.0f};
    case Sfx::Star:
        return {{{tone(Wave::Pulse12, note("E6"), 0.05f), tone(Wave::Pulse12, note("B6"), 0.05f), tone(Wave::Pulse12, note("E7"), 0.12f, note("A6"))},
                 {noise(7000.0f, 0.15f, 9000.0f, 0.3f)}},
                -17.0f};
    case Sfx::Wind:
        return {{{noise(4000.0f, 0.5f, 9000.0f, 0.7f)}, {tone(Wave::Pulse12, note("G5"), 0.4f, note("D6"), 0.4f)}}, -17.0f};
    case Sfx::Bloom:
        return {{{tone(Wave::Pulse25, note("C5"), 0.08f), tone(Wave::Pulse25, note("E5"), 0.08f), tone(Wave::Pulse25, note("G5"), 0.08f),
                  tone(Wave::Pulse25, note("C6"), 0.3f)},
                 {tone(Wave::Triangle, note("C4"), 0.54f)}},
                -16.0f};
    case Sfx::Roll: return {{{noise(3000.0f, 0.18f, 800.0f, 0.6f)}, {tone(Wave::Triangle, note("G3"), 0.15f, note("C3"))}}, -21.0f};
    case Sfx::NoMana: return {{{tone(Wave::Pulse12, note("E4"), 0.06f), rest(0.03f), tone(Wave::Pulse12, note("C4"), 0.08f)}}, -22.0f};
    case Sfx::Portal:
        return {{{tone(Wave::Pulse25, note("C5"), 0.4f, note("C6"), 0.6f)}, {noise(5000.0f, 0.4f, 12000.0f, 0.4f)},
                 {tone(Wave::Triangle, note("G4"), 0.4f, note("G5"))}},
                -16.0f};
    case Sfx::Sealed: return {{{tone(Wave::Pulse12, note("C4"), 0.07f), rest(0.03f), tone(Wave::Pulse12, note("C4"), 0.09f)}}, -21.0f};
    case Sfx::QuestAccept:
        return {{{tone(Wave::Pulse25, note("G5"), 0.07f), tone(Wave::Pulse25, note("C6"), 0.14f)}}, -18.0f};
    case Sfx::QuestComplete:
        return {{{tone(Wave::Pulse25, note("E5"), 0.07f), tone(Wave::Pulse25, note("G5"), 0.07f), tone(Wave::Pulse25, note("E6"), 0.2f)}}, -17.0f};
    case Sfx::QuestDone:
        return {{{tone(Wave::Pulse25, note("C5"), 0.08f), tone(Wave::Pulse25, note("E5"), 0.08f), tone(Wave::Pulse25, note("G5"), 0.08f),
                  tone(Wave::Pulse25, note("E6"), 0.08f), tone(Wave::Pulse25, note("C6"), 0.3f)},
                 {tone(Wave::Triangle, note("C4"), 0.32f), tone(Wave::Triangle, note("G3"), 0.3f)}},
                -15.0f};
    case Sfx::Learn:
        return {{{tone(Wave::Pulse12, note("D6"), 0.06f), tone(Wave::Pulse12, note("A6"), 0.06f), tone(Wave::Pulse12, note("D7"), 0.16f)}}, -17.0f};
    case Sfx::Point: return {{{tone(Wave::Pulse25, note("E6"), 0.05f, note("A6"))}}, -19.0f};
    case Sfx::Equip:
        return {{{noise(5000.0f, 0.05f, 2000.0f, 0.6f)}, {tone(Wave::Pulse12, note("A5"), 0.06f), tone(Wave::Pulse12, note("D6"), 0.08f)}}, -19.0f};
    case Sfx::Upgrade:
        return {{{noise(9000.0f, 0.06f, 4000.0f, 1.0f), rest(0.1f), noise(9000.0f, 0.06f, 4000.0f, 1.0f)},
                 {tone(Wave::Pulse12, note("E6"), 0.06f), rest(0.1f), tone(Wave::Pulse12, note("E6"), 0.06f), tone(Wave::Pulse25, note("B6"), 0.3f)}},
                -15.0f};
    case Sfx::Buy: return {{{tone(Wave::Pulse12, note("E6"), 0.04f), tone(Wave::Pulse12, note("B6"), 0.09f)}}, -20.0f};
    case Sfx::Sell: return {{{tone(Wave::Pulse12, note("B6"), 0.04f), tone(Wave::Pulse12, note("E6"), 0.09f)}}, -20.0f};
    case Sfx::MenuMove: return {{{tone(Wave::Pulse12, note("A5"), 0.03f)}}, -26.0f};
    case Sfx::MenuSelect: return {{{tone(Wave::Pulse25, note("E5"), 0.04f), tone(Wave::Pulse25, note("A5"), 0.07f)}}, -21.0f};
    case Sfx::MenuBack: return {{{tone(Wave::Pulse25, note("A5"), 0.04f), tone(Wave::Pulse25, note("E5"), 0.06f)}}, -22.0f};
    case Sfx::Error: return {{{tone(Wave::Pulse12, note("C4"), 0.08f, note("A3"))}}, -21.0f};
    case Sfx::Type: return {{{tone(Wave::Triangle, note("A5"), 0.025f)}}, -26.0f};
    case Sfx::Open: return {{{tone(Wave::Pulse25, note("C5"), 0.04f, note("G5"), 0.7f), tone(Wave::Pulse12, note("C6"), 0.05f)}}, -21.0f};
    case Sfx::Close: return {{{tone(Wave::Pulse25, note("G5"), 0.04f, note("C5"), 0.7f)}}, -22.0f};
    case Sfx::BossRoar:
        return {{{tone(Wave::Pulse25, note("A3"), 0.3f, note("E3"))}, {noise(800.0f, 0.4f, 200.0f, 0.7f)}}, -15.0f};
    case Sfx::BossLand: return {{{noise(1200.0f, 0.35f, 100.0f, 1.0f)}, {tone(Wave::Triangle, note("E2"), 0.3f, note("A1"))}}, -13.0f};
    case Sfx::BossDown:
        return {{{noise(2000.0f, 0.9f, 80.0f, 1.0f)}, {tone(Wave::Triangle, note("A3"), 0.9f, note("A1"))},
                 {tone(Wave::Pulse25, note("E5"), 0.15f), tone(Wave::Pulse25, note("C5"), 0.15f), tone(Wave::Pulse25, note("A4"), 0.5f)}},
                -12.0f};
    case Sfx::Lunge: return {{{tone(Wave::Pulse12, note("D5"), 0.08f, note("A5"), 0.5f)}}, -24.0f};
    case Sfx::Shot: return {{{tone(Wave::Pulse12, note("G5"), 0.1f, note("C5"), 0.6f)}, {noise(4000.0f, 0.08f, 1500.0f, 0.3f)}}, -21.0f};
    case Sfx::RainWarn: return {{{tone(Wave::Pulse12, note("E6"), 0.06f), tone(Wave::Pulse12, note("E6"), 0.06f, 0.0f, 0.0f), tone(Wave::Pulse12, note("E6"), 0.06f)}}, -22.0f};
    case Sfx::Sleep:
        return {{{tone(Wave::Triangle, note("E5"), 0.25f), tone(Wave::Triangle, note("C5"), 0.25f), tone(Wave::Triangle, note("D5"), 0.25f),
                  tone(Wave::Triangle, note("G4"), 0.6f)},
                 {tone(Wave::Triangle, note("C4"), 1.35f)}},
                -16.0f};
    case Sfx::Spawn: return {{{tone(Wave::Pulse12, note("C5"), 0.06f, note("G5"), 0.5f)}}, -26.0f};
    case Sfx::Chapter:
        return {{{tone(Wave::Pulse25, note("G4"), 0.12f), tone(Wave::Pulse25, note("C5"), 0.12f), tone(Wave::Pulse25, note("E5"), 0.12f),
                  tone(Wave::Pulse25, note("G5"), 0.24f), tone(Wave::Pulse25, note("E5"), 0.12f), tone(Wave::Pulse25, note("G5"), 0.6f)},
                 {tone(Wave::Pulse12, note("E4"), 0.36f), tone(Wave::Pulse12, note("G4"), 0.36f), tone(Wave::Pulse12, note("C5"), 0.72f)},
                 {tone(Wave::Triangle, note("C3"), 0.72f), tone(Wave::Triangle, note("C3"), 0.72f)}},
                -12.0f};
    case Sfx::Guard: return {{{tone(Wave::Pulse12, note("F7"), 0.06f, note("C7"))}, {noise(9000.0f, 0.04f)}}, -20.0f};
    case Sfx::Count: break;
    }
    return {{{tone(Wave::Pulse12, note("C5"), 0.05f)}}, -24.0f};
}

} // namespace wayfarer::app
