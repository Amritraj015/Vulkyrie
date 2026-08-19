# ---------------------------
# Benchmark Sources
# ---------------------------
file(GLOB_RECURSE VULKYRIE_BENCHMARK_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(benchmarks ${VULKYRIE_BENCHMARK_SOURCES})

# ---------------------------
# Compile Settings
# ---------------------------
target_compile_features(benchmarks PRIVATE cxx_std_26)

target_link_libraries(benchmarks PRIVATE
    Catch2::Catch2WithMain
    engine
)

target_include_directories(benchmarks PRIVATE
    # White-box benchmarks (e.g. the Chase-Lev deque) measure private engine headers directly.
    ${CMAKE_SOURCE_DIR}/engine/src

    # Rooted at src/ so any benchmark, at any depth, includes the shared scaffolding the same way:
    # #include "support/benchmark_support.h".
    ${CMAKE_CURRENT_SOURCE_DIR}/src

    # The mock RendererBackend lives with the tests and is shared rather than duplicated: it is the only complete
    # implementation of the RendererBackend concept in the tree, so a second copy would silently drift the moment
    # the concept changes.
    ${CMAKE_SOURCE_DIR}/tests/src
)

# ---------------------------
# CTest Integration
# ---------------------------
# Deliberately none: benchmarks are a measuring tool, not a pass/fail gate, and registering them
# with CTest would put multi-second timing runs on the path of every `ctest` invocation. Run the
# binary directly - see benchmarks/README.md.
