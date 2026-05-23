#pragma once

#include "renderer/mesh_textures.h"
#include "renderer/shader.h"
#include "renderer/vertex_array.h"
#include "renderer/vertex.h"

namespace Vulkyrie {
    class Mesh {
    public:
        /** @brief Virtual destructor for the Mesh class. */
        virtual ~Mesh() = default;

        /** @brief Draws the mesh using the specified shader.
         * @param shader The shader to use for rendering.
         */
        // TODO: Needs to be removed.
        virtual void Draw(Shader &shader) const = 0;

        /** @brief Gets the number of vertices in the mesh.
         * @returns The count of vertices.
         */
        // [[nodiscard]] VE_INLINE size_t GetVertexCount() const {
        //     return _vertexArray->GetVertexBuffer()->GetCount();
        // }

        // TODO: Needs to be removed.
        VE_INLINE void BindTextures() const {
            std::array<const std::vector<Ref<Texture2D>> *, 5> availableTextures = {
                &_textures.Ambient, &_textures.Diffuse, &_textures.Specular, &_textures.Normal, &_textures.Height
            };

            i32 unit = 0; // Texture unit counter - increments as we bind each texture

            // Bind all textures in order and set their corresponding shader uniforms
            for (auto typeVec : availableTextures) {
                for (const auto &texture : *typeVec) {
                    // Bind texture to the current unit and increment
                    texture->Bind(unit++);
                }
            }
        }

        /** @brief Gets the number of indices in the mesh.
         * @returns The count of indices.
         */
        [[nodiscard]] VE_INLINE size_t GetIndexCount() const {
            return _vertexArray->GetIndexBuffer()->GetCount();
        }

        /** @brief Binds the mesh's vertex array. */
        VE_INLINE void Bind() const {
            _vertexArray->Bind();
        }

        /** @brief Unbinds the mesh's vertex array. */
        VE_INLINE void Unbind() const {
            _vertexArray->Unbind();
        }

        /** @brief Creates a mesh with the specified vertex array and textures.
         * @param vertices The vertices that make up the mesh.
         * @param indices The indices defining the mesh's faces.
         * @param textures The textures associated with the mesh.
         * @returns A reference to the created Mesh.
         */
        static Ref<Mesh> Create(std::vector<Vertex> &&vertices, std::vector<u32> &&indices, MeshTextures &&textures);

    protected:
        /** @brief Constructs a mesh with the specified vertices, indices, and textures.
         * @param vertices The vertices that make up the mesh.
         * @param indices The indices defining the mesh's faces.
         * @param textures The textures associated with the mesh.
         */
        Mesh(std::vector<Vertex> &&vertices, std::vector<u32> &&indices, MeshTextures &&textures)
            : _vertices(std::move(vertices))
            , _indices(std::move(indices))
            , _textures(std::move(textures)) {
        }

        /** @brief The vertices that make up the mesh. */
        std::vector<Vertex> _vertices;

        /** @brief The indices defining the mesh's faces. */
        std::vector<u32> _indices;

        /** @brief The textures associated with the mesh. */
        MeshTextures _textures;

        /** @brief The vertex array representing the mesh geometry. */
        Ref<VertexArray> _vertexArray;
    };
} // namespace Vulkyrie
