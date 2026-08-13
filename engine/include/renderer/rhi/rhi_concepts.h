#pragma once

#include "renderer/rhi/rhi_types.h"

namespace Vulkyrie {

    template <typename T>
    concept HasDeviceLifetime = requires(T &t, const T &ct, const DeviceCreationInfo &info) {
        { t.Initialize(info) } -> std::same_as<DeviceStatus>;
        { t.Shutdown() } -> std::same_as<void>;
        { ct.QueryCapabilities() } -> std::same_as<DeviceCapabilities>;
        { t.WaitIdle() } -> std::same_as<DeviceStatus>;
    };

    template <typename T>
    concept ManagesHeap = requires(T &t, const T &ct, const BufferHandle bh, u32 n, const QueryHeapHandle qhh) {
        { t.CreateQueryHeap(n) } -> std::same_as<QueryHeapHandle>;
        { t.DestroyQueryHeap(qhh) } -> std::same_as<bool>;
        { ct.GetDeviceAddress(bh) } -> std::same_as<u32>;
        { ct.MappedPointer(bh) } -> std::same_as<std::byte *>;
    };

    template <typename T>
    concept ManagesPipelines = requires(T &t, const GraphicsPipelineDescriptor &gpd, const ComputePipelineDescriptor &cpd, const PipelineHandle ph) {
        { t.CreateGraphicsPipeline(gpd) } -> std::same_as<PipelineHandle>;
        { t.CreateComputePipeline(cpd) } -> std::same_as<PipelineHandle>;
        { t.DestroyPipeline(ph) } -> std::same_as<bool>;
    };

    template <typename T>
    concept ManagesBindless = requires(T &t, const TextureHandle th, const BufferHandle bh, const BindlessIndex bi, const u32 n) {
        { t.RegisterBindless(th, TextureSubresource{}) } -> std::same_as<BindlessIndex>;
        { t.RegisterBindlessStorage(th, n) } -> std::same_as<BindlessIndex>;
        { t.RegisterBindless(bh) } -> std::same_as<BindlessIndex>;
        { t.UnregisterBindless(bi) } -> std::same_as<void>;
    };

    template <typename T>
    concept PlacesResources =
        requires(T &t, const T &ct, const HeapDescriptor &hd, const BufferDescriptor &bd, const TextureDescriptor &td, HeapHandle hh, u64 offset) {
            { t.CreateHeap(hd) } -> std::same_as<HeapHandle>;
            { t.DestroyHeap(hh) } -> std::same_as<bool>;
            { ct.QueryFootprint(bd) } -> std::same_as<ResourceFootprint>;
            { ct.QueryFootprint(td) } -> std::same_as<ResourceFootprint>;
            { t.CreateBufferPlaced(bd, hh, offset) } -> std::same_as<BufferHandle>;
            { t.CreateTexturePlaced(td, hh, offset) } -> std::same_as<TextureHandle>;
        };

    // template <typename T>
    // concept SubmitsWork = requires(T &t, TimelineValue tv, u32 n) {
    //     // { t.AcquireCommandList(QueueType::Graphics, n) } -> std::same_as<CommandList *>;
    //     { t.Submit(sd, lists) } -> std::same_as<TimelineValue>;
    //     { t.CompletedValue(QueueType::Graphics) } -> std::same_as<u32>;
    //     { t.WaitForTimeline(tv, 0ull) } -> std::same_as<DeviceStatus>;
    //     { t.CurrentEpoch() } -> std::same_as<u32>;
    //     { t.CompletedEpoch() } -> std::same_as<u32>;
    //     { t.AdvanceEpoch() } -> std::same_as<u32>;
    // };

    template <typename T>
    concept PresentsFrames = requires(T &t, const T &ct, TextureHandle &outBackbuffer, const Extent2D extent) {
        { t.AcquireNextImage(outBackbuffer) } -> std::same_as<DeviceStatus>;
        { t.Present() } -> std::same_as<DeviceStatus>;
        { t.ResizeSwapchain(extent) } -> std::same_as<DeviceStatus>;
        { ct.SwapchainFormat() } -> std::same_as<Format>;
    };

    template <typename T>
    concept ReportsStatistics = requires(T &t, const T &ct, const QueryHeapHandle qh, std::span<u32> out, const u32 n) {
        { t.ReadQueryResults(qh, n, out) } -> std::same_as<bool>;
        { ct.QueryMemory() } -> std::same_as<MemoryStats>;
    };

    template <typename T>
    concept RHIDevice = HasDeviceLifetime<T>   // Satisfies device lifetime concept
                        && ManagesHeap<T>      // and manages heap
                        && ManagesPipelines<T> // and manages pipelines
                        && ManagesBindless<T>  // and manages bindles resources
                        && PlacesResources<T>  // and satisfies placement resources concept
                        // && SubmitsWork<T>        // and submits rendering work
                        && PresentsFrames<T>     // and presents frames
                        && ReportsStatistics<T>; // and reports rendering statistics

} // namespace Vulkyrie
