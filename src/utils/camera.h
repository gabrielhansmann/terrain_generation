#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Camera {
public:
	explicit Camera(GLFWwindow* window);
	~Camera();

	Camera(const Camera&) = delete;
	Camera& operator=(const Camera&) = delete;

	void update(float deltaTime);
	void setControlsEnabled(bool enabled);

	glm::vec3 position() const;
	glm::mat4 viewMatrix() const;
	glm::mat4 projectionMatrix() const;
	bool controlsEnabled() const;

private:
	void updateVectors();

	GLFWwindow* m_window = nullptr;
	glm::vec3 m_position;
	glm::vec3 m_front;
	glm::vec3 m_up;
	glm::vec3 m_right;
	glm::vec3 m_worldUp;
	float m_yaw;
	float m_pitch;
	float m_speed;
	float m_sensitivity;
	float m_fov;
	double m_lastX;
	double m_lastY;
	bool m_firstMouse;
	bool m_controlsEnabled;
	bool m_toggleKeyWasDown;
	int m_width;
	int m_height;
};