#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <algorithm>
#include <vulkyrie.h>

using namespace Vulkyrie;

// Helper: build an AABB cube of given side length at position (x, y, z).
static AABB MakeAABB(float x, float y, float z, float size = 1.0f) {
    return AABB(glm::vec3(x, y, z), glm::vec3(x + size, y + size, z + size));
}

// ===========================================================================================
// Construction
// ===========================================================================================

TEST_CASE("DynamicAABBTree - Default construction creates queryable empty tree", "[physics][bvh]") {
    DynamicAABBTree tree;

    std::vector<i32> results;
    tree.QueryOverlaps(MakeAABB(-1000.0f, -1000.0f, -1000.0f, 2000.0f), results);
    REQUIRE(results.empty());
}

TEST_CASE("DynamicAABBTree - Construction with custom initial capacity", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f, 4);

    // Should be able to add objects without error
    i32 idx = tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 0);
    REQUIRE(idx >= 0);
}

// ===========================================================================================
// AddObject / GetFatAABB
// ===========================================================================================

TEST_CASE("DynamicAABBTree - AddObject returns valid leaf index", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);

    i32 idx = tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 42);
    REQUIRE(idx >= 0);
}

TEST_CASE("DynamicAABBTree - AddObject with zero inflation stores exact AABB", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    AABB aabb(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(4.0f, 5.0f, 6.0f));

    i32 idx = tree.AddObject(aabb, 0);
    const AABB &fat = tree.GetFatAABB(idx);

    REQUIRE(fat.GetMin() == glm::vec3(1.0f, 2.0f, 3.0f));
    REQUIRE(fat.GetMax() == glm::vec3(4.0f, 5.0f, 6.0f));
}

TEST_CASE("DynamicAABBTree - AddObject inflates fat AABB by inflation percentage of extents", "[physics][bvh]") {
    constexpr float inflationPct = 0.1f;
    DynamicAABBTree tree(inflationPct);
    AABB aabb(glm::vec3(0.0f), glm::vec3(10.0f)); // extents = (10,10,10), inflation = 1 per side

    i32 idx = tree.AddObject(aabb, 0);
    const AABB &fat = tree.GetFatAABB(idx);

    REQUIRE(fat.GetMin().x == Catch::Approx(-1.0f));
    REQUIRE(fat.GetMin().y == Catch::Approx(-1.0f));
    REQUIRE(fat.GetMin().z == Catch::Approx(-1.0f));
    REQUIRE(fat.GetMax().x == Catch::Approx(11.0f));
    REQUIRE(fat.GetMax().y == Catch::Approx(11.0f));
    REQUIRE(fat.GetMax().z == Catch::Approx(11.0f));
}

TEST_CASE("DynamicAABBTree - Fat AABB contains original AABB after inflation", "[physics][bvh]") {
    DynamicAABBTree tree(0.05f);
    AABB aabb(glm::vec3(-3.0f, -1.0f, 0.0f), glm::vec3(3.0f, 1.0f, 2.0f));

    i32 idx = tree.AddObject(aabb, 0);

    REQUIRE(tree.GetFatAABB(idx).Contains(aabb));
}

TEST_CASE("DynamicAABBTree - AddObject multiple distinct indices", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);

    i32 i0 = tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 0);
    i32 i1 = tree.AddObject(MakeAABB(5.0f, 0.0f, 0.0f), 1);
    i32 i2 = tree.AddObject(MakeAABB(10.0f, 0.0f, 0.0f), 2);

    REQUIRE(i0 != i1);
    REQUIRE(i1 != i2);
    REQUIRE(i0 != i2);
}

// ===========================================================================================
// GetNodeData / GetNodeDataPointer
// ===========================================================================================

TEST_CASE("DynamicAABBTree - GetNodeData returns stored integer", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);

    i32 idx = tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 1337);

    REQUIRE(tree.GetNodeData(idx) == 1337);
}

TEST_CASE("DynamicAABBTree - GetNodeData survives tree mutations", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);

    i32 idx = tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 99);
    // Add other nodes to force internal node creation
    tree.AddObject(MakeAABB(10.0f, 0.0f, 0.0f), 0);
    tree.AddObject(MakeAABB(20.0f, 0.0f, 0.0f), 0);

    REQUIRE(tree.GetNodeData(idx) == 99);
}

TEST_CASE("DynamicAABBTree - GetNodeDataPointer returns stored pointer", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    int sentinel = 0xDEAD;

    i32 idx = tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), static_cast<void *>(&sentinel));

    REQUIRE(tree.GetNodeDataPointer(idx) == static_cast<void *>(&sentinel));
}

// ===========================================================================================
// GetRootNodeAABB
// ===========================================================================================

TEST_CASE("DynamicAABBTree - GetRootNodeAABB on single-node tree equals its fat AABB", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    AABB aabb(glm::vec3(1.0f), glm::vec3(3.0f));

    i32 idx = tree.AddObject(aabb, 0);

    REQUIRE(tree.GetRootNodeAABB().GetMin() == tree.GetFatAABB(idx).GetMin());
    REQUIRE(tree.GetRootNodeAABB().GetMax() == tree.GetFatAABB(idx).GetMax());
}

TEST_CASE("DynamicAABBTree - GetRootNodeAABB encompasses all leaf AABBs", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 0);
    tree.AddObject(MakeAABB(10.0f, 0.0f, 0.0f), 1);
    tree.AddObject(MakeAABB(-5.0f, -5.0f, -5.0f), 2);

    const AABB &root = tree.GetRootNodeAABB();

    REQUIRE(root.Contains(MakeAABB(0.0f, 0.0f, 0.0f)));
    REQUIRE(root.Contains(MakeAABB(10.0f, 0.0f, 0.0f)));
    REQUIRE(root.Contains(MakeAABB(-5.0f, -5.0f, -5.0f)));
}

// ===========================================================================================
// RemoveObject
// ===========================================================================================

TEST_CASE("DynamicAABBTree - RemoveObject from single-node tree leaves tree empty", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    AABB aabb = MakeAABB(0.0f, 0.0f, 0.0f);
    i32 idx = tree.AddObject(aabb, 0);

    tree.RemoveObject(idx);

    std::vector<i32> results;
    tree.QueryOverlaps(MakeAABB(-10.0f, -10.0f, -10.0f, 20.0f), results);
    REQUIRE(results.empty());
}

TEST_CASE("DynamicAABBTree - RemoveObject sibling becomes new root for 2-node tree", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    AABB aabbA = MakeAABB(0.0f, 0.0f, 0.0f);
    AABB aabbB = MakeAABB(10.0f, 0.0f, 0.0f);

    i32 idxA = tree.AddObject(aabbA, 1);
    i32 idxB = tree.AddObject(aabbB, 2);

    tree.RemoveObject(idxA);

    std::vector<i32> resultsA;
    tree.QueryOverlaps(aabbA, resultsA);
    REQUIRE(resultsA.empty());

    std::vector<i32> resultsB;
    tree.QueryOverlaps(aabbB, resultsB);
    REQUIRE(resultsB.size() == 1);
    REQUIRE(resultsB[0] == idxB);
}

TEST_CASE("DynamicAABBTree - Removed node slot can be reused", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    AABB aabb = MakeAABB(0.0f, 0.0f, 0.0f);

    i32 first = tree.AddObject(aabb, 1);
    tree.RemoveObject(first);
    i32 second = tree.AddObject(aabb, 2);

    REQUIRE(tree.GetNodeData(second) == 2);

    std::vector<i32> results;
    tree.QueryOverlaps(aabb, results);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == second);
}

TEST_CASE("DynamicAABBTree - RemoveObject from multi-node tree preserves remaining nodes", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    i32 i0 = tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 0);
    i32 i1 = tree.AddObject(MakeAABB(5.0f, 0.0f, 0.0f), 1);
    i32 i2 = tree.AddObject(MakeAABB(10.0f, 0.0f, 0.0f), 2);
    i32 i3 = tree.AddObject(MakeAABB(15.0f, 0.0f, 0.0f), 3);

    tree.RemoveObject(i1);

    // Remaining three nodes must still be findable
    std::vector<i32> results;
    tree.QueryOverlaps(MakeAABB(-1.0f, -1.0f, -1.0f, 20.0f), results);
    REQUIRE(results.size() == 3);

    std::vector<i32> remaining = { i0, i2, i3 };
    for (i32 r : remaining) {
        std::vector<i32> single;
        tree.QueryOverlaps(tree.GetFatAABB(r), single);
        REQUIRE(!single.empty());
    }
}

TEST_CASE("DynamicAABBTree - Interleaved add and remove keeps tree consistent", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);

    i32 i0 = tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 0);
    i32 i1 = tree.AddObject(MakeAABB(5.0f, 0.0f, 0.0f), 1);
    tree.RemoveObject(i0);
    i32 i2 = tree.AddObject(MakeAABB(10.0f, 0.0f, 0.0f), 2);
    tree.RemoveObject(i1);
    i32 i3 = tree.AddObject(MakeAABB(15.0f, 0.0f, 0.0f), 3);

    std::vector<i32> results;
    tree.QueryOverlaps(MakeAABB(-1.0f, -1.0f, -1.0f, 20.0f), results);
    REQUIRE(results.size() == 2);

    std::vector<i32> found = { i2, i3 };
    for (i32 f : found) {
        REQUIRE(tree.GetNodeData(f) == (f == i2 ? 2 : 3));
    }
}

// ===========================================================================================
// UpdateObject
// ===========================================================================================

TEST_CASE("DynamicAABBTree - UpdateObject returns false when new AABB fits inside fat AABB", "[physics][bvh]") {
    DynamicAABBTree tree(0.2f); // 20% inflation
    AABB aabb(glm::vec3(0.0f), glm::vec3(10.0f)); // extents=10, inflation=2 → fat=[−2..12]

    i32 idx = tree.AddObject(aabb, 0);

    // Slight shift still within fat AABB
    AABB slightlyMoved(glm::vec3(0.5f), glm::vec3(10.5f));
    bool updated = tree.UpdateObject(idx, slightlyMoved, false);

    REQUIRE(!updated);
}

TEST_CASE("DynamicAABBTree - UpdateObject returns true when new AABB escapes fat AABB", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f); // No padding → any movement escapes immediately

    i32 idx = tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 0);

    bool updated = tree.UpdateObject(idx, MakeAABB(50.0f, 50.0f, 50.0f), false);
    REQUIRE(updated);
}

TEST_CASE("DynamicAABBTree - UpdateObject forceReinsert always returns true", "[physics][bvh]") {
    DynamicAABBTree tree(0.5f); // Large padding — AABB is well inside fat
    AABB aabb(glm::vec3(0.0f), glm::vec3(10.0f));

    i32 idx = tree.AddObject(aabb, 0);

    // Same AABB but forced reinsert
    bool updated = tree.UpdateObject(idx, aabb, true);
    REQUIRE(updated);
}

TEST_CASE("DynamicAABBTree - Updated node is queryable at new position and not at old", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    AABB original = MakeAABB(0.0f, 0.0f, 0.0f);
    AABB moved    = MakeAABB(100.0f, 100.0f, 100.0f);

    i32 idx = tree.AddObject(original, 7);
    tree.UpdateObject(idx, moved, false);

    std::vector<i32> oldArea;
    tree.QueryOverlaps(original, oldArea);
    REQUIRE(oldArea.empty());

    std::vector<i32> newArea;
    tree.QueryOverlaps(moved, newArea);
    REQUIRE(newArea.size() == 1);
    REQUIRE(newArea[0] == idx);
}

TEST_CASE("DynamicAABBTree - UpdateObject re-inflates fat AABB around new position", "[physics][bvh]") {
    DynamicAABBTree tree(0.1f);
    AABB original(glm::vec3(0.0f), glm::vec3(10.0f));

    i32 idx = tree.AddObject(original, 0);

    AABB moved(glm::vec3(50.0f), glm::vec3(60.0f));
    tree.UpdateObject(idx, moved, false);

    REQUIRE(tree.GetFatAABB(idx).Contains(moved));
}

TEST_CASE("DynamicAABBTree - UpdateObject data is preserved after reinsert", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    AABB original = MakeAABB(0.0f, 0.0f, 0.0f);

    i32 idx = tree.AddObject(original, 555);
    tree.UpdateObject(idx, MakeAABB(50.0f, 50.0f, 50.0f), false);

    REQUIRE(tree.GetNodeData(idx) == 555);
}

// ===========================================================================================
// QueryOverlaps
// ===========================================================================================

TEST_CASE("DynamicAABBTree - QueryOverlaps on empty tree returns nothing", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);

    std::vector<i32> results;
    tree.QueryOverlaps(MakeAABB(-1000.0f, -1000.0f, -1000.0f, 2000.0f), results);
    REQUIRE(results.empty());
}

TEST_CASE("DynamicAABBTree - QueryOverlaps finds the single overlapping node", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    AABB aabb = MakeAABB(0.0f, 0.0f, 0.0f);

    i32 idx = tree.AddObject(aabb, 5);

    std::vector<i32> results;
    tree.QueryOverlaps(aabb, results);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == idx);
}

TEST_CASE("DynamicAABBTree - QueryOverlaps misses non-overlapping node", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 0);

    std::vector<i32> results;
    tree.QueryOverlaps(MakeAABB(100.0f, 100.0f, 100.0f), results);
    REQUIRE(results.empty());
}

TEST_CASE("DynamicAABBTree - QueryOverlaps finds all overlapping nodes", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(2.0f)), 0);
    tree.AddObject(AABB(glm::vec3(1.0f), glm::vec3(3.0f)), 1); // overlaps first
    tree.AddObject(MakeAABB(50.0f, 50.0f, 50.0f), 2);           // far away

    std::vector<i32> results;
    tree.QueryOverlaps(AABB(glm::vec3(0.0f), glm::vec3(4.0f)), results);
    REQUIRE(results.size() == 2);
}

TEST_CASE("DynamicAABBTree - QueryOverlaps with touching (face-shared) AABB reports overlap", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    i32 idx = tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(1.0f)), 0);

    // Query AABB shares exactly one face (x=1 plane)
    std::vector<i32> results;
    tree.QueryOverlaps(AABB(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(2.0f, 1.0f, 1.0f)), results);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0] == idx);
}

TEST_CASE("DynamicAABBTree - QueryOverlaps appends to existing results vector", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 0);

    std::vector<i32> results;
    results.push_back(-5); // sentinel pre-existing element

    tree.QueryOverlaps(MakeAABB(0.0f, 0.0f, 0.0f), results);

    // Results are appended; the pre-existing element is still at front
    REQUIRE(results.size() == 2);
    REQUIRE(results[0] == -5);
}

TEST_CASE("DynamicAABBTree - QueryOverlaps returns nothing when all nodes removed", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    i32 i0 = tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 0);
    i32 i1 = tree.AddObject(MakeAABB(5.0f, 0.0f, 0.0f), 1);

    tree.RemoveObject(i0);
    tree.RemoveObject(i1);

    std::vector<i32> results;
    tree.QueryOverlaps(MakeAABB(-10.0f, -10.0f, -10.0f, 30.0f), results);
    REQUIRE(results.empty());
}

// ===========================================================================================
// QueryOverlappingPairs
// ===========================================================================================

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs on empty tree returns nothing", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);

    std::vector<std::pair<i32, i32>> pairs;
    tree.QueryOverlappingPairs({}, pairs);
    REQUIRE(pairs.empty());
}

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs single node produces no pairs", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    i32 idx = tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 0);

    std::vector<std::pair<i32, i32>> pairs;
    tree.QueryOverlappingPairs({ idx }, pairs);
    REQUIRE(pairs.empty());
}

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs two non-overlapping nodes produce no pairs", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    i32 i0 = tree.AddObject(MakeAABB(0.0f, 0.0f, 0.0f), 0);
    i32 i1 = tree.AddObject(MakeAABB(10.0f, 0.0f, 0.0f), 1);

    std::vector<std::pair<i32, i32>> pairs;
    tree.QueryOverlappingPairs({ i0, i1 }, pairs);
    REQUIRE(pairs.empty());
}

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs finds one overlapping pair", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);
    i32 i0 = tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(2.0f)), 0);
    i32 i1 = tree.AddObject(AABB(glm::vec3(1.0f), glm::vec3(3.0f)), 1);

    std::vector<std::pair<i32, i32>> pairs;
    tree.QueryOverlappingPairs({ i0, i1 }, pairs);

    REQUIRE(pairs.size() == 2);
    REQUIRE(pairs[0].first  == std::min(i0, i1));
    REQUIRE(pairs[0].second == std::max(i0, i1));
}

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs both-active pair appears exactly twice (once per test node)", "[physics][bvh]") {
    // With no deduplication, when both endpoints are in nodeIndices each acts as the test
    // node and finds the other — so the same geometric pair is emitted twice.
    DynamicAABBTree tree(0.0f);
    i32 i0 = tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(2.0f)), 0);
    i32 i1 = tree.AddObject(AABB(glm::vec3(1.0f), glm::vec3(3.0f)), 1);

    std::vector<std::pair<i32, i32>> pairs;
    tree.QueryOverlappingPairs({ i0, i1 }, pairs);

    REQUIRE(pairs.size() == 2);
}

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs finds one overlapping pair (no dedupe)", "[physics][bvh]") {
    // No deduplication: two active nodes each find the other, producing 2 raw entries.
    // This test just confirms each direction finds a pair containing both indices.
    DynamicAABBTree tree(0.0f);
    i32 i0 = tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(2.0f)), 0);
    i32 i1 = tree.AddObject(AABB(glm::vec3(1.0f), glm::vec3(3.0f)), 1);

    std::vector<std::pair<i32, i32>> pairs;
    tree.QueryOverlappingPairs({ i0, i1 }, pairs);

    REQUIRE(pairs.size() == 2);
    // Both raw entries must reference the same two nodes (order within a pair may differ)
    for (const auto &[a, b] : pairs) {
        REQUIRE(((a == i0 && b == i1) || (a == i1 && b == i0)));
    }
}

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs three mutually overlapping nodes yield six raw entries", "[physics][bvh]") {
    // C(3,2) = 3 geometric pairs, but each active node finds both others → 3*2 = 6 raw entries.
    DynamicAABBTree tree(0.0f);
    i32 i0 = tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(3.0f)), 0);
    i32 i1 = tree.AddObject(AABB(glm::vec3(1.0f), glm::vec3(4.0f)), 1);
    i32 i2 = tree.AddObject(AABB(glm::vec3(2.0f), glm::vec3(5.0f)), 2);

    std::vector<std::pair<i32, i32>> pairs;
    tree.QueryOverlappingPairs({ i0, i1, i2 }, pairs);

    REQUIRE(pairs.size() == 6);
}

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs mixed: one overlapping, one not", "[physics][bvh]") {
    // i0 and i1 overlap; i2 is isolated. Both i0 and i1 are active so the pair is emitted
    // twice (once from each direction). i2 finds nothing.
    DynamicAABBTree tree(0.0f);
    i32 i0 = tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(2.0f)), 0);
    i32 i1 = tree.AddObject(AABB(glm::vec3(1.0f), glm::vec3(3.0f)), 1); // overlaps i0
    i32 i2 = tree.AddObject(MakeAABB(50.0f, 50.0f, 50.0f), 2);           // isolated

    std::vector<std::pair<i32, i32>> pairs;
    tree.QueryOverlappingPairs({ i0, i1, i2 }, pairs);

    REQUIRE(pairs.size() == 2);
    for (const auto &[a, b] : pairs) {
        REQUIRE(((a == i0 && b == i1) || (a == i1 && b == i0)));
    }
}

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs appends to existing output vector", "[physics][bvh]") {
    // Two active overlapping nodes → 2 raw entries appended to the sentinel.
    DynamicAABBTree tree(0.0f);
    i32 i0 = tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(2.0f)), 0);
    i32 i1 = tree.AddObject(AABB(glm::vec3(1.0f), glm::vec3(3.0f)), 1);

    std::vector<std::pair<i32, i32>> pairs;
    pairs.emplace_back(-1, -1); // sentinel

    tree.QueryOverlappingPairs({ i0, i1 }, pairs);

    REQUIRE(pairs.size() == 3); // sentinel + 2 raw entries
    REQUIRE(pairs[0] == std::make_pair(i32(-1), i32(-1)));
}

// ===========================================================================================
// Capacity growth
// ===========================================================================================

TEST_CASE("DynamicAABBTree - Tree capacity grows automatically when exceeded", "[physics][bvh]") {
    // Start with capacity for 2 nodes; every insertion past that doubles the pool
    DynamicAABBTree tree(0.0f, 2);

    std::vector<i32> indices;
    for (int i = 0; i < 16; ++i) {
        float x = static_cast<float>(i * 3);
        indices.push_back(tree.AddObject(MakeAABB(x, 0.0f, 0.0f), i));
    }

    // All nodes must still report correct data
    for (int i = 0; i < 16; ++i) {
        REQUIRE(tree.GetNodeData(indices[i]) == i);
    }
}

TEST_CASE("DynamicAABBTree - All nodes queryable after capacity growth", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f, 2);

    std::vector<i32> indices;
    for (int i = 0; i < 8; ++i) {
        float x = static_cast<float>(i * 5);
        indices.push_back(tree.AddObject(MakeAABB(x, 0.0f, 0.0f), i));
    }

    for (int i = 0; i < 8; ++i) {
        float x = static_cast<float>(i * 5);
        std::vector<i32> results;
        tree.QueryOverlaps(MakeAABB(x, 0.0f, 0.0f), results);
        REQUIRE(results.size() == 1);
        REQUIRE(results[0] == indices[i]);
    }
}

// ===========================================================================================
// Large-tree structural integrity
// ===========================================================================================

TEST_CASE("DynamicAABBTree - 50 non-overlapping objects: each found by exact query", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);

    std::vector<i32> indices;
    for (int i = 0; i < 50; ++i) {
        float x = static_cast<float>(i * 3); // 2-unit gap between each
        indices.push_back(tree.AddObject(MakeAABB(x, 0.0f, 0.0f), i));
    }

    for (int i = 0; i < 50; ++i) {
        float x = static_cast<float>(i * 3);
        std::vector<i32> results;
        tree.QueryOverlaps(MakeAABB(x, 0.0f, 0.0f), results);

        REQUIRE(results.size() == 1);
        REQUIRE(results[0] == indices[i]);
        REQUIRE(tree.GetNodeData(results[0]) == i);
    }
}

TEST_CASE("DynamicAABBTree - Remove half then query remaining are still correct", "[physics][bvh]") {
    DynamicAABBTree tree(0.0f);

    std::vector<i32> indices;
    for (int i = 0; i < 30; ++i) {
        float x = static_cast<float>(i * 3);
        indices.push_back(tree.AddObject(MakeAABB(x, 0.0f, 0.0f), i));
    }

    // Remove the first 15
    for (int i = 0; i < 15; ++i) {
        tree.RemoveObject(indices[i]);
    }

    // Removed range should be gone
    for (int i = 0; i < 15; ++i) {
        float x = static_cast<float>(i * 3);
        std::vector<i32> results;
        tree.QueryOverlaps(MakeAABB(x, 0.0f, 0.0f), results);
        REQUIRE(results.empty());
    }

    // Remaining nodes (15–29) should still be found
    for (int i = 15; i < 30; ++i) {
        float x = static_cast<float>(i * 3);
        std::vector<i32> results;
        tree.QueryOverlaps(MakeAABB(x, 0.0f, 0.0f), results);
        REQUIRE(results.size() == 1);
        REQUIRE(tree.GetNodeData(results[0]) == i);
    }
}

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs on 10 overlapping objects yields N*(N-1) raw entries", "[physics][bvh]") {
    // All 10 objects at the same position → every active node finds every other active node.
    // Each of the 10 test nodes emits 9 entries → 90 raw entries total (each geometric pair twice).
    DynamicAABBTree tree(0.0f);

    std::vector<i32> indices;
    for (int i = 0; i < 10; ++i) {
        indices.push_back(tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(1.0f)), i));
    }

    std::vector<std::pair<i32, i32>> pairs;
    tree.QueryOverlappingPairs(indices, pairs);

    // 10 test nodes × 9 other active leaves = 90 raw entries
    REQUIRE(pairs.size() == 90);
}

// ===========================================================================================
// QueryOverlappingPairs — active-vs-static deduplication
// ===========================================================================================

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs: moved node (higher index) finds static node (lower index)", "[physics][bvh]") {
    // Only the higher-indexed node is in nodeIndices; the lower-indexed node is static.
    // The single test node finds the static node and emits exactly one raw entry.
    //
    // Moved nodes: [i1]   Static: [i0]   (i0 inserted first → i0 < i1)
    DynamicAABBTree tree(0.0f);
    i32 i0 = tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(2.0f)), 0); // static
    i32 i1 = tree.AddObject(AABB(glm::vec3(1.0f), glm::vec3(3.0f)), 1); // "moved"

    // Only i1 is in the active (moved) set; i0 is static
    std::vector<std::pair<i32, i32>> pairs;
    tree.QueryOverlappingPairs({ i1 }, pairs);

    REQUIRE(pairs.size() == 1);
    // Implementation normalizes to canonical order (smaller index first).
    // i0 < i1, so the entry is (i0, i1) even though i1 was the test node.
    REQUIRE(pairs[0].first  == std::min(i0, i1));
    REQUIRE(pairs[0].second == std::max(i0, i1));
}

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs: moved node finds static node, no false positives", "[physics][bvh]") {
    // The moved node overlaps one static node but not another. Single test node → one raw entry.
    DynamicAABBTree tree(0.0f);
    i32 iStatic0 = tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(2.0f)), 0); // overlaps moved
    i32 iStatic1 = tree.AddObject(MakeAABB(50.0f, 50.0f, 50.0f), 1);           // far away
    i32 iMoved   = tree.AddObject(AABB(glm::vec3(1.0f), glm::vec3(3.0f)), 2);  // overlaps only iStatic0

    std::vector<std::pair<i32, i32>> pairs;
    tree.QueryOverlappingPairs({ iMoved }, pairs);

    REQUIRE(pairs.size() == 1);
    // Implementation normalizes to canonical order (smaller index first).
    REQUIRE(pairs[0].first  == std::min(iStatic0, iMoved));
    REQUIRE(pairs[0].second == std::max(iStatic0, iMoved));
    (void)iStatic1;
}

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs: mixed active+static, raw entry count", "[physics][bvh]") {
    // Two active nodes (iA, iB) each overlap the static node AND each other.
    // iA finds {iStatic, iB}  → 2 entries
    // iB finds {iStatic, iA}  → 2 entries
    // Total: 4 raw entries (active-active pair appears twice, each active-static once).
    DynamicAABBTree tree(0.0f);
    i32 iStatic = tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(5.0f)), 0); // overlaps everything
    i32 iA      = tree.AddObject(AABB(glm::vec3(1.0f), glm::vec3(4.0f)), 1);
    i32 iB      = tree.AddObject(AABB(glm::vec3(2.0f), glm::vec3(5.0f)), 2);

    std::vector<std::pair<i32, i32>> pairs;
    tree.QueryOverlappingPairs({ iA, iB }, pairs);

    REQUIRE(pairs.size() == 4);
    (void)iStatic;
}

TEST_CASE("DynamicAABBTree - QueryOverlappingPairs: both nodes active, query order does not affect entry count", "[physics][bvh]") {
    // Querying with i1 first or i0 first should still emit 2 raw entries.
    DynamicAABBTree tree(0.0f);
    i32 i0 = tree.AddObject(AABB(glm::vec3(0.0f), glm::vec3(2.0f)), 0);
    i32 i1 = tree.AddObject(AABB(glm::vec3(1.0f), glm::vec3(3.0f)), 1);

    std::vector<std::pair<i32, i32>> pairsAB, pairsBA;
    tree.QueryOverlappingPairs({ i0, i1 }, pairsAB);
    tree.QueryOverlappingPairs({ i1, i0 }, pairsBA);

    REQUIRE(pairsAB.size() == 2);
    REQUIRE(pairsBA.size() == 2);
}
