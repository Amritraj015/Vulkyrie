#include "renderer/open_gl/open_gl_mesh.h"
#include "core/asserts.h"
#include "vlkypch.h"
#include "glad/glad.h"

namespace Vulkyrie::Renderer {
    /**
     * @brief Constructs an OpenGL mesh from vertex, index, and texture data.
     *
     * Initializes the mesh by transferring ownership of the provided data and setting up
     * GPU buffers. The vertex and index data are uploaded to the GPU and then released
     * from CPU memory to save resources.
     *
     * @param vertices The vertex data for the mesh (moved)
     * @param indices The index data defining mesh faces (moved)
     * @param textures The texture resources organized by type (moved)
     */
    OpenGLMesh::OpenGLMesh(std::vector<Vertex> &&vertices, std::vector<u32> &&indices, MeshTextures &&textures)
        : Mesh(std::move(vertices), std::move(indices), std::move(textures)) {
        SetupMesh();
    }

    /**
     * @brief Renders the mesh using the specified shader.
     *
     * Binds all textures to their corresponding texture units and sets the appropriate
     * shader uniforms. The texture binding follows the naming convention:
     * - texture_ambient1, texture_ambient2, ...
     * - texture_diffuse1, texture_diffuse2, ...
     * - texture_specular1, texture_specular2, ...
     * - texture_normal1, texture_normal2, ...
     * - texture_height1, texture_height2, ...
     *
     * @param shader The shader program to use for rendering
     */
    inline void OpenGLMesh::Draw(Shader &shader) const {
        // Texture unit counter - increments as we bind each texture
        u32 unit = 0;

        // Array of pointers to texture vectors for efficient iteration
        std::array<const std::vector<Ref<Texture2D>> *, 5> availableTextures = {
            &_textures.Ambient, &_textures.Diffuse, &_textures.Specular, &_textures.Normal, &_textures.Height
        };

        // Bind all textures in order and set their corresponding shader uniforms
        for (auto typeVec : availableTextures) {
            for (const auto &texture : *typeVec) {
                // Set the sampler uniform to the texture unit index
                shader.SetIntUniform(_textureUniformNames[unit].c_str(), static_cast<i32>(unit));

                // Bind texture to the current unit and increment
                texture->Bind(unit++);
            }
        }

        // Issue the draw call
        _vertexArray->Bind();
        glDrawElements(GL_TRIANGLES, _vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
        _vertexArray->Unbind();
    }

    /**
     * @brief Initializes GPU buffers and prepares texture uniform names.
     *
     * This method:
     * 1. Creates and configures the vertex array object (VAO)
     * 2. Uploads vertex and index data to GPU buffers
     * 3. Releases CPU-side copies to save memory
     * 4. Pre-generates texture sampler uniform names for efficient rendering
     */
    void OpenGLMesh::SetupMesh() {
        // ========================================
        // GPU Buffer Setup
        // ========================================
        _vertexArray = VertexArray::Create();

        // Create and configure vertex buffer with layout information
        const Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(_vertices);
        vertexBuffer->SetLayout(Vertex::GetLayout());
        _vertexArray->AddVertexBuffer(vertexBuffer);

        // Create and attach index buffer
        const Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(_indices.data(), _indices.size());
        _vertexArray->SetIndexBuffer(indexBuffer);

        // Release CPU-side copies now that the data lives on the GPU
        // Using swap idiom ensures immediate memory deallocation
        std::vector<Vertex>().swap(_vertices);
        std::vector<u32>().swap(_indices);

        // ========================================
        // Texture Uniform Name Generation
        // ========================================

        // Pre-generate uniform names to avoid string construction during rendering
        _textureUniformNames.clear();
        _textureUniformNames.reserve(_textures.TotalCount());

        // Counters for each texture type (starting from 1 per convention)
        u32 ambientNr = 1;
        u32 diffuseNr = 1;
        u32 specularNr = 1;
        u32 normalNr = 1;
        u32 heightNr = 1;

        // NOTE: Shader sampler naming convention:
        // Each texture type follows the pattern 'texture_<type>N' where N starts at 1
        // Example: texture_diffuse1, texture_diffuse2, texture_specular1, etc.

        // Generate names for ambient textures
        for (size_t i = 0; i < _textures.Ambient.size(); i++) {
            std::string name = "texture_ambient" + std::to_string(ambientNr++);
            _textureUniformNames.emplace_back(std::move(name));
        }

        // Generate names for diffuse textures
        for (size_t i = 0; i < _textures.Diffuse.size(); i++) {
            std::string name = "texture_diffuse" + std::to_string(diffuseNr++);
            _textureUniformNames.emplace_back(std::move(name));
        }

        // Generate names for specular textures
        for (size_t i = 0; i < _textures.Specular.size(); i++) {
            std::string name = "texture_specular" + std::to_string(specularNr++);
            _textureUniformNames.emplace_back(std::move(name));
        }

        // Generate names for normal maps
        for (size_t i = 0; i < _textures.Normal.size(); i++) {
            std::string name = "texture_normal" + std::to_string(normalNr++);
            _textureUniformNames.emplace_back(std::move(name));
        }

        // Generate names for height/displacement maps
        for (size_t i = 0; i < _textures.Height.size(); i++) {
            std::string name = "texture_height" + std::to_string(heightNr++);
            _textureUniformNames.emplace_back(std::move(name));
        }

#if defined(VULKYRIE_DEBUG)
        // ========================================
        // Debug Validation
        // ========================================

        // Query GPU's maximum supported texture units
        GLint maxUnits;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);

        // Verify that we generated the correct number of uniform names
        // This catches errors in the name generation logic above
        VASSERT(_textureUniformNames.size() == _textures.TotalCount(),
                "Texture/uniform mismatch: {} names vs {} textures",
                _textureUniformNames.size(),
                _textures.TotalCount());
#endif
    }
} // namespace Vulkyrie::Renderer
