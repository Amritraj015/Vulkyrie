#include "renderer/open_gl/open_gl_mesh.h"
#include "glad/glad.h"
#include "vlkypch.h"

namespace Vulkyrie::Renderer {
    OpenGLMesh::OpenGLMesh(std::vector<Vertex> &&vertices, std::vector<u32> &&indices, std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> &&textures)
        : Mesh(std::move(vertices), std::move(indices), std::move(textures)) {
        SetupMesh();
    }

    inline void OpenGLMesh::Draw(Shader &shader) const {
        for (u32 i = 0; i < _textures.size(); i++) {
            // Activate proper texture unit before binding
            glActiveTexture(GL_TEXTURE0 + i);

            // Set the sampler uniform.
            shader.SetIntUniform(_textureUniformNames[i].c_str(), i);

            // Bind the texture.
            glBindTexture(GL_TEXTURE_2D, _textures[i].second->GetTextureID());
        }

        // Draw mesh
        _vertexArray->Bind();
        glDrawElements(GL_TRIANGLES, _vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, 0);
        _vertexArray->Unbind();

        // Always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
    }

    // initializes all the buffer objects/arrays
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
        _textureUniformNames.reserve(_textures.size());

        u32 ambientNr = 1;
        u32 diffuseNr = 1;
        u32 specularNr = 1;
        u32 normalNr = 1;
        u32 heightNr = 1;

        for (const auto &textureData : _textures) {
            std::string name;
            switch (textureData.first) {
                case MeshTextureType::Ambient:
                    name = "texture_ambient" + std::to_string(ambientNr++);
                    break;
                case MeshTextureType::Diffuse:
                    name = "texture_diffuse" + std::to_string(diffuseNr++);
                    break;
                case MeshTextureType::Specular:
                    name = "texture_specular" + std::to_string(specularNr++);
                    break;
                case MeshTextureType::Normal:
                    name = "texture_normal" + std::to_string(normalNr++);
                    break;
                case MeshTextureType::Height:
                    name = "texture_height" + std::to_string(heightNr++);
                    break;
            }

            _textureUniformNames.emplace_back(std::move(name));
        }
    }
} // namespace Vulkyrie::Renderer
