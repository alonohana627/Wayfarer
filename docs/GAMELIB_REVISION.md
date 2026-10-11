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
| 2026-10-11 | `ae5bcc0a15343f65b13fb11982049c5baf7a52a3` | the end of Wayfarer P0 batch 1: ability points, the stat window and item grids; the game now uses GameLib's hyper jump, scaledTuning, Shop::sell with a price and the Spawner runs |
| 2026-10-11 | `3174481f68a99df8554f0321d8c956f7db14b745` | Wayfarer P0 batch 2: drawParallaxSprites draws Hearthvale's back row (replaces the skyline loop) |
| 2026-10-11 | `df24f902c3eebdd21bf3a3845e5c331822042202` | Wayfarer P0 batch 3: fx::Banners draws the map title, GameClock::nightAmount, duskAmount and draw::lerp(ColorGrade) blend day and night |
| 2026-10-11 | `5cdebe0bde76da3f1970da4f83ac4ac5cd5525d5` | Wayfarer P0 batch 4: game-drawn frames for menus, save slots, the dialogue box and the stat window; item card icons; quest record saves (all adopted) |
| 2026-10-11 | `a8753e47a9a5dc9cdb72ad24f41e51183c0e28d3` | Wayfarer P0 batch 5: title painters replace the transparent-title trick; the orange points number, the bold "+" and the 30-size dialogue choices are back |
