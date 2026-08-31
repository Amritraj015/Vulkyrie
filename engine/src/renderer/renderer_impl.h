#include "renderer/renderer.h"
#include "renderer/backend_concepts.h"
#include "renderer/common/device.h"
#include "renderer/frame_graph/frame_graph.h"

namespace Vulkyrie {

    template <RendererBackend B> class RendererImpl final : public Renderer {
    public:
        explicit RendererImpl(const DeviceCreationInfo &info)
            : mDevice(info)
            , mFrameGraph(mDevice)
            , mFrames(makeFrames(mDevice.Context(), info, std::make_index_sequence<B::kFramesInFlight>{})) {
        }

        VE_DELETE_MOVE_AND_COPY(RendererImpl);

        ~RendererImpl() override {
            if (mDevice.ContextCreated()) {
                mDevice.WaitIdle();
            }
        }

        [[nodiscard]] StatusCode InitializeContext() override {
            return mDevice.Context().Initialize();
        }

        [[nodiscard]] VE_INLINE GraphicsAPI BackendType() const noexcept override {
            return B::kType;
        }

        [[nodiscard]] VE_INLINE const DeviceCapabilities &QueryCapabilities() const override {
            return mDevice.QueryCapabilities();
        }

        void OnWindowResize(u32 width, u32 height) override {
            // TODO: recreate the swapchain once one exists; no-op until the frame graph owns presentation.
            (void)width;
            (void)height;
        }

        void Render() override {
            mDevice.GetDeletionQueue().Collect(mStats.FrameIndex);
            mDevice.Transients().ResetFrame();
            mFrameGraph.Reset();

            auto &frame = mFrames[mStats.FrameIndex % B::kFramesInFlight];

            mFrameGraph.Compile();
            mFrameGraph.Execute(frame);

            mDevice.Context().test();

            ++mStats.FrameIndex;
        }

        VE_INLINE void WaitIdle() override {
            mDevice.WaitIdle();
        }

        [[nodiscard]] VE_INLINE bool DeviceLost() const noexcept override {
            return mDevice.DeviceLost();
        }

        [[nodiscard]] VE_INLINE const RendererStatistics &GetStatistics() const noexcept override {
            return mStats;
        }

        [[nodiscard]] VE_INLINE bool ContextCreated() const noexcept override {
            return mDevice.ContextCreated();
        }

    private:
        /** @brief Constructs one `FrameContext` per frame in flight, each told which slot it owns.
         *
         * A `FrameContext` is not default-constructible - it cannot size its command lists without knowing the
         * worker count - so the array is built by expanding the pack directly into aggregate initialisation, where
         * each element is initialised in place from a prvalue rather than moved into position.
         * @param context The backend context the frames record against.
         * @param info Creation info; supplies the worker count.
         * @returns The per-frame contexts, indexed by slot. */
        template <size_t... TSlots>
        [[nodiscard]] static std::array<FrameContext<B>, sizeof...(TSlots)>
        makeFrames(typename B::Context &context, const DeviceCreationInfo &info, std::index_sequence<TSlots...>) {
            return std::array<FrameContext<B>, sizeof...(TSlots)>{ FrameContext<B>{ context, static_cast<u32>(TSlots), info.WorkerCount, 0 }... };
        }

        Device<B> mDevice;
        typename B::Swapchain mSwapchain;
        RendererStatistics mStats;
        FrameGraph<B> mFrameGraph;
        std::array<FrameContext<B>, B::kFramesInFlight> mFrames;
    };

} // namespace Vulkyrie
