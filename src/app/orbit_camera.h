#pragma once
#include "glm/fwd.hpp"
#include "ui.h"
#include <glm/glm.hpp>

struct ShaderSettings;

class OrbitCamera {
	public:
		void update(float time, float mouseX, float mouseY, bool mouseDown, 
		int screenW, int screenH, const ShaderSettings& settings);
		
		glm::mat4 viewMatrix() const { return m_view; }
		glm::mat4 projMatrix() const { return m_proj; }
		glm::mat4 viewProjMatrix() const { return m_proj * m_view; }
		glm::vec3 position() const { return m_position; }
		void zoom(float scrollTicks);
	private:
		float m_azimuth = 0.0f;
        float m_elevation = -0.3f;
        float m_distance = 3.0f;
        bool m_dragging = false;
        float m_lastMouseX = 0.0f;
        float m_lastMouseY = 0.0f;
		
		glm::mat4 m_view;
		glm::mat4 m_proj;
		glm::vec3 m_position;
};
