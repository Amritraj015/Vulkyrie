#include "vlkypch.h"
#include "renderer/open_gl/open_gl_model.h"
#include <stb_image.h>

namespace Vulkyrie::Renderer {
    /**
     * @brief Constructs an OpenGL model by loading it from a file.
     *
     * @param path The filesystem path to the 3D model file (supports formats: OBJ, FBX, GLTF, etc.)
     * @param gammaCorrection Whether to apply gamma correction to textures
     */
    OpenGLModel::OpenGLModel(std::filesystem::path const &path, bool gammaCorrection)
        : Model(path, gammaCorrection) {
        LoadModel(path);
    }

    /**
     * @brief Loads a 3D model from a file using ASSIMP.
     *
     * This method uses ASSIMP to import the model with the following post-processing:
     * - Triangulation: Converts all primitives to triangles
     * - Normal generation: Generates smooth normals if not present
     * - UV flipping: Flips texture coordinates for OpenGL convention
     * - Tangent calculation: Computes tangent and bitangent vectors for normal mapping
     *
     * @param path The filesystem path to the model file
     */
    void OpenGLModel::LoadModel(std::filesystem::path const &path) {
        // Initialize ASSIMP importer and load the model file
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path.string(),
                                                 aiProcess_Triangulate |          // Convert all primitives to triangles
                                                     aiProcess_GenSmoothNormals | // Generate smooth normals
                                                     aiProcess_FlipUVs |          // Flip UVs on Y-axis for OpenGL
                                                     aiProcess_CalcTangentSpace); // Calculate tangent/bitangent for normal mapping

        // Validate that the model loaded successfully
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            VERROR("Failed to load model from path: {}, Error: {}", path.string(), importer.GetErrorString());
            return;
        }

        // Store the model's directory for resolving relative texture paths
        _modelDirectory = path.parent_path();

        // Recursively process the scene graph starting from the root node
        ProcessNode(scene->mRootNode, scene);
    }

    /**
     * @brief Recursively processes a node in the ASSIMP scene graph.
     *
     * The scene graph is a tree structure where:
     * - Each node may contain mesh indices (references to actual mesh data in the scene)
     * - Each node may have child nodes forming the hierarchy
     * - Nodes define transformations but don't own the actual mesh data
     *
     * @param node The current node to process
     * @param scene The ASSIMP scene containing all model data
     */
    void OpenGLModel::ProcessNode(aiNode *node, const aiScene *scene) {
        // Process all meshes referenced by this node
        // Note: The node only stores indices; actual mesh data lives in the scene
        for (u32 i = 0; i < node->mNumMeshes; i++) {
            // Retrieve the mesh from the scene using the node's index
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            // Process the mesh and add it to our collection
            _meshes.push_back(ProcessMesh(mesh, scene));
        }

        // Recursively process all child nodes in the scene graph
        for (u32 i = 0; i < node->mNumChildren; i++) {
            ProcessNode(node->mChildren[i], scene);
        }
    }

    /**
     * @brief Processes an ASSIMP mesh into our engine's mesh representation.
     *
     * This method extracts all mesh data from ASSIMP's format and converts it to
     * our engine's format. It handles:
     * - Vertex attributes (position, normal, UV, tangent, bitangent)
     * - Face indices (assuming triangulated mesh)
     * - Material textures (ambient, diffuse, specular, normal, height)
     *
     * Vertices are constructed directly in the vector's heap memory using designated
     * initializers for optimal performance.
     *
     * @param mesh The ASSIMP mesh to process
     * @param scene The ASSIMP scene containing material data
     * @return A reference to the created OpenGLMesh
     */
    Ref<OpenGLMesh> OpenGLModel::ProcessMesh(aiMesh *mesh, const aiScene *scene) {
        // ========================================
        // Initialize Data Structures
        // ========================================

        std::vector<Vertex> vertices;
        vertices.reserve(mesh->mNumVertices);

        std::vector<u32> indices;
        indices.reserve(mesh->mNumFaces * 3); // Each face is a triangle (ensured by aiProcess_Triangulate)

        MeshTextures textures;

        // ========================================
        // Extract Vertex Data
        // ========================================

        // Process each vertex and construct directly in the vector's heap memory
        // Uses designated initializers for clarity and optimal performance
        for (u32 i = 0; i < mesh->mNumVertices; i++) {
            vertices.emplace_back(
                // Position: Extract from mesh or default to origin
                mesh->HasPositions() ? glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z) : glm::vec3{},
                // Normal: Extract from mesh or default to zero vector
                mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3{},
                // Texture Coordinates: Use first UV set (index 0) or default to (0,0)
                mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2{ 0.0f, 0.0f },
                // Tangent: Required for normal mapping, extract if available
                mesh->HasTangentsAndBitangents() ? glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z) : glm::vec3{},
                // Bitangent: Completes the TBN matrix for normal mapping
                mesh->HasTangentsAndBitangents() ? glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z) : glm::vec3{});
        }

        // ========================================
        // Extract Index Data
        // ========================================

        // Walk through each face (triangle) and extract vertex indices
        for (u32 i = 0; i < mesh->mNumFaces; i++) {
            const aiFace &face = mesh->mFaces[i];

            // Each face should have exactly 3 indices (triangle) due to aiProcess_Triangulate
            for (u32 j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        // ========================================
        // Load Material Textures
        // ========================================

        // Retrieve the material associated with this mesh
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        // Load all texture types from the material
        // Textures are cached to avoid loading duplicates across meshes
        LoadMaterialTextures(textures.Ambient, material, aiTextureType_AMBIENT);
        LoadMaterialTextures(textures.Diffuse, material, aiTextureType_DIFFUSE);
        LoadMaterialTextures(textures.Specular, material, aiTextureType_SPECULAR);
        LoadMaterialTextures(textures.Height, material, aiTextureType_HEIGHT);
        LoadMaterialTextures(textures.Normal, material, aiTextureType_NORMALS);

        // Create and return the mesh with all extracted data
        return CreateRef<OpenGLMesh>(std::move(vertices), std::move(indices), std::move(textures));
    }

    /**
     * @brief Loads textures of a specific type from an ASSIMP material.
     *
     * This method implements texture caching to prevent loading the same texture
     * multiple times when it's shared across different meshes. The cache uses
     * normalized filesystem paths as keys for reliable duplicate detection.
     *
     * @param out The output vector to append loaded textures to
     * @param mat The ASSIMP material containing texture information
     * @param type The type of texture to load (ambient, diffuse, specular, etc.)
     */
    void OpenGLModel::LoadMaterialTextures(std::vector<Ref<Texture2D>> &out, aiMaterial *mat, aiTextureType type) {
        // Query how many textures of this type the material has
        const u32 count = mat->GetTextureCount(type);
        out.reserve(count);

        for (u32 i = 0; i < count; i++) {
            // Get the texture path from the material
            aiString str;
            mat->GetTexture(type, i, &str);

            // Resolve relative path and normalize it for consistent cache keys
            auto path = (_modelDirectory / str.C_Str()).lexically_normal();
            auto key = path.generic_string();

            // Check if we've already loaded this texture (cache lookup)
            auto it = _loadedTextures.find(key);
            if (it != _loadedTextures.end()) {
                // Texture found in cache - reuse it
                out.emplace_back(it->second);
                continue;
            }

            // Texture not in cache - load it from disk
            auto texture = Texture2D::Create(path);
            out.emplace_back(texture);
            // Cache the texture for future reuse
            _loadedTextures.emplace(std::move(key), texture);
        }
    }
} // namespace Vulkyrie::Renderer
