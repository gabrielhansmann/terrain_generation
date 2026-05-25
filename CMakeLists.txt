cmake_minimum_required(VERSION 3.10)
project(terrain_generation)

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
# ImGui
add_library(imgui STATIC
    ext/imgui/imgui.cpp
    ext/imgui/imgui_draw.cpp
    ext/imgui/imgui_widgets.cpp
    ext/imgui/imgui_tables.cpp
    ext/imgui/imgui_demo.cpp
    ext/imgui/backends/imgui_impl_glfw.cpp
    ext/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC 
    ext/imgui 
    ext/imgui/backends
    ext/glad/include
)
target_link_libraries(imgui PRIVATE glfw)

# Main Executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/utils/shaders.cpp
    src/perlin_noise.cpp
	src/compute_pass.cpp
	src/framebuffer.cpp
    src/ui.cpp
)

# Linking and Includes
target_link_libraries(${PROJECT_NAME} PRIVATE glad glfw glm imgui dl pthread)
target_include_directories(${PROJECT_NAME} PRIVATE 
    src 
    ext/glad/include 
    ext/glm
    ext/imgui
    ext/imgui/backends
)

# Asset Handling
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
