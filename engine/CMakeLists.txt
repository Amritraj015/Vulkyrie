cmake_minimum_required(VERSION 3.27)
project(engine)
set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE ENGINE_HEADERS CONFIGURE_DEPENDS include/*.h)

add_library(${PROJECT_NAME} ${ENGINE_HEADERS} ${ENGINE_SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC glfw PRIVATE glad)

target_compile_definitions(${PROJECT_NAME} 
    PRIVATE 
        VULKYRIE_EXPORTS
        VULKYRIE_DEBUG
)
