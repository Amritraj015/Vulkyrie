#include "core/utilities.h"
#include "core/logger.h"
#include <fstream>

namespace Vulkyrie::Core {
    /** Reads the contents of a file at the given path and returns it as a string.
     * @param path The path to the file to read.
     * @returns The contents of the file as a string.
     */
    std::string ReadTextFromFile(const std::filesystem::path &path) {
        // Open the file.
        std::ifstream file(path, std::ios::in | std::ios::binary);

        // Check if file opened successfully.
        if (!file.is_open()) {
            // If the file failed to open, log an error and return an empty string.
            VERROR("Failed to open file: {}", path.c_str());

            return {};
        }

        // Read the file contents into a string.
        std::string contents;
        file.seekg(0, std::ios::end);
        contents.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(contents.data(), contents.size());

        // Close the file.
        file.close();

        // Return the file contents.
        return contents;
    }
} // namespace Vulkyrie::Core
