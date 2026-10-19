add_library(rover_astar astar.cpp)
target_include_directories(rover_astar PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(rover_terrain terrain.cpp)
target_link_libraries(rover_terrain PUBLIC rover_astar)

add_library(rover_simulation simulation.cpp rover.cpp)
target_link_libraries(rover_simulation PUBLIC rover_astar)

add_library(rover_camera world_camera.cpp)
target_link_libraries(rover_camera PUBLIC rover_astar raylib)

add_library(rover_ui ui_glass.cpp ui_controls.cpp)
target_link_libraries(rover_ui PUBLIC raylib)
target_include_directories(rover_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(rover_visuals scene_renderer.cpp terrain_art.cpp world_art.cpp visual_effects.cpp rover_renderer.cpp rover_3d_pose.cpp rover_mesh.cpp sandstorm.cpp)
target_link_libraries(rover_visuals PUBLIC rover_simulation rover_camera rover_terrain rover_ui raylib)

add_library(rover_feedback audio_system.cpp drive_audio_synth.cpp mission_feedback.cpp mission_hud.cpp)
target_link_libraries(rover_feedback PUBLIC rover_visuals)

add_executable(mars_rover main.cpp terrain_panel.cpp)
target_link_libraries(mars_rover PRIVATE rover_feedback)

foreach(target rover_astar rover_terrain rover_simulation rover_camera rover_ui rover_visuals rover_feedback mars_rover)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

# Optional assets travel with the executable. All graphics/audio have procedural fallbacks.
add_custom_target(rover_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:mars_rover>/assets"
    VERBATIM)
add_dependencies(mars_rover rover_assets)
install(TARGETS mars_rover RUNTIME DESTINATION .)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/assets" DESTINATION .)
