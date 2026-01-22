#include "renderer/open_gl/open_gl_mesh.h"
#include "glad/glad.h"

namespace Vulkyrie::Renderer {
    // constructor
    OpenGLMesh::OpenGLMesh(std::vector<Vertex> &&vertices, std::vector<u32> &&indices, std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> &&textures)
        : Mesh(std::move(vertices), std::move(indices), std::move(textures)) {
        SetupMesh();
    }

    // render the mesh
    inline void OpenGLMesh::Draw([[maybe_unused]] Shader &shader) const {
        // bind appropriate textures
        u32 ambientNr = 1;
        u32 diffuseNr = 1;
        u32 specularNr = 1;
        u32 normalNr = 1;
        u32 heightNr = 1;

        for (u32 i = 0; i < _textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding

            switch (_textures[i].first) {
                case MeshTextureType::Ambient:
                    shader.SetIntUniform(("texture_ambient" + std::to_string(ambientNr++)).c_str(), i);
                    break;
                case MeshTextureType::Diffuse:
                    shader.SetIntUniform(("texture_diffuse" + std::to_string(diffuseNr++)).c_str(), i);
                    break;
                case MeshTextureType::Specular:
                    shader.SetIntUniform(("texture_specular" + std::to_string(specularNr++)).c_str(), i);
                    break;
                case MeshTextureType::Normal:
                    shader.SetIntUniform(("texture_normal" + std::to_string(normalNr++)).c_str(), i);
                    break;
                case MeshTextureType::Height:
                    shader.SetIntUniform(("texture_height" + std::to_string(heightNr++)).c_str(), i);
                    break;
            }

            // and finally bind the texture
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

        const auto vertexBuffer = VertexBuffer::Create(Vulkyrie::Core::GraphicsAPI::OpenGL, _vertices);
        vertexBuffer->SetLayout(Vertex::GetLayout());
        _vertexArray->AddVertexBuffer(vertexBuffer);

        const auto indexBuffer = IndexBuffer::Create(Vulkyrie::Core::GraphicsAPI::OpenGL, _indices.data(), _indices.size());
        _vertexArray->SetIndexBuffer(indexBuffer);
    }
} // namespace Vulkyrie::Renderer
