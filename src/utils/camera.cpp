#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

static GLFWwindow* s_window = nullptr;

static glm::vec3 s_Position(0.0f, 0.8f, 1.5f);
static glm::vec3 s_Front(0.0f, 0.0f, -1.0f);
static glm::vec3 s_Up(0.0f, 1.0f, 0.0f);
static glm::vec3 s_Right(1.0f, 0.0f, 0.0f);
static glm::vec3 s_WorldUp(0.0f, 1.0f, 0.0f);

static float s_Yaw = -90.0f;
static float s_Pitch = 0.0f;
static float s_Speed = 3.0f;
static float s_Sensitivity = 0.12f;
static float s_Fov = 45.0f;

static double s_LastX = 0.0, s_LastY = 0.0;
static bool s_FirstMouse = true;

static int s_Width = 800, s_Height = 600;

void updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(s_Yaw)) * cos(glm::radians(s_Pitch));
    front.y = sin(glm::radians(s_Pitch));
    front.z = sin(glm::radians(s_Yaw)) * cos(glm::radians(s_Pitch));
    s_Front = glm::normalize(front);
    s_Right = glm::normalize(glm::cross(s_Front, s_WorldUp));
    s_Up = glm::normalize(glm::cross(s_Right, s_Front));
}

void Camera_Init(GLFWwindow* window) {
    s_window = window;
    if (!s_window) return;
    glfwGetFramebufferSize(s_window, &s_Width, &s_Height);
    s_LastX = s_Width * 0.5;
    s_LastY = s_Height * 0.5;
    s_FirstMouse = true;
    updateCameraVectors();
    // Capture the cursor for rotation
    glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Camera_Update(float deltaTime) {
    if (!s_window) return;

    // Keyboard movement
    float velocity = s_Speed * deltaTime;
    if (glfwGetKey(s_window, GLFW_KEY_W) == GLFW_PRESS)
        s_Position += s_Front * velocity;
    if (glfwGetKey(s_window, GLFW_KEY_S) == GLFW_PRESS)
        s_Position -= s_Front * velocity;
    if (glfwGetKey(s_window, GLFW_KEY_A) == GLFW_PRESS)
        s_Position -= s_Right * velocity;
    if (glfwGetKey(s_window, GLFW_KEY_D) == GLFW_PRESS)
        s_Position += s_Right * velocity;
    if (glfwGetKey(s_window, GLFW_KEY_SPACE) == GLFW_PRESS)
        s_Position += s_WorldUp * velocity;
    if (glfwGetKey(s_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        s_Position -= s_WorldUp * velocity;

    // Mouse rotation
    double xpos, ypos;
    glfwGetCursorPos(s_window, &xpos, &ypos);
    if (s_FirstMouse) {
        s_LastX = xpos;
        s_LastY = ypos;
        s_FirstMouse = false;
    }
    float xoffset = float(xpos - s_LastX);
    float yoffset = float(s_LastY - ypos); // reversed: y ranges bottom->top
    s_LastX = xpos;
    s_LastY = ypos;

    xoffset *= s_Sensitivity;
    yoffset *= s_Sensitivity;

    s_Yaw += xoffset;
    s_Pitch += yoffset;
    if (s_Pitch > 89.0f) s_Pitch = 89.0f;
    if (s_Pitch < -89.0f) s_Pitch = -89.0f;
    updateCameraVectors();

    // keep projection updated in case of resize
    glfwGetFramebufferSize(s_window, &s_Width, &s_Height);
}

glm::vec3 Camera_GetPosition() { return s_Position; }

glm::mat4 Camera_GetViewMatrix() {
    return glm::lookAt(s_Position, s_Position + s_Front, s_Up);
}

glm::mat4 Camera_GetProjectionMatrix() {
    float aspect = s_Width > 0 && s_Height > 0 ? (float)s_Width / (float)s_Height : 4.0f/3.0f;
    return glm::perspective(glm::radians(s_Fov), aspect, 0.1f, 100.0f);
}

void Camera_Cleanup() {
    if (s_window) glfwSetInputMode(s_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    s_window = nullptr;
}
