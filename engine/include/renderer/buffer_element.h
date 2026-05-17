#pragma once

#include "shader_data_type.h"

namespace Vulkyrie {
    /** @brief Represents an element in a buffer layout, defining its type, name, size, offset, and normalization. */
    class BufferElement {
    public:
        /** Data type for the buffer element. */
        ShaderDataType Type;

        /** Name of the buffer element. */
        std::string_view Name;

        /** Size of the buffer element in bytes. */
        u32 Size;

        // TODO: Try to make this a const as well.
        // TODO: This is currently calculated in BufferLayout's CalculateOffsetsAndStride method.
        /** Offset of the buffer element in the buffer layout. */
        size_t Offset;

        /** Whether the buffer element is normalized. */
        bool Normalized;

        BufferElement() {
        }

        /** @brief Constructs a BufferElement with the given type, name, and normalization flag.
         * @param type The shader data type of the buffer element.
         * @param name The name of the buffer element.
         * @param normalized Whether the buffer element is normalized.
         */
        BufferElement(const ShaderDataType type, const std::string_view name, const bool normalized = false)
            : Type(type)
            , Name(name)
            , Size(GetShaderDataTypeSize(type))
            , Offset(0)
            , Normalized(normalized) {
        }

        /** @brief Gets the number of components in the buffer element based on its shader data type.
         * @returns The number of components.
         */
        [[nodiscard]] inline constexpr i32 GetComponentCount() const {
            switch (Type) {
                case ShaderDataType::Float:
                case ShaderDataType::Int:
                case ShaderDataType::Bool:
                    return 1;
                case ShaderDataType::Float2:
                case ShaderDataType::Int2:
                    return 2;
                case ShaderDataType::Float3:
                case ShaderDataType::Int3:
                    return 3;
                case ShaderDataType::Float4:
                case ShaderDataType::Int4:
                    return 4;
                case ShaderDataType::Mat3:
                    return 9; // 3 columns and 3 rows => 3 * 3 = 9 components
                case ShaderDataType::Mat4:
                    return 16; // 4 columns and 4 rows => 4 * 4 = 16 components
                default:
                    return 0;
            }
        }

    private:
        /** @brief Gets the size in bytes of the given shader data type.
         * @param type The shader data type.
         * @returns The size in bytes.
         */
        [[nodiscard]] constexpr static u32 GetShaderDataTypeSize(ShaderDataType type) {
            switch (type) {
                case ShaderDataType::Float:
                    return 4; // 1 * (size of float) => 1 * 4 = 4 bytes
                case ShaderDataType::Float2:
                    return 8; // 2 * (size of float) => 2 * 4 = 8 bytes
                case ShaderDataType::Float3:
                    return 12; // 3 * (size of float) => 3 * 4 = 12 bytes
                case ShaderDataType::Float4:
                    return 16; // 4 * (size of float) => 4 * 4 = 16 bytes
                case ShaderDataType::Mat3:
                    return 36; // 3 * 3 * (size of float) => 3 * 3 * 4 = 36 bytes
                case ShaderDataType::Mat4:
                    return 64; // 4 * 4 * (size of float) => 4 * 4 * 4 = 64 bytes
                case ShaderDataType::Int:
                    return 4; // 1 * (size of int) => 1 * 4 = 4 bytes
                case ShaderDataType::Int2:
                    return 8; // 2 * (size of int) => 2 * 4 = 8 bytes
                case ShaderDataType::Int3:
                    return 12; // 3 * (size of int) => 3 * 4 = 12 bytes
                case ShaderDataType::Int4:
                    return 16; // 4 * (size of int) => 4 * 4 = 16 bytes
                case ShaderDataType::Bool:
                    return 1; // 1 * (size of bool) => 1 * 1 = 1 byte
                default:
                    return 0;
            }
        }
    };
} // namespace Vulkyrie
