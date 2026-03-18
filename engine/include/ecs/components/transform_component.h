#pragma once

namespace Vulkyrie::ECS {

    struct TransformComponent final {
        public:
            glm::vec3 Position;
            glm::quat Rotation;
            f32 Scale;
    };

} // namespace Vulkyrie::ECS
