#include "renderer/backends/device.h"
#include "renderer/renderer.h"
#include "renderer/backends/backend_concepts.h"

namespace Vulkyrie {

    template <RendererBackend B> class RendererImpl final : public Renderer {
    public:
        explicit RendererImpl(const DeviceCreationInfo &info)
            : mDevice(info) {
        }

        VE_DELETE_MOVE_AND_COPY(RendererImpl);

        ~RendererImpl() override {
            if (mDevice.ContextCreated()) {
                mDevice.WaitIdle();
            }
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
            // TODO: drive the frame graph once render passes exist.
        }

        VE_INLINE void WaitIdle() override {
            mDevice.WaitIdle();
        }

        [[nodiscard]] VE_INLINE bool DeviceLost() const noexcept override {
            return mDevice.DeviceLost();
        }

        [[nodiscard]] VE_INLINE const RendererStatistics &GetStatistics() const noexcept override {
            return stats;
        }

        [[nodiscard]] VE_INLINE bool ContextCreated() const noexcept override {
            return mDevice.ContextCreated();
        }

    private:
        Device<B> mDevice;
        typename B::Swapchain mSwapchain;
        RendererStatistics stats;
    };

} // namespace Vulkyrie
