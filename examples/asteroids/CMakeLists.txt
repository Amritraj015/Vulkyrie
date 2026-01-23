Cmake_Minimum_Required(VERSION 3.31)
Project(asteroids)
Set(CMAKE_CXX_STANDARD 23)

File(GLOB_RECURSE ASTEROIDS_SOURCES CONFIGURE_DEPENDS src/*.cpp)

Add_Executable(${PROJECT_NAME})

Target_Sources(${PROJECT_NAME} PRIVATE ${ASTEROIDS_SOURCES})
Target_Link_Libraries(${PROJECT_NAME} PRIVATE engine)

Target_Compile_Definitions(${PROJECT_NAME}
    PRIVATE 
        VULKYRIE_EXPORTS
        VULKYRIE_DEBUG
)
