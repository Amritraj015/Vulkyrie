# -------------------------------------
# Global configuration
# -------------------------------------
cmake_minimum_required(VERSION 3.27)
project(vulkyrie)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -------------------------------------
# Third-party dependencies
# -------------------------------------
# Fetch GLFW
include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

# Optional: disable stuff you don't need
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)
# -------------------------------------------------

# GLAD (simple static library)
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# -------------------------------------
# Build the Vulkyrie Engine.
# -------------------------------------
add_subdirectory(engine)

# -------------------------------------
# Build the Vulkyrie CLI tool.
# -------------------------------------
add_subdirectory(vulky-cli)

# -------------------------------------
# Games based on the Vulkyrie engine
# -------------------------------------
# add_subdirectory(games/asteroids)
add_subdirectory(games/pong)
