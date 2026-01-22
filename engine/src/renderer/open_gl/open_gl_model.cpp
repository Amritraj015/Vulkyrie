#include "renderer/open_gl/open_gl_model.h"
#include "vendor/stb_image.h"
#include "core/logger.h"

namespace Vulkyrie::Renderer {
    OpenGLModel::OpenGLModel(std::filesystem::path const &path, bool gammaCorrection) : Model(path, gammaCorrection) {
        LoadModel(path);
    }

    void OpenGLModel::LoadModel(std::filesystem::path const &path) {
        // Read file via ASSIMP
        Assimp::Importer importer;
        const aiScene *scene =
            importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        // Make sure the import succeeded.
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            VERROR("Failed to load model from path: {}, Error: {}", path.string(), importer.GetErrorString());
            return;
        }

        // Retrieve the directory path of the filepath
        _modelDirectory = path.parent_path();

        // Process ASSIMP's root node recursively
        ProcessNode(scene->mRootNode, scene);
    }

    void OpenGLModel::ProcessNode(aiNode *node, const aiScene *scene) {
        // process each mesh located at the current node
        for (u32 i = 0; i < node->mNumMeshes; i++) {
            // the node object only contains indices to index the actual objects in the scene.
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            _meshes.push_back(ProcessMesh(mesh, scene));
        }

        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (u32 i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene);
        }
    }

    Ref<OpenGLMesh> OpenGLModel::ProcessMesh(aiMesh *mesh, const aiScene *scene) {
        // data to fill
        std::vector<Vertex> vertices;
        vertices.reserve(mesh->mNumVertices);

        std::vector<u32> indices;
        indices.reserve(mesh->mNumFaces * 3); // assuming each face is a triangle -> See aiProcess_Triangulate usage in LoadModel

        MeshTextures textures;

        // walk through each of the mesh's vertices
        for (u32 i = 0; i < mesh->mNumVertices; i++) {
            vertices.emplace_back(
                mesh->HasPositions() ? glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z) : glm::vec3{},             // Position
                mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3{},                  // Normal
                mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2{ 0.0f, 0.0f },  // TextureCoords
                mesh->HasTangentsAndBitangents() ? glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z) : glm::vec3{}, // Tangent
                mesh->HasTangentsAndBitangents() ? glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z)
                                                 : glm::vec3{}); // Bitangent
        }

        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (u32 i = 0; i < mesh->mNumFaces; i++) {
            const aiFace &face = mesh->mFaces[i];

            // retrieve all indices of the face and store them in the indices vector
            for (u32 j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        // process materials
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // Load all texture types directly into the textures vector
        LoadMaterialTextures(textures.Ambient, material, aiTextureType_AMBIENT);
        LoadMaterialTextures(textures.Diffuse, material, aiTextureType_DIFFUSE);
        LoadMaterialTextures(textures.Specular, material, aiTextureType_SPECULAR);
        LoadMaterialTextures(textures.Height, material, aiTextureType_HEIGHT);
        LoadMaterialTextures(textures.Normal, material, aiTextureType_NORMALS);

        // Return a mesh object created from the extracted mesh data.
        return CreateRef<OpenGLMesh>(std::move(vertices), std::move(indices), std::move(textures));
    }

    void OpenGLModel::LoadMaterialTextures(std::vector<Ref<Texture2D>> &out, aiMaterial *mat, aiTextureType type) {
        const u32 count = mat->GetTextureCount(type);
        out.reserve(count);

        for (u32 i = 0; i < count; i++) {
            aiString str;
            mat->GetTexture(type, i, &str);

            auto path = (_modelDirectory / str.C_Str()).lexically_normal();
            auto key = path.generic_string();

            auto it = _loadedTextures.find(key);
            if (it != _loadedTextures.end()) {
                out.emplace_back(it->second);
                continue;
            }

            auto texture = Texture2D::Create(Core::GraphicsAPI::OpenGL, path);
            out.emplace_back(texture);
            _loadedTextures.emplace(std::move(key), texture);
        }
    }
} // namespace Vulkyrie::Renderer
