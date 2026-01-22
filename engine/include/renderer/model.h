#pragma once

#include <unordered_map>
#include "vlkypch.h"
#include "renderer/mesh.h"

namespace Vulkyrie::Renderer {
    /** @brief Represents a 3D model composed of multiple meshes. */
    class Model {
        public:
            /** @brief Creates a model from the specified file path.
             * @param api The graphics API to use.
             * @param path The file path to the 3D model.
             * @param gamma Whether to apply gamma correction. Default is false.
             * @return A reference to the created Model.
             */
            static Ref<Model> Create(Vulkyrie::Core::GraphicsAPI api, const std::filesystem::path &path, bool gamma = false);

            /** @brief Draws the model using the specified shader.
             * @param shader The shader to use for rendering.
             */
            // TODO: Needs to be removed.
            inline void Draw(Vulkyrie::Renderer::Shader &shader) const {
                for (auto &mesh : _meshes) {
                    mesh->Draw(shader);
                }
            }

        protected:
            Model(const std::string &path, bool gammaCorrection) : _path(path), _gammaCorrection(gammaCorrection) {
            }

            /** @brief The directory path of the model file. */
            std::filesystem::path _path;

            /** @brief Indicates whether gamma correction is applied. */
            bool _gammaCorrection;

            /** @brief The directory where the model is located. */
            std::filesystem::path _modelDirectory;

            // TODO: stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
            std::unordered_map<std::string, Ref<Texture2D>> _loadedTextures;

            /** @brief The meshes that make up the model. */
            std::vector<Ref<Vulkyrie::Renderer::Mesh>> _meshes;
    };
} // namespace Vulkyrie::Renderer
