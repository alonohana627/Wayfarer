# wayfarer_game: the rules - the character (stats, levels, skills, gear),
# items and loot, the maps of the town and the five regions, monsters and
# guardians, the story's quests and conversations, the session that runs a
# playthrough and the demo bot. Depends only on GameLib::core and
# GameLib::ai: no window, GPU, audio or clock, deterministic for the
# per-tick input.
add_library(wayfarer_game STATIC
    bosses.cpp
    character.cpp
    demo.cpp
    items.cpp
    maps.cpp
    maps_forest.cpp
    maps_regions.cpp
    maps_town.cpp
    monsters.cpp
    session.cpp
    story.cpp)
target_include_directories(wayfarer_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(wayfarer_game PUBLIC GameLib::core GameLib::ai)
gamelib_target_warnings(wayfarer_game)
