# Wayfarer

A small action-RPG platformer in a cozy, MapleStory-inspired chibi anime
style: a hub town, Hearthvale, and five regions - Bellwood Forest, the
Glimmerdeep Mines, Murkmallow Marsh, the Sunspire Ruins and Frostcrown Peak -
each with a guardian at its heart. A hush has fallen over the land; the
wayfarer who arrives in town grows by experience, four core stats, skills
and gear, helps the townsfolk and brings the songs back.

Every picture is drawn by shaders (no image files) and every sound and song
is synthesized at startup. Built on [GameLib](https://github.com/alonohana627/GameLib)
(C++26, Vulkan, GLFW, miniaudio).

## Controls (rebindable under CONTROLS)

| Key | Action |
|---|---|
| Arrow keys | walk, climb ropes and ladders |
| Up | talk, rest at the inn, enter doors and waystones |
| Down + Z | drop through a foothold |
| Z / Space | jump; again in the air: hyper jump (hold Up to soar straight up) |
| X / Ctrl | attack (hold for the three-hit combo) |
| Shift / V | dodge roll |
| A S D F | Crescent Sweep, Starbolt, Tailwind, Bloom |
| Q / W | HP potion, MP potion (in the bag, Q / W on a potion puts it on that quick slot) |
| C K I L | stats, skills, bag & equipment, quest log |
| M | bigger minimap |
| Esc / P | pause |
| F11 | fullscreen |

A gamepad works too (A jump, X attack, B roll, bumpers, Y and the right
trigger for skills, the left trigger for a potion, Back for the bag, Start to
pause).

## Build and run

```powershell
cmake --preset release
cmake --build --preset release
build/release/bin/wayfarer.exe          # --demo: watch the demo bot
```

Clang, CMake 3.28+ and Ninja are needed; GameLib is fetched at a pinned commit
(`docs/GAMELIB_REVISION.md`). Saves and settings live in the user data folder
(`WAYFARER_DATA_DIR` moves them).
