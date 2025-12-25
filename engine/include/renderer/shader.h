#pragma once

#include <filesystem>
#include "defines.h"

namespace Vulkyrie::Renderer {
    u32 CreateComputeShader(const std::filesystem::path &path);
    u32 ReloadComputeShader(u32 shaderHandle, const std::filesystem::path &path);

    u32 CreateGraphicsShader(const std::filesystem::path &vertexPath, const std::filesystem::path &fragmentPath);
    u32 ReloadGraphicsShader(u32 shaderHandle, const std::filesystem::path &vertexPath, const std::filesystem::path &fragmentPath);
}