cmake_minimum_required(VERSION 3.27)
project(asteroids)
set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE ASTEROIDS_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE ${ASTEROIDS_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE engine)