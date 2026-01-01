Cmake_Minimum_Required(VERSION 3.27)
Project(pong)
Set(CMAKE_CXX_STANDARD 23)

File(GLOB_RECURSE PONG_SOURCES CONFIGURE_DEPENDS src/*.cpp)

Add_Executable(${PROJECT_NAME})

Target_Sources(${PROJECT_NAME} PRIVATE ${PONG_SOURCES})
Target_Link_Libraries(${PROJECT_NAME} PRIVATE engine)

Target_Compile_Definitions(${PROJECT_NAME}
    PRIVATE 
        VULKYRIE_EXPORTS
        VULKYRIE_DEBUG
)

Add_Custom_Target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

Add_Dependencies(${PROJECT_NAME} copy_assets)
