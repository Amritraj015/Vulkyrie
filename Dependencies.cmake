# ------------------------------------------------
# Third-party dependencies (vcpkg provided ports)
# ------------------------------------------------
find_package(Threads REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)
# find_package(slang CONFIG REQUIRED)

# STB Image header path.
find_path(STB_INCLUDE_DIRS "stb_image.h")
# find_package(Stb REQUIRED)

# Catch2 (test framework, and the harness the benchmarks are written against)
if(VULKYRIE_BUILD_TESTS OR VULKYRIE_BUILD_BENCHMARKS)
    find_package(Catch2 CONFIG REQUIRED)
endif()
