#pragma once

#include "renderer/shader.h"
#include "renderer/texture_2D.h"
#include "renderer/vertex_array.h"
#include "renderer/vertex.h"

namespace Vulkyrie::Renderer {
    enum class MeshTextureType : u8 {
        Ambient,
        Diffuse,
        Specular,
        Normal,
        Height,
    };

    // struct MeshTexture {
    //     public:
    //         u32 Id;
    //         TextureType Type;
    //         std::string Path;
    // };

    class Mesh {
        public:
            /** @brief Virtual destructor for the Mesh class. */
            virtual ~Mesh() = default;

            /** @brief Draws the mesh using the specified shader.
             * @param shader The shader to use for rendering.
             */
            // TODO: Needs to be removed.
            virtual void Draw(Shader &shader) const = 0;

            /** @brief Creates a mesh with the specified vertex array and textures.
             * @param api The graphics API to use.
             * @param vertexArray The vertex array representing the mesh geometry.
             * @param textures The textures associated with the mesh.
             * @return A reference to the created Mesh.
             */
            static Ref<Mesh> Create(Vulkyrie::Core::GraphicsAPI api, std::vector<Vertex> &&vertices, std::vector<u32> &&indices, std::vector<Ref<Texture2D>> &&textures);

        protected:
            /** @brief Constructs a mesh with the specified vertices, indices, and textures.
             * @param vertices The vertices that make up the mesh.
             * @param indices The indices defining the mesh's faces.
             * @param textures The textures associated with the mesh.
             */
            Mesh(std::vector<Vertex> &&vertices, std::vector<u32> &&indices, std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> &&textures)
                : _vertices(std::move(vertices)), _indices(std::move(indices)), _textures(std::move(textures)) {
            }

            /** @brief The vertices that make up the mesh. */
            std::vector<Vertex> _vertices;

            /** @brief The indices defining the mesh's faces. */
            std::vector<u32> _indices;

            /** @brief The textures associated with the mesh. */
            std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> _textures;

            /** @brief The vertex array representing the mesh geometry. */
            Ref<VertexArray> _vertexArray;
    };
} // namespace Vulkyrie::Renderer
