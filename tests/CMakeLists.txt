Cmake_Minimum_Required(VERSION 3.31)
Set(CMAKE_CXX_STANDARD 23)

Include(FetchContent)
Project(tests)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.12.0
)

FetchContent_MakeAvailable(Catch2)

File(GLOB_RECURSE VULKYRIE_TEST_SOURCES CONFIGURE_DEPENDS src/*.cpp)

Add_Executable(${PROJECT_NAME})

Target_Sources(${PROJECT_NAME} PRIVATE ${VULKYRIE_TEST_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE Catch2::Catch2WithMain engine)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(${PROJECT_NAME})

# Target_Compile_Definitions(${PROJECT_NAME}
#     PRIVATE 
#         VULKYRIE_EXPORTS
#         VULKYRIE_DEBUG
# )

Add_Custom_Target(copy_test_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

Add_Dependencies(${PROJECT_NAME} copy_test_assets)
