#include "renderer/backends/device.h"
#include "renderer/renderer.h"
#include "renderer/backends/backend_concepts.h"

namespace Vulkyrie {

    template <RendererBackend B> class RendererImpl final : public Renderer {
    public:
        explicit RendererImpl(const DeviceCreationInfo &info);

        VE_DELETE_MOVE_AND_COPY(RendererImpl);

        ~RendererImpl() override {
            mDevice.WaitIdle();
        }

        [[nodiscard]] GraphicsAPI BackendType() const noexcept override;
        [[nodiscard]] const DeviceCapabilities &QueryCapabilities() const override;

        void OnWindowResize(u32 width, u32 height) override;
        void Render() override;
        void WaitIdle() override;

        [[nodiscard]] bool DeviceLost() override;
        [[nodiscard]] const RendererStatistics &GetStatistics() const noexcept override;

    private:
        Device<B> mDevice;
        typename B::Swapchain mSwapchain;
    };

} // namespace Vulkyrie
