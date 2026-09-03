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
target_compile_features(engine PUBLIC cxx_std_26)

# ---------------------------
# Engine Version Constants (Generated)
# ---------------------------
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/vulkyrie_version.h
    @ONLY
)

# ---------------------------
# Include Paths
# ---------------------------
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/generated
        # ${Stb_INCLUDE_DIR}
        ${STB_INCLUDE_DIRS} # Defined in Dependencies.cmake in the root directory.
)

# ---------------------------
# Dependencies
# ---------------------------
target_link_libraries(engine
    PUBLIC
        Threads::Threads
        glfw
        glad::glad
        volk::volk
        volk::volk_headers
        GPUOpen::VulkanMemoryAllocator
        unofficial::spirv-reflect
        glm::glm
        assimp::assimp
        OpenAL::OpenAL
        # slang::slang
)

# ---------------------------
# Compile Definitions (Propagate to all dependents)
# ---------------------------
target_compile_definitions(engine
    PUBLIC
        VE_EXPORTS
        $<$<CONFIG:Debug>:VE_DEBUG>
        VE_LOG_LEVEL=${VE_LOG_LEVEL}
        VE_VK_ENABLE_VALIDATION
        VK_NO_PROTOTYPES
        GLM_ENABLE_EXPERIMENTAL
        GLFW_INCLUDE_NONE
)

# ---------------------------
# Precompiled Headers
# ---------------------------
target_precompile_headers(engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/vlkypch.h
)

# ---------------------------
# Compile shaders, use ${SLANG_EXECUTABLE}
# ---------------------------

# ---------------------------
# Warnings & Errors (Propagate)
# ---------------------------
if (MSVC)
    target_compile_options(engine
       PRIVATE 
            /W4
            /WX
       PUBLIC
            /Zc:preprocessor
            /EHsc
    )
else()
    target_compile_options(engine
        PRIVATE
            -Wall
            -Wextra
            -Wconversion 
            -Wsign-conversion 
            -Wpedantic
            -Werror
    )
endif()
