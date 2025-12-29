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

fetchcontent_declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

# Optional: disable stuff you don't need
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

fetchcontent_makeavailable(glfw)
# -------------------------------------------------

# GLAD (simple static library)
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)
# -------------------------------------------------

# GLM
find_package(glm 1.0.1 QUIET)

message(STATUS "GLM found: ${glm_FOUND}")

if (NOT glm_FOUND)
    fetchcontent_declare(
        glm
        DOWNLOAD_EXTRACT_TIMESTAMP OFF
        URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
    )

    fetchcontent_makeavailable(glm)
endif()

if (TARGET glm)
    message("GLM target 'glm' found.")
    set_target_properties(glm PROPERTIES FOLDER "Dependencies")
elseif (TARGET glm::glm)
    message("GLM target 'glm::glm' found.")
    # If glm::glm exists but not glm, we can create an alias or just use glm::glm
    # But since the code uses target_link_libraries(Core glm), we might need the 'glm' target.
    add_library(glm ALIAS glm::glm)
endif()
# -------------------------------------------------

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
