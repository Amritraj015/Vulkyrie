#pragma once

#include "renderer/frame_graph/frame_graph_context.h"

namespace Vulkyrie {

    template <RendererBackend B> class FrameGraph;
    template <RendererBackend B> class FrameGraphResources;

    /** @brief A pass's setup function: invocable as `setup(Builder &, TPassData &)`.
     *
     * The builder is a template parameter because `FrameGraph::Builder` is a nested type and cannot be named before
     * `FrameGraph` itself is defined; `AddPass` always substitutes its own `Builder`.
     *
     * Setup runs immediately and is never stored, so unlike the execute function it may capture freely. */
    template <typename TSetup, typename TBuilder, typename TPassData>
    concept FrameGraphSetupFn = std::invocable<TSetup, TBuilder &, TPassData &>;

    /** @brief A pass's execute function: invocable as `execute(const TPassData &, FrameGraphPassContext<B> &)`.
     *
     * This only constrains the signature. `AddPass` additionally requires the callable to convert to a plain
     * function pointer, which is what rejects captures; that check is a `static_assert` rather than part of this
     * concept so a capturing lambda produces a message explaining the rule instead of an overload-resolution
     * failure that only says the concept was not satisfied. */
    template <typename TExecute, typename TPassData, typename B>
    concept FrameGraphExecuteFn = std::invocable<TExecute, const TPassData &, FrameGraphPassContext<B> &>;

    /** @brief The plain `Acquire`: the resource type takes its own storage from the transient pool and the graph's
     * byte-offset aliasing plan does not reach it. */
    template <typename T, typename B>
    concept HasPlainAcquire = requires(T a) {
        { a.Acquire(std::declval<const typename T::Descriptor &>(), ResourceLifetime{}, std::declval<const FrameGraphContext<B> &>()) } -> std::same_as<void>;
    };

    /** @brief The placed `Acquire`, which additionally receives the offset the graph's aliasing plan assigned, so
     * the resource binds to storage the graph has already laid out instead of taking its own. This is what turns
     * that plan from a report into memory actually saved, on backends that can bind two resources to one
     * allocation (`B::kHasMemoryAliasing`).
     *
     * A type that takes the placement must honour it. The graph guarantees only that two resources given
     * overlapping byte ranges have disjoint lifetimes - it does not guarantee they are never simultaneously
     * materialized, which is why `Record` may acquire every resource before any pass runs. */
    template <typename T, typename B>
    concept HasPlacedAcquire = requires(T a) {
        {
            a.Acquire(std::declval<const typename T::Descriptor &>(), ResourceLifetime{}, ResourcePlacement{}, std::declval<const FrameGraphContext<B> &>())
        } -> std::same_as<void>;
    };

    /** @brief A type the graph can manage: a descriptor, acquisition in either the plain or the placed form, and
     * release. Everything beyond this is opt-in - see `HasPreRead`, `HasPreWrite` and `HasMemoryRequirements`.
     *
     * The verbs are `Acquire`/`Release` rather than create/destroy because that is what actually happens: these
     * resources come from `TransientPool`, which hands out existing `B::Image`/`B::Buffer` objects and reclaims
     * them in bulk at frame end. Nothing is built or torn down per frame.
     *
     * `Release` takes no descriptor. A resource knows its own handle, and the pool needs nothing to take one back;
     * the hook exists so a pool-backed type can clear its handle, which turns a use-after-release into a Debug
     * assertion rather than a stale bind.
     *
     * The two `Acquire` forms are a compile-time choice resolved once per resource type when its entry is wired up,
     * so a type that takes no interest in placement generates exactly the code it would without placement existing.
     *
     * @tparam T The resource type.
     * @tparam B The renderer backend the resource is acquired against. */
    template <typename T, typename B>
    concept FrameGraphResourceType = std::is_default_constructible_v<T> && std::is_move_constructible_v<T> && requires { typename T::Descriptor; } &&
                                     (HasPlainAcquire<T, B> || HasPlacedAcquire<T, B>) && requires(T a) {
                                         { a.Release(std::declval<const FrameGraphContext<B> &>()) } -> std::same_as<void>;
                                     };

    /** @brief Whether the type wants to be notified before a pass reads the resource. Types that do not implement
     * it pay nothing: the graph stores a null hook rather than dispatching into an empty body. */
    template <typename T, typename B>
    concept HasPreRead = requires(T a) {
        { a.PreRead(std::declval<const ResourceUsage &>(), std::declval<const FrameGraphContext<B> &>()) } -> std::same_as<void>;
    };

    /** @brief Whether the type wants to be notified before a pass writes the resource. */
    template <typename T, typename B>
    concept HasPreWrite = requires(T a) {
        { a.PreWrite(std::declval<const ResourceUsage &>(), std::declval<const FrameGraphContext<B> &>()) } -> std::same_as<void>;
    };

    /** @brief Whether the type can report what its resources cost, which is what the byte-packing aliasing plan
     * needs to place two of them in the same storage.
     *
     * The size must come from the driver, not from arithmetic over the descriptor: a value that reads low places
     * two resources overlapping. Optional - a type without it is simply left out of the plan. */
    template <typename T, typename B>
    concept HasMemoryRequirements = requires(T a) {
        {
            a.GetMemoryRequirements(std::declval<const typename T::Descriptor &>(), std::declval<const Device<B> &>())
        } -> std::same_as<ResourceMemoryRequirements>;
    };

} // namespace Vulkyrie
