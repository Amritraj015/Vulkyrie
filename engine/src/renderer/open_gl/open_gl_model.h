#pragma once

#include "renderer/model.h"
#include "renderer/mesh.h"
#include "renderer/open_gl/open_gl_mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Vulkyrie::Renderer {
    class OpenGLModel : public Vulkyrie::Renderer::Model {
        public:
            OpenGLModel(const std::filesystem::path &path, bool gammaCorrection = false);

        private:
            /** @brief Loads a model from the specified file path.
             * @param path The file path to the 3D model.
             */
            void LoadModel(std::filesystem::path const &path);

            /** @brief Processes a node in the ASSIMP scene graph recursively.
             * @param node The current node to process.
             * @param scene The ASSIMP scene containing the model data.
             */
            void ProcessNode(aiNode *node, const aiScene *scene);

            /** @brief Processes an individual mesh from the ASSIMP scene.
             * @param mesh The ASSIMP mesh to process.
             * @param scene The ASSIMP scene containing the model data.
             * @return A Mesh object representing the processed mesh.
             */
            Ref<OpenGLMesh> ProcessMesh(aiMesh *mesh, const aiScene *scene);

            /** @brief Loads material textures of a specified type from an ASSIMP material.
             * @param mat The ASSIMP material to load textures from.
             * @param type The type of texture to load (e.g., diffuse, specular).
             * @param textureType The corresponding MeshTextureType for the loaded textures.
             * @return A vector of references to the loaded Texture2D and it's type objects stored in std::pair.
             */
            std::vector<std::pair<MeshTextureType, Ref<Texture2D>>> LoadMaterialTextures(aiMaterial *mat, aiTextureType type, MeshTextureType textureType);
    };
} // namespace Vulkyrie::Renderer
