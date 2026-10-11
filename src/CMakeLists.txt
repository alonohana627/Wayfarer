add_subdirectory(game)
add_subdirectory(app)

add_executable(wayfarer main.cpp)
target_link_libraries(wayfarer PRIVATE wayfarer_app)
gamelib_target_warnings(wayfarer)
gamelib_copy_runtime(wayfarer)
