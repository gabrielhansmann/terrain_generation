#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(GLFWwindow* window)
    : m_window(window),
      m_position(0.0f, 0.8f, 1.5f),
      m_front(0.0f, 0.0f, -1.0f),
      m_up(0.0f, 1.0f, 0.0f),
      m_right(1.0f, 0.0f, 0.0f),
      m_worldUp(0.0f, 1.0f, 0.0f),
      m_yaw(-90.0f),
      m_pitch(0.0f),
      m_speed(3.0f),
      m_sensitivity(0.12f),
      m_fov(45.0f),
      m_lastX(0.0),
      m_lastY(0.0),
      m_firstMouse(true),
            m_controlsEnabled(true),
            m_toggleKeyWasDown(false),
      m_width(800),
      m_height(600) {
    if (m_window) {
        glfwGetFramebufferSize(m_window, &m_width, &m_height);
        m_lastX = m_width * 0.5;
        m_lastY = m_height * 0.5;
        updateVectors();
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
}

Camera::~Camera() {
    if (m_window) {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void Camera::updateVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));

    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

void Camera::update(float deltaTime) {
    if (!m_window) return;

    bool toggleKeyDown = glfwGetKey(m_window, GLFW_KEY_C) == GLFW_PRESS;
    if (toggleKeyDown && !m_toggleKeyWasDown) {
        setControlsEnabled(!m_controlsEnabled);
    }
    m_toggleKeyWasDown = toggleKeyDown;

    if (m_controlsEnabled) {
        float velocity = m_speed * deltaTime;
        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) m_position += m_front * velocity;
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) m_position -= m_front * velocity;
        if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) m_position -= m_right * velocity;
        if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) m_position += m_right * velocity;
        if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) m_position += m_worldUp * velocity;
        if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) m_position -= m_worldUp * velocity;
    }

    double xpos = 0.0;
    double ypos = 0.0;
    glfwGetCursorPos(m_window, &xpos, &ypos);
    if (m_firstMouse) {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - m_lastX);
    float yoffset = static_cast<float>(m_lastY - ypos);
    m_lastX = xpos;
    m_lastY = ypos;

    if (!m_controlsEnabled) {
        glfwGetFramebufferSize(m_window, &m_width, &m_height);
        return;
    }

    xoffset *= m_sensitivity;
    yoffset *= m_sensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;
    if (m_pitch > 89.0f) m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;

    updateVectors();
    glfwGetFramebufferSize(m_window, &m_width, &m_height);
}

glm::vec3 Camera::position() const {
    return m_position;
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::projectionMatrix() const {
    float aspect = m_width > 0 && m_height > 0 ? static_cast<float>(m_width) / static_cast<float>(m_height) : 4.0f / 3.0f;
    return glm::perspective(glm::radians(m_fov), aspect, 0.1f, 100.0f);
}

bool Camera::controlsEnabled() const {
    return m_controlsEnabled;
}

void Camera::setControlsEnabled(bool enabled) {
    if (m_controlsEnabled == enabled) return;

    m_controlsEnabled = enabled;
    m_firstMouse = true;

    if (m_window) {
        glfwSetInputMode(m_window, GLFW_CURSOR,
                         m_controlsEnabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}
