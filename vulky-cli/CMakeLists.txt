cmake_minimum_required(VERSION 3.27)
project(vulky)
set(CMAKE_CXX_STANDARD 23)

File(GLOB_RECURSE VULKY_CLI_SOURCES CONFIGURE_DEPENDS src/*.cpp)

Add_Executable(${PROJECT_NAME})

Target_Sources(${PROJECT_NAME} PRIVATE ${VULKY_CLI_SOURCES})
Target_Link_Libraries(${PROJECT_NAME} PRIVATE engine)

Target_Compile_Definitions(${PROJECT_NAME}
    PRIVATE 
        VULKYRIE_EXPORTS
        VULKYRIE_DEBUG
)
