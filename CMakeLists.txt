# -------------------------------------
# Global configuration
# -------------------------------------
Cmake_Minimum_Required(VERSION 3.27)
Project(vulkyrie)
Set(CMAKE_CXX_STANDARD 23)
Set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -------------------------------------
# Third-party dependencies
# -------------------------------------
# Fetch GLFW
Include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

# Optional: disable stuff you don't need
Set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
Set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
Set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
Set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)
# -------------------------------------------------

# GLAD (simple static library)
Add_Library(glad STATIC external/glad/src/glad.c)
Target_Include_Directories(glad PUBLIC external/glad/include)
# -------------------------------------------------

# GLM
Find_Package(glm 1.0.1 QUIET)

Message(STATUS "GLM found: ${glm_FOUND}")

if (NOT glm_FOUND)
    FetchContent_Declare(
        glm
        DOWNLOAD_EXTRACT_TIMESTAMP OFF
        URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.zip
    )

    FetchContent_MakeAvailable(glm)
endif()

if (TARGET glm)
    Message("GLM target 'glm' found.")
    Set_Target_Properties(glm PROPERTIES FOLDER "Dependencies")
elseif (TARGET glm::glm)
    Message("GLM target 'glm::glm' found.")
    # If glm::glm exists but not glm, we can create an alias or just use glm::glm
    # But since the code uses target_link_libraries(Core glm), we might need the 'glm' target.
    Add_Library(glm ALIAS glm::glm)
endif()
# -------------------------------------------------

# -------------------------------------
# Build the Vulkyrie Engine.
# -------------------------------------
Add_Subdirectory(engine)

# -------------------------------------
# Build the Vulkyrie CLI tool.
# -------------------------------------
# Add_Subdirectory(vulky-cli)

# -------------------------------------
# Games based on the Vulkyrie engine
# -------------------------------------
# Add_Subdirectory(games/asteroids)
Add_Subdirectory(games/pong)
