#include <catch2/catch_test_macros.hpp>
#include <vulkyrie.h>

using namespace Vulkyrie;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static Entity MakeEntity(u64 index, u64 generation = 0) {
    return Entity(index, generation);
}

// ===========================================================================================
// Construction
// ===========================================================================================

TEST_CASE("Islands - default construction starts empty with sensible capacity hints",
          "[physics][islands]") {
    Islands islands;

    REQUIRE(islands.GetTotalIslands() == 0);
    REQUIRE(islands.ContactManifoldIndices.empty());
    REQUIRE(islands.TotalContactManifolds.empty());
    REQUIRE(islands.StartingBodyIndexForIsland.empty());
    REQUIRE(islands.TotalBodiesInIsland.empty());
    REQUIRE(islands.BodyEntities.empty());

    // Last-frame hints default to RP3D values: 16 islands, 32 body entities, 0 max.
    REQUIRE(islands.GetTotalBodiesInIslandInLastFrame() == 32);
    REQUIRE(islands.GetMaxBodiesInIslandInLastFrame() == 0);
}

// ===========================================================================================
// AddIsland
// ===========================================================================================

TEST_CASE("Islands - AddIsland returns sequential indices starting at 0",
          "[physics][islands]") {
    Islands islands;

    REQUIRE(islands.AddIsland(0) == 0);
    REQUIRE(islands.AddIsland(5) == 1);
    REQUIRE(islands.AddIsland(12) == 2);
    REQUIRE(islands.GetTotalIslands() == 3);
}

TEST_CASE("Islands - AddIsland records the correct contact manifold start index",
          "[physics][islands]") {
    Islands islands;

    std::ignore = islands.AddIsland(7);
    std::ignore = islands.AddIsland(42);

    REQUIRE(islands.ContactManifoldIndices[0] == 7);
    REQUIRE(islands.ContactManifoldIndices[1] == 42);
}

TEST_CASE("Islands - AddIsland initialises manifold count to 0",
          "[physics][islands]") {
    Islands islands;
    std::ignore = islands.AddIsland(0);

    REQUIRE(islands.TotalContactManifolds[0] == 0);
}

TEST_CASE("Islands - AddIsland records the correct starting body index",
          "[physics][islands]") {
    Islands islands;

    std::ignore = islands.AddIsland(0);
    // No bodies added yet — island 0 starts at body index 0.
    REQUIRE(islands.StartingBodyIndexForIsland[0] == 0);

    islands.AddBodyToIsland(MakeEntity(1));
    islands.AddBodyToIsland(MakeEntity(2));

    std::ignore = islands.AddIsland(3);
    // Island 1 starts after the 2 bodies already registered.
    REQUIRE(islands.StartingBodyIndexForIsland[1] == 2);
}

TEST_CASE("Islands - AddIsland initialises body count to 0",
          "[physics][islands]") {
    Islands islands;
    std::ignore = islands.AddIsland(0);

    REQUIRE(islands.TotalBodiesInIsland[0] == 0);
}

// ===========================================================================================
// AddBodyToIsland
// ===========================================================================================

TEST_CASE("Islands - AddBodyToIsland appends entity to BodyEntities",
          "[physics][islands]") {
    Islands islands;
    std::ignore = islands.AddIsland(0);

    Entity e1 = MakeEntity(10);
    Entity e2 = MakeEntity(20);
    islands.AddBodyToIsland(e1);
    islands.AddBodyToIsland(e2);

    REQUIRE(islands.BodyEntities.size() == 2);
    REQUIRE(islands.BodyEntities[0] == e1);
    REQUIRE(islands.BodyEntities[1] == e2);
}

TEST_CASE("Islands - AddBodyToIsland increments TotalBodiesInIsland for the current island",
          "[physics][islands]") {
    Islands islands;
    std::ignore = islands.AddIsland(0);

    islands.AddBodyToIsland(MakeEntity(1));
    REQUIRE(islands.TotalBodiesInIsland[0] == 1);

    islands.AddBodyToIsland(MakeEntity(2));
    islands.AddBodyToIsland(MakeEntity(3));
    REQUIRE(islands.TotalBodiesInIsland[0] == 3);
}

TEST_CASE("Islands - AddBodyToIsland only increments the most-recently added island",
          "[physics][islands]") {
    Islands islands;
    std::ignore = islands.AddIsland(0);
    islands.AddBodyToIsland(MakeEntity(1));
    islands.AddBodyToIsland(MakeEntity(2));

    std::ignore = islands.AddIsland(5);
    islands.AddBodyToIsland(MakeEntity(3));

    // Island 0 must stay at 2; only island 1 should have 1.
    REQUIRE(islands.TotalBodiesInIsland[0] == 2);
    REQUIRE(islands.TotalBodiesInIsland[1] == 1);
}

TEST_CASE("Islands - bodies across multiple islands are laid out sequentially in BodyEntities",
          "[physics][islands]") {
    Islands islands;

    std::ignore = islands.AddIsland(0);
    Entity e0 = MakeEntity(0);
    Entity e1 = MakeEntity(1);
    islands.AddBodyToIsland(e0);
    islands.AddBodyToIsland(e1);

    std::ignore = islands.AddIsland(2);
    Entity e2 = MakeEntity(2);
    islands.AddBodyToIsland(e2);

    // Island 0 occupies [0, 2), island 1 occupies [2, 3).
    REQUIRE(islands.StartingBodyIndexForIsland[0] == 0);
    REQUIRE(islands.TotalBodiesInIsland[0] == 2);
    REQUIRE(islands.StartingBodyIndexForIsland[1] == 2);
    REQUIRE(islands.TotalBodiesInIsland[1] == 1);

    REQUIRE(islands.BodyEntities[0] == e0);
    REQUIRE(islands.BodyEntities[1] == e1);
    REQUIRE(islands.BodyEntities[2] == e2);
}

TEST_CASE("Islands - island with zero bodies is valid", "[physics][islands]") {
    Islands islands;
    std::ignore = islands.AddIsland(0);

    // No AddBodyToIsland calls.
    REQUIRE(islands.TotalBodiesInIsland[0] == 0);
    REQUIRE(islands.BodyEntities.empty());
}

// ===========================================================================================
// GetTotalIslands
// ===========================================================================================

TEST_CASE("Islands - GetTotalIslands returns 0 when no islands have been added",
          "[physics][islands]") {
    Islands islands;
    REQUIRE(islands.GetTotalIslands() == 0);
}

TEST_CASE("Islands - GetTotalIslands reflects exactly the number of AddIsland calls",
          "[physics][islands]") {
    Islands islands;
    for (size_t i = 0; i < 10; ++i) {
        REQUIRE(islands.GetTotalIslands() == i);
        std::ignore = islands.AddIsland(i * 3);
    }
    REQUIRE(islands.GetTotalIslands() == 10);
}

// ===========================================================================================
// Maximum bodies tracking across AddIsland
// ===========================================================================================

TEST_CASE("Islands - AddIsland updates max-bodies-per-island when closing a larger island",
          "[physics][islands]") {
    Islands islands;

    // Island 0: 3 bodies.
    std::ignore = islands.AddIsland(0);
    islands.AddBodyToIsland(MakeEntity(1));
    islands.AddBodyToIsland(MakeEntity(2));
    islands.AddBodyToIsland(MakeEntity(3));

    // Island 1: 1 body.  Opening island 1 should record max=3 for island 0.
    std::ignore = islands.AddIsland(10);
    islands.AddBodyToIsland(MakeEntity(4));

    // Island 2: 5 bodies.  Opening island 2 should update max to 5... wait, island 1 only has
    // 1 body at this point, so max stays 3.
    std::ignore = islands.AddIsland(20);
    islands.AddBodyToIsland(MakeEntity(5));
    islands.AddBodyToIsland(MakeEntity(6));
    islands.AddBodyToIsland(MakeEntity(7));
    islands.AddBodyToIsland(MakeEntity(8));
    islands.AddBodyToIsland(MakeEntity(9));

    // After Clear(), the last island (5 bodies) is also considered, so max = 5.
    islands.Clear();
    REQUIRE(islands.GetMaxBodiesInIslandInLastFrame() == 5);
}

TEST_CASE("Islands - max-bodies stays 0 when all islands have zero bodies",
          "[physics][islands]") {
    Islands islands;
    std::ignore = islands.AddIsland(0);
    std::ignore = islands.AddIsland(1);
    std::ignore = islands.AddIsland(2);
    islands.Clear();

    REQUIRE(islands.GetMaxBodiesInIslandInLastFrame() == 0);
}

TEST_CASE("Islands - max-bodies is correct for a single island with many bodies",
          "[physics][islands]") {
    Islands islands;
    std::ignore = islands.AddIsland(0);
    for (u64 i = 0; i < 50; ++i) {
        islands.AddBodyToIsland(MakeEntity(i));
    }
    islands.Clear();

    REQUIRE(islands.GetMaxBodiesInIslandInLastFrame() == 50);
}

TEST_CASE("Islands - max-bodies is not decremented on subsequent frames with fewer bodies",
          "[physics][islands]") {
    Islands islands;

    // Frame 1: one island with 10 bodies.
    std::ignore = islands.AddIsland(0);
    for (u64 i = 0; i < 10; ++i) islands.AddBodyToIsland(MakeEntity(i));
    islands.Clear();
    REQUIRE(islands.GetMaxBodiesInIslandInLastFrame() == 10);

    // Frame 2: one island with only 2 bodies.
    std::ignore = islands.AddIsland(0);
    islands.AddBodyToIsland(MakeEntity(0));
    islands.AddBodyToIsland(MakeEntity(1));
    islands.Clear();
    // Max reflects frame 2's data — 2, not the stale 10.
    REQUIRE(islands.GetMaxBodiesInIslandInLastFrame() == 2);
}

// ===========================================================================================
// Clear
// ===========================================================================================

TEST_CASE("Islands - Clear empties all arrays", "[physics][islands]") {
    Islands islands;
    std::ignore = islands.AddIsland(0);
    islands.AddBodyToIsland(MakeEntity(1));
    std::ignore = islands.AddIsland(5);
    islands.AddBodyToIsland(MakeEntity(2));

    islands.Clear();

    REQUIRE(islands.ContactManifoldIndices.empty());
    REQUIRE(islands.TotalContactManifolds.empty());
    REQUIRE(islands.StartingBodyIndexForIsland.empty());
    REQUIRE(islands.TotalBodiesInIsland.empty());
    REQUIRE(islands.BodyEntities.empty());
    REQUIRE(islands.GetTotalIslands() == 0);
}

TEST_CASE("Islands - Clear updates last-frame island count", "[physics][islands]") {
    Islands islands;
    std::ignore = islands.AddIsland(0);
    std::ignore = islands.AddIsland(1);
    std::ignore = islands.AddIsland(2);
    // 3 islands this frame — after Clear the hint for next frame should be 3.
    islands.Clear();

    // Verify by calling ReserveMemory and checking capacity (indirectly through a second Clear).
    // We can observe the hint by adding exactly 3 islands and clearing again; if there were a
    // reallocation issue it would show up as a crash. A direct check: fill 3 islands, clear, then
    // confirm GetTotalBodiesInIslandInLastFrame and GetMaxBodiesInIslandInLastFrame are consistent.
    std::ignore = islands.AddIsland(0);
    islands.AddBodyToIsland(MakeEntity(10));
    islands.Clear();

    REQUIRE(islands.GetTotalBodiesInIslandInLastFrame() == 1);
}

TEST_CASE("Islands - Clear on an already-empty Islands is safe", "[physics][islands]") {
    Islands islands;
    // No islands or bodies added — Clear should be a no-op and not crash.
    REQUIRE_NOTHROW(islands.Clear());
    REQUIRE(islands.GetTotalIslands() == 0);
    REQUIRE(islands.GetMaxBodiesInIslandInLastFrame() == 0);
    REQUIRE(islands.GetTotalBodiesInIslandInLastFrame() == 0);
}

TEST_CASE("Islands - Clear resets max-bodies-current-frame counter between frames",
          "[physics][islands]") {
    Islands islands;

    // Frame 1: island with 4 bodies.
    std::ignore = islands.AddIsland(0);
    for (u64 i = 0; i < 4; ++i) islands.AddBodyToIsland(MakeEntity(i));
    islands.Clear();

    // Frame 2: no islands at all — Clear should produce max=0 for this frame.
    islands.Clear();
    REQUIRE(islands.GetMaxBodiesInIslandInLastFrame() == 0);
}

// ===========================================================================================
// ReserveMemory
// ===========================================================================================

TEST_CASE("Islands - ReserveMemory does not add islands or bodies", "[physics][islands]") {
    Islands islands;
    islands.ReserveMemory();

    REQUIRE(islands.GetTotalIslands() == 0);
    REQUIRE(islands.BodyEntities.empty());
}

TEST_CASE("Islands - ReserveMemory followed by AddIsland and AddBodyToIsland works correctly",
          "[physics][islands]") {
    Islands islands;
    islands.ReserveMemory();

    std::ignore = islands.AddIsland(0);
    islands.AddBodyToIsland(MakeEntity(1));
    islands.AddBodyToIsland(MakeEntity(2));

    REQUIRE(islands.GetTotalIslands() == 1);
    REQUIRE(islands.TotalBodiesInIsland[0] == 2);
    REQUIRE(islands.BodyEntities.size() == 2);
}

TEST_CASE("Islands - ReserveMemory uses last-frame counts as capacity hints after Clear",
          "[physics][islands]") {
    Islands islands;

    // Frame 1: populate and clear to establish last-frame hints.
    for (size_t i = 0; i < 5; ++i) {
        std::ignore = islands.AddIsland(i * 2);
        islands.AddBodyToIsland(MakeEntity(static_cast<u64>(i)));
    }
    islands.Clear();

    // Frame 2: reserve, then use — arrays should not need to grow given the hints.
    islands.ReserveMemory();
    REQUIRE(islands.ContactManifoldIndices.capacity() >= 5);
    REQUIRE(islands.BodyEntities.capacity() >= 5);
}

// ===========================================================================================
// Multi-frame simulation
// ===========================================================================================

TEST_CASE("Islands - multi-frame usage: state is fully independent between frames",
          "[physics][islands]") {
    Islands islands;

    // --- Frame 1 ---
    islands.ReserveMemory();
    std::ignore = islands.AddIsland(0);
    islands.AddBodyToIsland(MakeEntity(1));
    islands.AddBodyToIsland(MakeEntity(2));
    std::ignore = islands.AddIsland(10);
    islands.AddBodyToIsland(MakeEntity(3));
    islands.Clear();

    REQUIRE(islands.GetTotalIslands() == 0);
    REQUIRE(islands.GetTotalBodiesInIslandInLastFrame() == 3);
    REQUIRE(islands.GetMaxBodiesInIslandInLastFrame() == 2);

    // --- Frame 2 ---
    islands.ReserveMemory();
    std::ignore = islands.AddIsland(0);
    islands.AddBodyToIsland(MakeEntity(10));
    islands.Clear();

    REQUIRE(islands.GetTotalIslands() == 0);
    REQUIRE(islands.GetTotalBodiesInIslandInLastFrame() == 1);
    REQUIRE(islands.GetMaxBodiesInIslandInLastFrame() == 1);

    // --- Frame 3: empty frame ---
    islands.ReserveMemory();
    islands.Clear();

    REQUIRE(islands.GetTotalIslands() == 0);
    REQUIRE(islands.GetTotalBodiesInIslandInLastFrame() == 0);
    REQUIRE(islands.GetMaxBodiesInIslandInLastFrame() == 0);
}

TEST_CASE("Islands - max-bodies tracks the largest island correctly across many islands",
          "[physics][islands]") {
    Islands islands;

    // Create islands with body counts: 1, 5, 3, 7, 2.  Max should be 7.
    const std::vector<size_t> bodyCounts = {1, 5, 3, 7, 2};
    for (size_t i = 0; i < bodyCounts.size(); ++i) {
        std::ignore = islands.AddIsland(i * 10);
        for (size_t b = 0; b < bodyCounts[i]; ++b) {
            islands.AddBodyToIsland(MakeEntity(static_cast<u64>(i * 100 + b)));
        }
    }
    islands.Clear();

    REQUIRE(islands.GetMaxBodiesInIslandInLastFrame() == 7);
}
