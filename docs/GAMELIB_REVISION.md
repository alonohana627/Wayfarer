# GameLib revision

Wayfarer fetches GameLib from GitHub at a pinned commit (`CMakeLists.txt`,
`FetchContent_Declare(GameLib ... GIT_TAG <hash>)`). Each row is a GameLib
commit the game was built and run with (zero warnings, zero validation
messages); bump the pin only together with a new row.

| Date | GameLib commit | Why |
|---|---|---|
| 2026-10-11 | `333092bffeabbb8fe5805e3efc24ffd8796fe751` (release 0.0.10) | the newest GameLib main when Wayfarer started |
