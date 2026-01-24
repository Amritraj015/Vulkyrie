# -------------------------------------
# Global configuration
# -------------------------------------
Cmake_Minimum_Required(VERSION 3.27)
Project(vulkyrie)
Set(CMAKE_CXX_STANDARD 23)

# -------------------------------------
# Options
# -------------------------------------
Option(VULKYRIE_BUILD_EXAMPLES "Build Vulkyrie examples" ON)
Option(VULKYRIE_BUILD_CLI "Build vulky-cli" ON)
Option(VULKYRIE_BUILD_TESTS "Build Vulkyrie tests" ON)
Option(VULKYRIE_EXPORT_COMPILE_COMMANDS "Export compile_commands.json" ON)

# -------------------------------------
# Export compile commands
# -------------------------------------
if (VULKYRIE_EXPORT_COMPILE_COMMANDS)
    Set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# -------------------------------------
# Include dependencies configuration
# -------------------------------------
Include(Dependencies.cmake)

# -------------------------------------
# Build the Vulkyrie Engine.
# -------------------------------------
Add_Subdirectory(engine)

# -------------------------------------
# Build the Vulkyrie CLI tool.
# -------------------------------------
if (VULKYRIE_BUILD_CLI)
    Add_Subdirectory(vulky-cli)
endif()

# -------------------------------------
# Examples/Games based on the Vulkyrie engine
# -------------------------------------
if (VULKYRIE_BUILD_EXAMPLES)
    Add_Subdirectory(examples/asteroids)
    Add_Subdirectory(examples/sandbox)
endif()

# -------------------------------------
# Build tests
# -------------------------------------
if (VULKYRIE_BUILD_TESTS)
    Add_Subdirectory(tests)
endif()
