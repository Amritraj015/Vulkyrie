#pragma once

#include "renderer/mesh.h"

namespace Vulkyrie::Renderer {
    class OpenGLMesh : public Mesh {
        public:
            OpenGLMesh(std::vector<Vertex> &&vertices, std::vector<u32> &&indices, std::vector<Ref<Texture2D>> &&textures);

            // render the mesh
            void Draw(Shader &shader) const override;

        private:
            void SetupMesh();
    };
} // namespace Vulkyrie::Renderer
