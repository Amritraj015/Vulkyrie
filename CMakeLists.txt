# -------------------------------------
# Global configuration
# -------------------------------------
cmake_minimum_required(VERSION 3.27)
project(vulkyrie)
set(CMAKE_CXX_STANDARD 23)

# -------------------------------------
# Options
# -------------------------------------
option(VULKYRIE_BUILD_EXAMPLES "Build Vulkyrie examples" ON)
option(VULKYRIE_BUILD_CLI "Build vulky-cli" ON)
option(VULKYRIE_BUILD_EDITOR "Build Vulkyrie Editor" ON)
option(VULKYRIE_BUILD_TESTS "Build Vulkyrie tests" ON)
option(VULKYRIE_EXPORT_COMPILE_COMMANDS "Export compile_commands.json" ON)

# -------------------------------------
# Export compile commands
# -------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ${VULKYRIE_EXPORT_COMPILE_COMMANDS})

if(VULKYRIE_EXPORT_COMPILE_COMMANDS)
    add_custom_target(export-compile-commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
        COMMENT "Exporting compile_commands.json to source directory"
    )
endif()

# -------------------------------------
# Include dependencies configuration
# -------------------------------------
include(Dependencies.cmake)

# -------------------------------------
# Build the Vulkyrie Engine.
# -------------------------------------
add_subdirectory(engine)

# -------------------------------------
# Build the Vulkyrie CLI tool.
# -------------------------------------
if(VULKYRIE_BUILD_CLI)
    add_subdirectory(vulky-cli)
endif()

# -------------------------------------
# Examples/Games based on the Vulkyrie engine
# -------------------------------------
if(VULKYRIE_BUILD_EXAMPLES)
    add_subdirectory(examples/asteroids)
    add_subdirectory(examples/sandbox)
endif()

# -------------------------------------
# Build Editor 
# -------------------------------------
if(VULKYRIE_BUILD_EDITOR)
    add_subdirectory(editor)
endif()

# -------------------------------------
# Build tests
# -------------------------------------
if(VULKYRIE_BUILD_TESTS)
    add_subdirectory(tests)
endif()

