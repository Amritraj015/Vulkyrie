file(GLOB_RECURSE ASTEROIDS_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(asteroids ${ASTEROIDS_SOURCES})

# Require C++23 properly
target_compile_features(asteroids PRIVATE cxx_std_23)

# Link engine
target_link_libraries(asteroids PRIVATE engine)
