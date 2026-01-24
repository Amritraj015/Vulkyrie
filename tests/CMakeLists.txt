include(FetchContent)

# ---------------------------
# Catch2 Dependency
# ---------------------------
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.12.0
)

FetchContent_MakeAvailable(Catch2)

# ---------------------------
# Test Sources
# ---------------------------
file(GLOB_RECURSE VULKYRIE_TEST_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(tests ${VULKYRIE_TEST_SOURCES})

# ---------------------------
# Compile Settings
# ---------------------------
target_compile_features(tests PRIVATE cxx_std_23)

target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
    engine
)

# ---------------------------
# CTest Integration
# ---------------------------
include(CTest)
include(Catch)

catch_discover_tests(tests)

# ---------------------------
# Assets (if needed)
# ---------------------------
add_custom_command(
    TARGET tests
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
        $<TARGET_FILE_DIR:tests>/assets
    COMMENT "Copying test assets"
)
