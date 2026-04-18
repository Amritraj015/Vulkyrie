#pragma once

#include "vlkypch.h"

namespace Vulkyrie {
    /** Reads the contents of a file at the given path and returns it as a string.
     * @param path The path to the file to read.
     * @returns The contents of the file as a string.
     */
    std::string ReadTextFromFile(const std::filesystem::path &path);

} // namespace Vulkyrie
