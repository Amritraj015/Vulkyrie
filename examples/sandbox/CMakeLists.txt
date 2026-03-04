file(GLOB_RECURSE SANDBOX_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(sandbox ${SANDBOX_SOURCES})

# Require C++23 properly
target_compile_features(sandbox PRIVATE cxx_std_23)

# Link engine
target_link_libraries(sandbox PRIVATE engine imgui::imgui)

# Copy assets after build
add_custom_command(
    TARGET sandbox
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
        $<TARGET_FILE_DIR:sandbox>/assets
    COMMENT "Copying sandbox assets"
)
