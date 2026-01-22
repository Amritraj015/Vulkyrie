#pragma once

#include <string>

#include "renderer/mesh.h"

namespace Vulkyrie::Renderer {
    class OpenGLMesh : public Mesh {
        public:
            /** @brief Constructs an OpenGL mesh with the specified vertices, indices, and textures.
             * @param vertices The vertices that make up the mesh.
             * @param indices The indices defining the mesh's faces.
             * @param textures The textures associated with the mesh.
             */
            OpenGLMesh(std::vector<Vertex> &&vertices, std::vector<u32> &&indices, std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> &&textures);

            // render the mesh
            // TODO: Needs to be removed.
            void Draw(Shader &shader) const override;

        private:
            /** @brief Initializes all the buffer objects/arrays for the mesh. */
            void SetupMesh();

            /** @brief Stores the uniform names for the textures used in the mesh. */
            std::vector<std::string> _textureUniformNames;
    };
} // namespace Vulkyrie::Renderer
