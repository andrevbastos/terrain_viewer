cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)

project(terrain_viewer VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(FetchContent)

find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

FetchContent_Declare(
    ifcg
    GIT_REPOSITORY https://github.com/andrevbastos/ifcg.git
    GIT_TAG main
)

FetchContent_MakeAvailable(ifcg)

if(TARGET ifcg_core)
    target_compile_options(ifcg_core PRIVATE -Wno-error)
endif()

add_executable(terrain_viewer
    src/terrain_viewer.cpp
    src/stb_impl.cpp
)

target_link_libraries(terrain_viewer PRIVATE ifcg_core)
target_include_directories(terrain_viewer PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_custom_command(TARGET terrain_viewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${ifcg_SOURCE_DIR}/resources $<TARGET_FILE_DIR:terrain_viewer>/resources
)
