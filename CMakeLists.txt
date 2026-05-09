cmake_minimum_required(VERSION 3.10)
project(TerrainGeneration)

# Project Standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(ext/glfw)
# GLAD
add_library(glad ext/glad/src/glad.c)
target_include_directories(glad PUBLIC ext/glad/include)
# GLM
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ext/glm)

# Main Executable
add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/utils/shaders.cpp
)

# Linking and Includes
target_link_libraries(${PROJECT_NAME} PRIVATE glad glfw glm dl)
target_include_directories(${PROJECT_NAME} PRIVATE 
    src 
    ext/glad/include 
    ext/glm
)

# Asset Handling
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})