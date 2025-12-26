cmake_minimum_required(VERSION 3.27)
project(pong)
set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE PONG_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE ${PONG_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE engine)

target_compile_definitions(${PROJECT_NAME} 
    PRIVATE 
        VULKYRIE_EXPORTS
        VULKYRIE_DEBUG
)
