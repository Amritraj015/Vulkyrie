# ------------------------------------------------
# Third-party dependencies (vcpkg provided ports)
# ------------------------------------------------

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

# Catch2
if(VULKYRIE_BUILD_TESTS)
    find_package(Catch2 CONFIG REQUIRED)
endif()
