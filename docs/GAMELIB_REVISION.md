# GameLib revision

Wayfarer fetches GameLib from GitHub at a pinned commit (`CMakeLists.txt`,
`FetchContent_Declare(GameLib ... GIT_TAG <hash>)`). Each row is a GameLib
commit the game was built and run with (zero warnings, zero validation
messages); bump the pin only together with a new row.

| Date | GameLib commit | Why |
|---|---|---|
| 2026-10-11 | `333092bffeabbb8fe5805e3efc24ffd8796fe751` (release 0.0.10) | the newest GameLib main when Wayfarer started |
| 2026-10-11 | `c407fa08fe177a1b90566c19ddac9173a91cae4e` | the audio device opens and closes off the main thread: launches and quits no longer stall ~30 s |
| 2026-10-11 | `c65adcf3778e70720c49be55bb2221e3450f8eb5` | Wayfarer P0 batch 1 so far: fonts for menus and dialogue, RPG container saves, damage numbers drawn with the game's digits (ported), movement stats, the hyper jump, sell prices and field spawns (adopted in the next steps) |
