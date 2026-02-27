# -------------------------------------
# Third-party dependencies
# -------------------------------------
# GLFW
# We don't need to build the examples, tests, or docs.
# Set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# Set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# Set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
# Set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
find_package(glfw3 CONFIG REQUIRED)
# -------------------------------------------------

# -------------------------------------------------
# GLM
find_package(glm CONFIG REQUIRED)
# -------------------------------------------------

# -------------------------------------------------
# ImGui
find_package(imgui CONFIG REQUIRED)
# -------------------------------------------------

# -------------------------------------------------
# Assimp 
# Set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# Set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
# Set(ASSIMP_NO_EXPORT OFF CACHE BOOL "" FORCE)
# Set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
# Set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
#
# # Set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
# Set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
# Set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
# Set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "" FORCE)
#
# # No need to build zlib if we're using the system one
# Set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "" FORCE)
find_package(assimp CONFIG REQUIRED)
# -------------------------------------------------

# -------------------------------------------------
# STB
# find_package(stb CONFIG REQUIRED)
# -------------------------------------------------

# -------------------------------------------------
# GLAD
Add_Library(glad STATIC external/glad/src/glad.c)
Target_Include_Directories(glad PUBLIC external/glad/include) # TODO Change this to private include directory.
