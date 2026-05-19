#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// Initialize camera with the GLFW window (required for input)
void Camera_Init(GLFWwindow* window);

// Per-frame update (call with delta time seconds)
// Controls: WASD for movement, Space/Shift for up/down, mouse for looking around
void Camera_Update(float deltaTime);

// Get the current camera position
glm::vec3 Camera_GetPosition();

// Get the current camera view matrix
glm::mat4 Camera_GetViewMatrix();

// Get the current camera projection matrix
glm::mat4 Camera_GetProjectionMatrix();

// Cleanup camera resources
void Camera_Cleanup();