#include "renderer/open_gl/open_gl_model.h"
#include "vendor/stb_image.h"
#include "core/logger.h"

namespace Vulkyrie::Renderer {
    OpenGLModel::OpenGLModel(std::filesystem::path const &path, bool gammaCorrection) : Model(path, gammaCorrection) {
        stbi_set_flip_vertically_on_load(true);

        LoadModel(path);

        stbi_set_flip_vertically_on_load(false);
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
        indices.reserve(mesh->mNumFaces * 3);

        std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> textures;

        // walk through each of the mesh's vertices
        for (u32 i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex{};

            // Positions
            if (mesh->HasPositions()) {
                vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            }

            // Normals
            if (mesh->HasNormals()) {
                vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            }

            // TODO: A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
            // TODO: use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            if (mesh->HasTextureCoords(0)) {
                vertex.TextureCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            } else {
                vertex.TextureCoords = glm::vec2(0.0f, 0.0f);
            }

            // Tangents and Bitangents
            if (mesh->HasTangentsAndBitangents()) {
                // Tangent
                vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);

                // Bitangent
                vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
            }

            vertices.emplace_back(std::move(vertex));
        }

        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (u32 i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];

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
        LoadMaterialTextures(textures, material, aiTextureType_AMBIENT, MeshTextureType::Ambient);
        LoadMaterialTextures(textures, material, aiTextureType_DIFFUSE, MeshTextureType::Diffuse);
        LoadMaterialTextures(textures, material, aiTextureType_SPECULAR, MeshTextureType::Specular);
        LoadMaterialTextures(textures, material, aiTextureType_HEIGHT, MeshTextureType::Height);
        LoadMaterialTextures(textures, material, aiTextureType_NORMALS, MeshTextureType::Normal);

        // Return a mesh object created from the extracted mesh data.
        return CreateRef<OpenGLMesh>(std::move(vertices), std::move(indices), std::move(textures));
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    void OpenGLModel::LoadMaterialTextures(std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> &textures, aiMaterial *mat, aiTextureType type, MeshTextureType textureType) {
        const u32 textureCount = mat->GetTextureCount(type);
        textures.reserve(textures.size() + textureCount);

        for (u32 i = 0; i < textureCount; i++) {
            aiString str;
            mat->GetTexture(type, i, &str);

            const std::filesystem::path texturePath = (_modelDirectory / str.C_Str()).lexically_normal();
            const std::string cacheKey = texturePath.generic_string();

            // Try to find existing texture in cache
            auto it = _loadedTextures.find(cacheKey);
            if (it != _loadedTextures.end()) {
                textures.emplace_back(textureType, it->second);
                continue;
            }

            // Load new texture and cache it
            auto texture = Texture2D::Create(Vulkyrie::Core::GraphicsAPI::OpenGL, texturePath);
            textures.emplace_back(textureType, texture);
            _loadedTextures.emplace(std::move(cacheKey), texture);
        }
    }
} // namespace Vulkyrie::Renderer
