#include "../support/mock_backend.h"

#include "renderer/common/deletion_queue.h"
#include "renderer/common/transient_pool.h"
#include "renderer/common/transient_registry.h"

#include <catch2/catch_test_macros.hpp>

using namespace Vulkyrie;
using namespace Vulkyrie::RendererTests;

namespace {

    constexpr TextureDescriptor SOME_TEXTURE{ .Width = 1920, .Height = 1080 };
    constexpr TextureDescriptor OTHER_TEXTURE{ .Width = 256, .Height = 256 };
    constexpr BufferDescriptor SOME_BUFFER{ .Size = 4096 };

    struct Fixture final {
        DeviceCreationInfo Info{ WindowHandle{}, 800, 600, 64, 64, 16, 16 };
        MockContext Context{ Info };
        DeletionQueue<MockBackend> Deletion{ Context, Info };
        TransientRegistry<MockBackend> Registry{ Context, 8, 8 };
        TransientPool<MockBackend> Pool{ Context, Registry, Deletion, 8, 8 };

        // Registered once, the way a renderer registers at setup; every acquire below is then an array index.
        TransientTextureID SomeTexture = Registry.Register(SOME_TEXTURE);
        TransientTextureID OtherTexture = Registry.Register(OTHER_TEXTURE);
        TransientBufferID SomeBuffer = Registry.Register(SOME_BUFFER);
    };

    using LifeTime = ResourceLifetime;

} // namespace

TEST_CASE("TransientPool - two disjoint lifetimes in the same frame share one resource", "[transientpool]") {
    Fixture fixture;

    const auto first = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 0, .LastUse = 2 });
    const auto second = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 3, .LastUse = 5 });

    REQUIRE(first.Image.Id == second.Image.Id);
    REQUIRE(fixture.Context.ImagesCreated() == 1);
    REQUIRE(fixture.Pool.GetStats().ImagesCreatedThisFrame == 1);
}

TEST_CASE("TransientPool - overlapping lifetimes in the same frame get distinct resources", "[transientpool]") {
    Fixture fixture;

    const auto first = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 0, .LastUse = 5 });
    const auto second = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 2, .LastUse = 7 });

    REQUIRE(first.Image.Id != second.Image.Id);
    REQUIRE(fixture.Context.ImagesCreated() == 2);
    REQUIRE(fixture.Pool.GetStats().ImagesCreatedThisFrame == 2);
}

TEST_CASE("TransientPool - a third request slots into whichever earlier entry is free", "[transientpool]") {
    Fixture fixture;

    // A [0,5] and B [2,7] overlap, so they get distinct resources; C [6,8] only overlaps A's interval
    // (its own [0,5] already ended before 6), so C must reuse A's resource, not B's.
    const auto a = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 0, .LastUse = 5 });
    const auto b = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 2, .LastUse = 7 });
    const auto c = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 6, .LastUse = 8 });

    REQUIRE(a.Image.Id != b.Image.Id);
    REQUIRE(c.Image.Id == a.Image.Id);
    REQUIRE(fixture.Context.ImagesCreated() == 2);
}

TEST_CASE("TransientPool - a different descriptor never reuses another descriptor's entry", "[transientpool]") {
    Fixture fixture;

    const auto first = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 0, .LastUse = 1 });
    const auto second = fixture.Pool.Acquire(fixture.OtherTexture, LifeTime{ .FirstUse = 2, .LastUse = 3 });

    REQUIRE(first.Image.Id != second.Image.Id);
    REQUIRE(fixture.Context.ImagesCreated() == 2);
}

TEST_CASE("TransientPool - ResetFrame lets a new frame reuse an entry regardless of last frame's interval", "[transientpool]") {
    Fixture fixture;

    const auto frame1 = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 10, .LastUse = 20 });
    fixture.Pool.ResetFrame();

    // A low interval that would have overlapped frame 1's [10,20] must still reuse the entry, because it
    // belongs to a new frame's execution order and frame 1's interval no longer applies.
    const auto frame2 = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 0, .LastUse = 1 });

    REQUIRE(frame1.Image.Id == frame2.Image.Id);
    REQUIRE(fixture.Context.ImagesCreated() == 1);
    REQUIRE(fixture.Pool.GetStats().ImagesCreatedThisFrame == 0);
}

TEST_CASE("TransientPool - buffers reuse the same way images do", "[transientpool]") {
    Fixture fixture;

    const auto first = fixture.Pool.Acquire(fixture.SomeBuffer, LifeTime{ .FirstUse = 0, .LastUse = 2 });
    const auto second = fixture.Pool.Acquire(fixture.SomeBuffer, LifeTime{ .FirstUse = 3, .LastUse = 5 });

    REQUIRE(first.Buffer.Id == second.Buffer.Id);
    REQUIRE(fixture.Context.BuffersCreated() == 1);
    REQUIRE(fixture.Pool.GetStats().BuffersCreatedThisFrame == 1);
}

TEST_CASE("TransientPool - TrimUnused ages out entries untouched for more than the threshold", "[transientpool]") {
    Fixture fixture;

    [[maybe_unused]] const auto acquired = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 0, .LastUse = 1 });
    REQUIRE(fixture.Pool.GetStats().ImageCount == 0); // stats are only refreshed by ResetFrame/TrimUnused

    fixture.Pool.ResetFrame();
    REQUIRE(fixture.Pool.GetStats().ImageCount == 1);

    // Three more frames pass with nobody re-acquiring the entry.
    fixture.Pool.ResetFrame();
    fixture.Pool.ResetFrame();
    fixture.Pool.ResetFrame();

    fixture.Pool.TrimUnused(/*unusedFrameThreshold=*/2);

    REQUIRE(fixture.Pool.GetStats().ImageCount == 0);
}

TEST_CASE("TransientPool - TrimUnused keeps an entry that is still being reacquired", "[transientpool]") {
    Fixture fixture;

    [[maybe_unused]] const auto acquired = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 0, .LastUse = 1 });

    for (int i = 0; i < 5; ++i) {
        fixture.Pool.ResetFrame();
        [[maybe_unused]] const auto reacquired = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 0, .LastUse = 1 });
        fixture.Pool.TrimUnused(/*unusedFrameThreshold=*/2);
    }

    REQUIRE(fixture.Pool.GetStats().ImageCount == 1);
    REQUIRE(fixture.Context.ImagesCreated() == 1);
}

TEST_CASE("TransientPool - Only a freshly created resource needs no discard", "[transientpool]") {
    Fixture fixture;

    // Nothing has ever been in a resource the pool just made, so there is nothing to throw away.
    const auto fresh = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 0, .LastUse = 2 });

    REQUIRE_FALSE(fresh.RequiresDiscard);

    // Reused within the frame: the image still holds the first resource's pixels, which a pass that blends rather
    // than fully overwrites has to be told about.
    const auto reused = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 3, .LastUse = 5 });

    REQUIRE(reused.Image.Id == fresh.Image.Id);
    REQUIRE(reused.RequiresDiscard);

    // Reused across frames carries last frame's contents, which is equally not this resource's data.
    fixture.Pool.ResetFrame();

    const auto nextFrame = fixture.Pool.Acquire(fixture.SomeTexture, LifeTime{ .FirstUse = 0, .LastUse = 2 });

    REQUIRE(nextFrame.Image.Id == fresh.Image.Id);
    REQUIRE(nextFrame.RequiresDiscard);
}

