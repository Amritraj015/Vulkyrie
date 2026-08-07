#pragma once

// Mock resource backends and the recording sink shared by every frame graph test file.
#include <vulkyrie.h>

#include <string>
#include <vector>

namespace Vulkyrie::FrameGraphTests {

    /** @brief Collects everything the graph did to its resources, in order, so a test can assert on the exact
     * sequence rather than on individual flags. Threaded through `FrameGraphContext::TransientResources`. */
    struct Recorder {
    public:
        /** @brief Every backend callback, in the order it fired. */
        std::vector<std::string> Events;

        /** @brief One entry per `EmitBarriers` call, describing the batch it carried. */
        std::vector<std::string> BarrierBatches;

        /** @brief Returns only the create/destroy events, for lifetime assertions that should not be disturbed by
         * the access hooks. */
        [[nodiscard]] std::vector<std::string> Lifecycle() const {
            std::vector<std::string> filtered;

            for (const std::string &event : Events) {
                if (event.starts_with("Create:") || event.starts_with("Destroy:")) {
                    filtered.push_back(event);
                }
            }

            return filtered;
        }
    };

    /** @brief Records an event if the context carries a recorder. */
    inline void RecordEvent(const FrameGraphContext &context, std::string event) {
        if (context.TransientResources != nullptr) {
            static_cast<Recorder *>(context.TransientResources)->Events.push_back(std::move(event));
        }
    }

    /** @brief Barrier hook that appends one line per pass batch. */
    inline void RecordBarriers(const FrameGraphContext &context, std::span<const ResourceBarrier> barriers) {
        if (context.TransientResources == nullptr) {
            return;
        }

        auto *recorder = static_cast<Recorder *>(context.TransientResources);
        std::string line = "Batch(" + std::to_string(barriers.size()) + "):";

        for (const ResourceBarrier &barrier : barriers) {
            line +=
                " e" + std::to_string(barrier.Entry.Get()) + "[" + std::to_string(barrier.Before.Layout) + "->" + std::to_string(barrier.After.Layout) + "]";

            // A discard names the stages it waits on rather than a layout it comes from, so print those instead.
            if (barrier.AliasingTransition) {
                line += "{discard,waitStages=" + std::to_string(barrier.Before.Stages) + "}";
            }
        }

        recorder->BarrierBatches.push_back(std::move(line));
    }

    /** @brief Builds a context pointing at a recorder, with the barrier hook wired up. */
    [[nodiscard]] inline FrameGraphContext MakeContext(Recorder &recorder) {
        return FrameGraphContext{ .RenderContext = nullptr, .TransientResources = &recorder, .EmitBarriers = &RecordBarriers };
    }

    struct MockTextureDescriptor {
    public:
        u32 Width = 0;
        u32 Height = 0;
        std::string Format;
    };

    /** @brief A texture backend that records every callback and reports memory requirements, so it exercises the
     * lifecycle hooks, the access hooks and the aliasing planner at once. */
    struct MockTexture {
    public:
        using Descriptor = MockTextureDescriptor;

        bool Created = false;
        bool Destroyed = false;
        i32 PreReadCount = 0;
        i32 PreWriteCount = 0;

        void Create(const Descriptor &descriptor, const FrameGraphContext &context) {
            Created = true;
            Destroyed = false;
            RecordEvent(context, "Create:" + extent(descriptor));
        }

        void Destroy(const Descriptor &descriptor, const FrameGraphContext &context) {
            Destroyed = true;
            RecordEvent(context, "Destroy:" + extent(descriptor));
        }

        void PreRead(const ResourceUsage &usage, const FrameGraphContext &context) {
            ++PreReadCount;
            RecordEvent(context, "PreRead:layout=" + std::to_string(usage.Layout));
        }

        void PreWrite(const ResourceUsage &usage, const FrameGraphContext &context) {
            ++PreWriteCount;
            RecordEvent(context, "PreWrite:layout=" + std::to_string(usage.Layout));
        }

        [[nodiscard]] ResourceMemoryRequirements GetMemoryRequirements(const Descriptor &descriptor) const {
            return ResourceMemoryRequirements{ .Size = static_cast<u64>(descriptor.Width) * descriptor.Height * 4, .Alignment = 256 };
        }

    private:
        [[nodiscard]] static std::string extent(const Descriptor &descriptor) {
            return std::to_string(descriptor.Width) + "x" + std::to_string(descriptor.Height);
        }
    };

    /** @brief A minimal backend: no access hooks, no memory requirements. Exercises the paths where the graph must
     * cope with a backend that opts into nothing. */
    struct MockBuffer {
    public:
        using Descriptor = size_t; // Size in bytes.

        bool Created = false;
        bool Destroyed = false;

        void Create(const Descriptor &descriptor, const FrameGraphContext &context) {
            Created = true;
            RecordEvent(context, "CreateBuffer:" + std::to_string(descriptor));
        }

        void Destroy(const Descriptor &descriptor, const FrameGraphContext &context) {
            Destroyed = true;
            RecordEvent(context, "DestroyBuffer:" + std::to_string(descriptor));
        }
    };

    /** @brief A backend whose descriptor owns no heap memory, so a test can attribute every byte the graph
     * allocates to the graph itself rather than to the descriptor. */
    struct MockPodTexture {
    public:
        struct Descriptor {
        public:
            u32 Width = 0;
            u32 Height = 0;
        };

        void Create(const Descriptor &, const FrameGraphContext &) {
        }

        void Destroy(const Descriptor &, const FrameGraphContext &) {
        }
    };

    /** @brief A texture backend that takes the placed `Create`, so a test can assert on the offsets the aliasing
     * plan actually hands the backend rather than only on the totals the report publishes. */
    struct MockPlacedTexture {
    public:
        using Descriptor = MockTextureDescriptor;

        ResourcePlacement Placement{};
        bool Created = false;

        void Create(const Descriptor &descriptor, ResourcePlacement placement, const FrameGraphContext &context) {
            Placement = placement;
            Created = true;
            RecordEvent(context, "Create:" + std::to_string(descriptor.Width) + "@" + (placement.IsAliased ? std::to_string(placement.Offset) : "unplaced"));
        }

        void Destroy(const Descriptor &, const FrameGraphContext &context) {
            RecordEvent(context, "Destroy");
        }

        [[nodiscard]] ResourceMemoryRequirements GetMemoryRequirements(const Descriptor &descriptor) const {
            return ResourceMemoryRequirements{ .Size = static_cast<u64>(descriptor.Width) * descriptor.Height * 4, .Alignment = 256 };
        }
    };

    static_assert(FrameGraphResourceType<MockTexture>, "MockTexture must satisfy the backend concept.");
    static_assert(FrameGraphResourceType<MockBuffer>, "MockBuffer must satisfy the backend concept.");
    static_assert(FrameGraphResourceType<MockPodTexture>, "PodTexture must satisfy the backend concept.");
    static_assert(HasPreRead<MockTexture> && HasPreWrite<MockTexture>, "MockTexture implements both access hooks.");
    static_assert(!HasPreRead<MockBuffer> && !HasPreWrite<MockBuffer>, "MockBuffer opts out of the access hooks.");
    static_assert(HasMemoryRequirements<MockTexture>, "MockTexture reports memory requirements.");
    static_assert(!HasMemoryRequirements<MockBuffer>, "MockBuffer does not report memory requirements.");
    static_assert(FrameGraphResourceType<MockPlacedTexture>, "PlacedTexture must satisfy the backend concept through the placed Create.");
    static_assert(HasPlacedCreate<MockPlacedTexture> && !HasPlainCreate<MockPlacedTexture>, "PlacedTexture takes only the placed Create.");
    static_assert(HasPlainCreate<MockTexture> && !HasPlacedCreate<MockTexture>, "MockTexture takes only the plain Create.");

} // namespace Vulkyrie::FrameGraphTests
