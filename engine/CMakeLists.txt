# ---------------------------
# Sources
# ---------------------------
file(GLOB_RECURSE ENGINE_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_library(engine STATIC ${ENGINE_SOURCES})

# ---------------------------
# C++ Standard
# ---------------------------
target_compile_features(engine PUBLIC cxx_std_23)

# ---------------------------
# Include Paths
# ---------------------------
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------
# Dependencies
# ---------------------------
target_link_libraries(engine
    PUBLIC
        glm
        glfw
        glad
        assimp
)

# ---------------------------
# Compile Definitions (Propagate to all dependents)
# ---------------------------
target_compile_definitions(engine
    PUBLIC
        VULKYRIE_EXPORTS
        $<$<CONFIG:Debug>:VULKYRIE_DEBUG>
        VULKYRIE_LOG_LEVEL=${VULKYRIE_LOG_LEVEL}
)

# ---------------------------
# Precompiled Headers
# ---------------------------
target_precompile_headers(engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/vlkypch.h
)

# ---------------------------
# Warnings & Errors (Propagate)
# ---------------------------
if (MSVC)
    target_compile_options(engine
       PRIVATE 
            /W4
            /WX
    )
else()
    target_compile_options(engine
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Werror
            # $<$<CXX_COMPILER_ID:GNU,Clang>:-fvisibility=hidden>
            # $<$<CXX_COMPILER_ID:GNU,Clang>:-fvisibility-inlines-hidden>
    )
endif()
