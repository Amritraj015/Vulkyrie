#include "renderer/open_gl/open_gl_mesh.h"
#include "core/asserts.h"
#include "vlkypch.h"
#include "glad/glad.h"

namespace Vulkyrie::Renderer {
    OpenGLMesh::OpenGLMesh(std::vector<Vertex> &&vertices, std::vector<u32> &&indices, MeshTextures &&textures)
        : Mesh(std::move(vertices), std::move(indices), std::move(textures)) {
        SetupMesh();
    }

    inline void OpenGLMesh::Draw(Shader &shader) const {
        u32 unit = 0;

        std::array<const std::vector<Ref<Texture2D>> *, 5> availableTextures = {
            &_textures.Ambient, &_textures.Diffuse, &_textures.Specular, &_textures.Normal, &_textures.Height
        };

        for (auto typeVec : availableTextures) {
            for (const auto &texture : *typeVec) {
                shader.SetIntUniform(_textureUniformNames[unit].c_str(), static_cast<int>(unit));
                texture->Bind(unit++);
            }
        }

        // Draw the mesh
        _vertexArray->Bind();
        glDrawElements(GL_TRIANGLES, _vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
        _vertexArray->Unbind();
    }

    void OpenGLMesh::SetupMesh() {
        _vertexArray = VertexArray::Create(Vulkyrie::Core::GraphicsAPI::OpenGL);

        const Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(Vulkyrie::Core::GraphicsAPI::OpenGL, _vertices);
        vertexBuffer->SetLayout(Vertex::GetLayout());
        _vertexArray->AddVertexBuffer(vertexBuffer);

        const Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(Vulkyrie::Core::GraphicsAPI::OpenGL, _indices.data(), _indices.size());
        _vertexArray->SetIndexBuffer(indexBuffer);

        // Release CPU-side copies now that the data lives on the GPU.
        std::vector<Vertex>().swap(_vertices);
        std::vector<u32>().swap(_indices);

        // Prepare texture uniform names
        _textureUniformNames.clear();
        _textureUniformNames.reserve(_textures.TotalCount());

        u32 ambientNr = 1;
        u32 diffuseNr = 1;
        u32 specularNr = 1;
        u32 normalNr = 1;
        u32 heightNr = 1;

        // Iterate over each texture type and generate uniform names
        for (size_t i = 0; i < _textures.Ambient.size(); i++) {
            std::string name = "texture_ambient" + std::to_string(ambientNr++);
            _textureUniformNames.emplace_back(std::move(name));
        }

        for (size_t i = 0; i < _textures.Diffuse.size(); i++) {
            std::string name = "texture_diffuse" + std::to_string(diffuseNr++);
            _textureUniformNames.emplace_back(std::move(name));
        }

        for (size_t i = 0; i < _textures.Specular.size(); i++) {
            std::string name = "texture_specular" + std::to_string(specularNr++);
            _textureUniformNames.emplace_back(std::move(name));
        }

        for (size_t i = 0; i < _textures.Normal.size(); i++) {
            std::string name = "texture_normal" + std::to_string(normalNr++);
            _textureUniformNames.emplace_back(std::move(name));
        }

        for (size_t i = 0; i < _textures.Height.size(); i++) {
            std::string name = "texture_height" + std::to_string(heightNr++);
            _textureUniformNames.emplace_back(std::move(name));
        }

#if defined(VULKYRIE_DEBUG)
        GLint maxUnits;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);

        VASSERT(_textureUniformNames.size() == _textures.TotalCount(),
                "Texture/uniform mismatch: {} names vs {} textures",
                _textureUniformNames.size(),
                _textures.TotalCount());
#endif
    }
} // namespace Vulkyrie::Renderer
