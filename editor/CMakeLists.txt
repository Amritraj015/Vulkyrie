file(GLOB_RECURSE VULKYRIE_EDITOR_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Declare the editor executable
add_executable(editor ${VULKYRIE_EDITOR_SOURCES})

# Require C++26 properly
target_compile_features(editor PRIVATE cxx_std_26)

# Link engine and other dependencies
target_link_libraries(editor PRIVATE engine imgui::imgui)
