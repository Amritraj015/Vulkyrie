#pragma once

#include "defines.h"
#include "buffer_element.h"

namespace Vulkyrie::Renderer {
    class BufferLayout {
        public:
            BufferLayout() {
            }

            BufferLayout(std::initializer_list<BufferElement> elements) : _elements(elements) {
                CalculateOffsetsAndStride();
            }

            [[nodiscard]] constexpr const std::vector<BufferElement> &GetElements() const {
                return _elements;
            }

            [[nodiscard]] constexpr i32 GetStride() const {
                return _stride;
            }

            std::vector<BufferElement>::iterator begin() {
                return _elements.begin();
            }
            std::vector<BufferElement>::iterator end() {
                return _elements.end();
            }
            std::vector<BufferElement>::const_iterator begin() const {
                return _elements.begin();
            }
            std::vector<BufferElement>::const_iterator end() const {
                return _elements.end();
            }

        private:
            std::vector<BufferElement> _elements;
            i32 _stride = 0;

            void CalculateOffsetsAndStride() {
                size_t offset = 0;
                _stride = 0;

                for (auto &element : _elements) {
                    element.Offset = offset;
                    offset += element.Size;
                    _stride += element.Size;
                }
            }
    };
} // namespace Vulkyrie::Renderer
