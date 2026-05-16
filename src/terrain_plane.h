#ifndef TERRAIN_PLANE_H
#define TERRAIN_PLANE_H

#include <glad/glad.h>
#include <glm/glm.hpp>

// Initialize GPU resources for the plane (VAO/VBO/EBO)
void Plane_Init();

// Render the plane. The function will set the model matrix internally
// and use the provided view/projection matrices and camera position.
void Plane_Render(GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos);

// Cleanup GPU resources used by the plane
void Plane_Cleanup();

#endif