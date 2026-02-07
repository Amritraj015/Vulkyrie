#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Renderer {
    enum class ShaderDataType : u32 {
        None = 0,
        Float,
        Float2,
        Float3,
        Float4,
        Mat3,
        Mat4,
        Int,
        Int2,
        Int3,
        Int4,
        Bool,
    };
}
