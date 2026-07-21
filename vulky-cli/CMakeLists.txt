file(GLOB_RECURSE VULKY_CLI_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(vulky ${VULKY_CLI_SOURCES})

# Require C++26 properly
target_compile_features(vulky PRIVATE cxx_std_26)

# Link engine
target_link_libraries(vulky PRIVATE engine)
