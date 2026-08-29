#pragma once

#include "vlkypch.h"
#include "renderer/rhi/rhi_types.h"
#include "renderer/rhi/shader_types.h"

namespace Vulkyrie {

    struct ShaderCompileRequest final {
        std::filesystem::path SourcePath;
        StaticString EntryPoint = "main";
        ShaderStage Stage = ShaderStage::None;
        ShaderTarget Target = ShaderTarget::Count;
        std::span<const StaticString> Defines{};
        std::span<const std::filesystem::path> IncludeDirs{};
        bool GenerateDebugInfo = false;
        bool Optimize = true;
    };

    struct ShaderCompileResult final {
        std::vector<std::byte> Blob; // SPIR-V words or GLSL text
        ShaderKey ShaderKey{};
        std::string ErrorLog;

        [[nodiscard]] VE_INLINE bool Ok() const noexcept {
            return !Blob.empty();
        }
    };

    class ShaderCompiler {
    public:
        ShaderCompiler();

        VE_DELETE_MOVE_AND_COPY(ShaderCompiler);

        ~ShaderCompiler();

        // // Thread-safe: callable from job system workers.
        // [[nodiscard]] ShaderCompileResult Compile(const ShaderCompileRequest &);
        //
        // // Hash of source + resolved includes, without invoking the compiler.
        // [[nodiscard]] u64 ComputeSourceHash(std::string_view sourcePath, std::span<const std::string_view> includeDirs);

    private:
        // struct Impl; // holds the DXC instance; hides dxcapi.h
        // Impl *mImpl = nullptr;
    };
} // namespace Vulkyrie
