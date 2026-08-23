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
# Include Paths
# ---------------------------
target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
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
)

# ---------------------------
# Compile Definitions (Propagate to all dependents)
# ---------------------------
target_compile_definitions(engine
    PUBLIC
        VULKYRIE_EXPORTS
        $<$<CONFIG:Debug>:VULKYRIE_DEBUG>
        VULKYRIE_LOG_LEVEL=${VULKYRIE_LOG_LEVEL}
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
# Shader compilation
# ---------------------------
# set(VULKYRIE_SHADER_OUT "${CMAKE_BINARY_DIR}/shaders")
# file(MAKE_DIRECTORY "${VULKYRIE_SHADER_OUT}")
#
# function(vulkyrie_add_shader target src stage entry)
#     get_filename_component(name "${src}" NAME_WE)
#     set(out "${VULKYRIE_SHADER_OUT}/${name}.${stage}.spv")
#     add_custom_command(
#         OUTPUT  "${out}"
#         COMMAND "${DIRECTX_DXC_TOOL}" -spirv -T ${stage} -E ${entry}
#                 -fspv-target-env=vulkan1.3
#                 "${CMAKE_CURRENT_SOURCE_DIR}/${src}" -Fo "${out}"
#         DEPENDS "${src}"
#         COMMENT "DXC ${src}"
#         VERBATIM)
#     target_sources(${target} PRIVATE "${out}")
# endfunction()

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
