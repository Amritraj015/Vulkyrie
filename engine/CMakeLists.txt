Cmake_Minimum_Required(VERSION 3.27)
Project(engine)
Set(CMAKE_CXX_STANDARD 23)

File(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS src/*.cpp)
File(GLOB_RECURSE ENGINE_HEADERS CONFIGURE_DEPENDS include/*.h)

Add_Library(${PROJECT_NAME} STATIC ${ENGINE_HEADERS} ${ENGINE_SOURCES})

Target_Include_Directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Target_Link_Libraries(${PROJECT_NAME} PUBLIC glm PRIVATE glfw glad)
Target_Link_Libraries(${PROJECT_NAME} PUBLIC glm glfw glad assimp)

Target_Compile_Definitions(${PROJECT_NAME}
    PRIVATE 
        VULKYRIE_EXPORTS
        VULKYRIE_DEBUG
)

Target_Precompile_Headers(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/vlkypch.h)
