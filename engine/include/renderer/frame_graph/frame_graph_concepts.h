#pragma once

#include "renderer/frame_graph/frame_graph_types.h"

namespace Vulkyrie {

    class FrameGraph;

    /** @brief A pass's setup function: invocable as `setup(Builder &, TPassData &)`.
     *
     * The builder is a template parameter because `FrameGraph::Builder` is a nested type and cannot be named before
     * `FrameGraph` itself is defined; `AddPass` always substitutes its own `Builder`. */
    template <typename TSetup, typename TBuilder, typename TPassData>
    concept FrameGraphSetupFn = std::invocable<TSetup, TBuilder &, TPassData &>;

    /** @brief A pass's execute function: invocable as
     * `execute(const TPassData &, FrameGraph &, const FrameGraphContext &)`. */
    template <typename TExecute, typename TPassData>
    concept FrameGraphExecuteFn = std::invocable<TExecute, const TPassData &, FrameGraph &, const FrameGraphContext &>;

    /** @brief The plain `Create`: the resource type allocates its own storage and the aliasing plan does not reach
     * it. */
    template <typename T>
    concept HasPlainCreate = requires(T a) {
        { a.Create(std::declval<const typename T::Descriptor &>(), std::declval<const FrameGraphContext &>()) } -> std::same_as<void>;
    };

    /** @brief The placed `Create`, which additionally receives the offset the aliasing plan assigned, so the
     * resource binds to storage the graph has already laid out instead of allocating its own. This is what turns
     * the plan from a report into actual memory saved.
     *
     * A type that takes the placement must honour it. The graph guarantees only that two resources given
     * overlapping byte ranges have disjoint lifetimes - it does not guarantee they are never simultaneously
     * materialized, which is why `Record` may create every resource before any pass runs. */
    template <typename T>
    concept HasPlacedCreate = requires(T a) {
        { a.Create(std::declval<const typename T::Descriptor &>(), ResourcePlacement{}, std::declval<const FrameGraphContext &>()) } -> std::same_as<void>;
    };

    /** @brief A type the graph can manage: a descriptor, release taking the frame's `FrameGraphContext`, and
     * materialization in either the plain or the placed form. Everything beyond this is opt-in - see `HasPreRead`,
     * `HasPreWrite` and `HasMemoryRequirements`.
     *
     * The two `Create` forms are a compile-time choice resolved once per resource type when its entry is wired up,
     * so a type that takes neither interest in placement nor memory requirements generates exactly the code it did
     * before placement existed. */
    template <typename T>
    concept FrameGraphResourceType = std::is_default_constructible_v<T> && std::is_move_constructible_v<T> && requires { typename T::Descriptor; } &&
                                     (HasPlainCreate<T> || HasPlacedCreate<T>) && requires(T a) {
                                         {
                                             a.Destroy(std::declval<const typename T::Descriptor &>(), std::declval<const FrameGraphContext &>())
                                         } -> std::same_as<void>;
                                     };

    /** @brief Whether the type wants to be notified before a pass reads the resource. Types that do not implement
     * it pay nothing: the graph stores a null hook rather than dispatching into an empty body. */
    template <typename T>
    concept HasPreRead = requires(T a) {
        { a.PreRead(std::declval<const ResourceUsage &>(), std::declval<const FrameGraphContext &>()) } -> std::same_as<void>;
    };

    /** @brief Whether the type wants to be notified before a pass writes the resource. */
    template <typename T>
    concept HasPreWrite = requires(T a) {
        { a.PreWrite(std::declval<const ResourceUsage &>(), std::declval<const FrameGraphContext &>()) } -> std::same_as<void>;
    };

    /** @brief Whether the type can report the size and alignment its resources need, which is what transient
     * aliasing requires to pack two resources with disjoint lifetimes into the same storage.
     *
     * Deliberately optional rather than part of `FrameGraphResourceType`: a type without it stays perfectly usable,
     * its resources are simply excluded from the aliasing plan. */
    template <typename T>
    concept HasMemoryRequirements = requires(T a) {
        { a.GetMemoryRequirements(std::declval<const typename T::Descriptor &>()) } -> std::same_as<ResourceMemoryRequirements>;
    };

} // namespace Vulkyrie
