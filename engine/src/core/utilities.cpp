#include "core/utilities.h"
#include "core/logger.h"
#include <fstream>

namespace Vulkyrie {

    [[nodiscard]] std::optional<std::vector<std::byte>> ReadBytesFromFile(const std::filesystem::path &path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            return std::nullopt;
        }

        const std::streamsize size = file.tellg();

        if (0 > size) {
            return std::nullopt;
        }

        std::vector<std::byte> buffer(static_cast<usize>(size));
        file.seekg(0, std::ios::beg);

        if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
            return std::nullopt;
        }

        return std::make_optional(buffer);
    }

    std::optional<std::string> ReadTextFromFile(const std::filesystem::path &path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);

        if (!file) {
            VERROR("Failed to open file: {}", path.string());
            return std::nullopt;
        }

        const auto size = file.tellg();

        if (size < 0) {
            VERROR("Failed to determine file size: {}", path.string());
            return std::nullopt;
        }

        std::string contents(static_cast<size_t>(size), '\0');

        file.seekg(0, std::ios::beg);

        if (!file.read(contents.data(), size)) {
            VERROR("Failed to read file: {}", path.string());
            return std::nullopt;
        }

        return contents;
    }

} // namespace Vulkyrie
