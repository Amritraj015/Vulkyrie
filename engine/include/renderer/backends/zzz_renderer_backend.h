#pragma once

#include "vlkypch.h"
#include "core/graphics_api.h"
#include "renderer/frame_buffer.h"
#include "renderer/index_buffer.h"
#include "renderer/shader.h"
#include "renderer/texture_2D.h"
#include "renderer/texture_cube_map.h"
#include "renderer/vertex_array.h"

namespace Vulkyrie {

    class RendererBackend {
    public:
        VE_DELETE_MOVE_AND_COPY(RendererBackend);

        virtual ~RendererBackend() = default;

        [[nodiscard]] VE_INLINE GraphicsAPI GetCurrentGraphicsAPI() {
            return _graphicsApi;
        }

        // -------------------------------------------------------------------------------------------------
        virtual StatusCode Initialize() = 0;
        virtual void SwapBuffers() = 0;
        [[nodiscard]] virtual BufferHandle CreateBuffer(std::span<f32> data) = 0;
        [[nodiscard]] virtual BufferHandle CreateBuffer(size_t size, std::span<f32> data) = 0;
        virtual void SetBufferData(const BufferHandle &handle, size_t startIndex, std::span<f32> data) = 0;
        virtual void DestroyBuffer(const BufferHandle &handle) = 0;
        // -------------------------------------------------------------------------------------------------

    protected:
        RendererBackend(GraphicsAPI api = GraphicsAPI::OpenGL)
            : _graphicsApi(api) {
        }

        IndexBuffer *_indexBuffer = nullptr;
        FrameBuffer *_frameBuffer = nullptr;
        Shader *_shader = nullptr;
        Texture2D *_texture = nullptr;
        TextureCubeMap *_cubeMapTexture = nullptr;
        VertexArray *_vertexArray = nullptr;
        VertexBuffer *_vertexBuffer = nullptr;

    private:
        GraphicsAPI _graphicsApi;
    };

} // namespace Vulkyrie
