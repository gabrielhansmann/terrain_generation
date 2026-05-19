#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

// Initialize GPU resources for the plane (VAO/VBO/EBO)
// Provide vertex and fragment shader file paths (relative to the running binary)
void Plane_Init(const char* vertexShaderPath, const char* fragmentShaderPath);

// Render the plane. The function will set the model matrix internally
// and use the provided view/projection matrices and camera position.
// The plane module now owns its shader program internally; callers
// do not need to provide a program.
void Plane_Render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos);

// Cleanup GPU resources used by the plane
void Plane_Cleanup();