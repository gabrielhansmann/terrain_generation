#include "orbit_camera.h"
#include "ui.h"
#include "glm/common.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

void OrbitCamera::update(float time, float mouseX, float mouseY, bool mouseDown,
                          int screenW, int screenH, const ShaderSettings& s) {
	const float sensitivity = 0.005f; // radians of orbit per pixel dragged
	const float maxElevation = 1.5f; // stop short of the poles so "up" never flips

	if (mouseDown) {
		//anchor to the cursor on first drag frame so the first delta is zero
		if (!m_dragging) {
			m_lastMouseX = mouseX;
			m_lastMouseY = mouseY;
			m_dragging = true;
		}
		m_azimuth -= (mouseX - m_lastMouseX) * sensitivity;
		m_elevation -= (mouseY - m_lastMouseY) * sensitivity;
		m_elevation = glm::clamp(m_elevation, -maxElevation, maxElevation);
		m_lastMouseX = mouseX;
		m_lastMouseY = mouseY;
	} else {
		m_dragging = false;
	}

	float cosElevation = cosf(m_elevation), sinElevation = sinf(m_elevation);
    float cosAzimuth = cosf(m_azimuth), sinAzimuth = sinf(m_azimuth);
    // same column layout as the old camera, kept so the orbit direction matches
    // what the rest of the project already expects
	glm::mat3 rotation(
		glm::vec3(cosAzimuth,				0.0f ,         -sinAzimuth),
		glm::vec3(sinAzimuth * sinElevation, cosElevation,  cosAzimuth * sinElevation),
		glm::vec3(sinAzimuth * cosElevation, -sinElevation, cosAzimuth * cosElevation)
	);

	// planet sits at origin, so orbit around that
	glm::vec3 target(0.0f);
	m_position = target + rotation * glm::vec3(0.0f, 0.0f, m_distance);
	glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);

	float fov = 45.0f; // wide enough to frame the whole radius-1 planet at distance 3 -> expose to options later!
	float aspect = screenH > 0 ? (float)screenW / (float)screenH : 1.0f;
	m_view = glm::lookAt(m_position, target , up);
	m_proj = glm::perspective(glm::radians(fov), aspect, 0.01f, 10.0f);
}
