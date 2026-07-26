# ------------------------------------------------
# Third-party dependencies (vcpkg provided ports)
# ------------------------------------------------

# Threads (std::jthread / std::thread need pthreads on Linux)
find_package(Threads REQUIRED)

# GLFW
find_package(glfw3 CONFIG REQUIRED)

# GLAD
find_package(glad CONFIG REQUIRED)

# GLM
find_package(glm CONFIG REQUIRED)

# ImGui
find_package(imgui CONFIG REQUIRED)

# Assimp 
find_package(assimp CONFIG REQUIRED)

# OpenAL
find_package(OpenAL CONFIG REQUIRED)

# STB
find_package(Stb REQUIRED)

# Catch2 (test framework, and the harness the benchmarks are written against)
if(VULKYRIE_BUILD_TESTS OR VULKYRIE_BUILD_BENCHMARKS)
    find_package(Catch2 CONFIG REQUIRED)
endif()
