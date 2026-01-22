#include "renderer/open_gl/open_gl_model.h"
#include "vendor/stb_image.h"
#include "core/logger.h"
#include <utility>

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
        textures.reserve(8);

        // walk through each of the mesh's vertices
        for (u32 i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;

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

            vertices.push_back(vertex);
        }

        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (u32 i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];

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

        // 1. Ambient maps
        std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> ambientMaps = LoadMaterialTextures(material, aiTextureType_AMBIENT, MeshTextureType::Ambient);
        textures.insert(textures.end(), ambientMaps.begin(), ambientMaps.end());

        // 2. diffuse maps
        std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, MeshTextureType::Diffuse);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // 3. Specular maps
        std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, MeshTextureType::Specular);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // 4. Height maps
        std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> heightMaps = LoadMaterialTextures(material, aiTextureType_HEIGHT, MeshTextureType::Height);
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        // 5. Normal maps
        std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> normalMaps = LoadMaterialTextures(material, aiTextureType_NORMALS, MeshTextureType::Normal);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        // Return a mesh object created from the extracted mesh data.
        return CreateRef<OpenGLMesh>(std::move(vertices), std::move(indices), std::move(textures));
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> OpenGLModel::LoadMaterialTextures(aiMaterial *mat, aiTextureType type, MeshTextureType textureType) {
        std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> textures;

        for (u32 i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;

            for (u32 j = 0; j < _loadedTextures.size(); j++) {
                if (std::strcmp(_loadedTextures[j]->GetTextureFileName().data(), str.C_Str()) == 0) {
                    textures.push_back(std::make_pair(textureType, _loadedTextures[j]));
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }

            // If texture hasn't been loaded already, load it.
            if (!skip) { 
                Ref<Texture2D> texture = Texture2D::Create(Vulkyrie::Core::GraphicsAPI::OpenGL, this->_modelDirectory.append(str.C_Str()));
                textures.push_back(std::make_pair(textureType, texture));

                // store it as texture loaded for entire model,
                //  to ensure we won't unnecessary load duplicate textures.
                _loadedTextures.push_back(texture); 
            }
        }

        return textures;
    }
} // namespace Vulkyrie::Renderer
