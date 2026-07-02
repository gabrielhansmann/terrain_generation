#include "camera.h"
#include "app/ui.h"
#include "glm/common.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

void OrbitCamera::updateFlat(float time, float mouseX, float mouseY, bool mouseDown,
	int screenW, int screenH, const ShaderSettings& s) {
	const float PI = glm::pi<float>();

	bool isLowAngle = s.useAutoLowAngle ? (fmod(time, 240.0f) >= 120.0f) : s.lowAngle;

	float cameraDistance = isLowAngle ? s.cameraDistLow : s.cameraDistHigh;
	float fov = isLowAngle ? s.cameraFovLow : s.cameraFovHigh;
	float baseAzimuth = isLowAngle ? s.cameraAngleLow : s.cameraAngleHigh;
	float baseElevation = isLowAngle ? s.cameraElevationLow : s.cameraElevationHigh;
	float spinRate = isLowAngle ? s.timeCamSpinLow : s.timeCamSpinHigh;

	float timeRevolutions = time * 2.0f * PI;

	float azimuth = timeRevolutions * spinRate
		+ sinf(timeRevolutions / 6.0f) * s.timeCamWobble * 6.0f
		+ baseAzimuth * 2.0f * PI;

	float elevation = baseElevation;

	if (s.cameraMouseControl && mouseDown) {
		float mouseNormX = mouseX / (float)screenW;
		float mouseNormY = glm::clamp(
			glm::mix((float)(screenH - mouseY) / (float)screenH, 0.5f, -1.0f),
			0.0f, 1.0f
		);
		azimuth = (mouseNormX - 0.5f) * (-PI * 2.0f);
		elevation = (mouseNormY - 1.0f) * (PI * 0.5f);
	}

	float cosElevation = cosf(elevation), sinElevation = sinf(elevation);
	float cosAzimuth = cosf(azimuth), sinAzimuth = sinf(azimuth);
	glm::mat3 cameraRotation(
		glm::vec3(cosAzimuth, 0.0f, -sinAzimuth),
		glm::vec3(sinAzimuth * sinElevation, cosElevation, cosAzimuth * sinElevation),
		glm::vec3(sinAzimuth * cosElevation, -sinElevation, cosAzimuth * cosElevation)
	);

	glm::vec3 lookAtTarget(s.cameraLookAt[0], s.cameraLookAt[1], s.cameraLookAt[2]);
	m_position = lookAtTarget + cameraRotation * glm::vec3(0.0f, 0.0f, cameraDistance);
	glm::vec3 cameraUp = cameraRotation * glm::vec3(0.0f, 1.0f, 0.0f);

	float aspect = screenH > 0 ? (float)screenW / (float)screenH : 1.0f;
	m_view = glm::lookAt(m_position, lookAtTarget, cameraUp);
	m_proj = glm::perspective(glm::radians(fov), aspect, 0.01f, 10.0f);
}

void OrbitCamera::updateRunner(float dt, bool forward, bool back, bool left, bool right,
	bool pitchUp, bool pitchDown, bool yawLeft, bool yawRight,
	int screenW, int screenH) {
	const float moveSpeed = 0.30f;  // world units / second (terrain spans ~1 unit)
	const float rotSpeed  = 1.3f;   // radians / second
	const float fov       = 65.0f;
	const float eyeHeight  = 0.6f; // low, skimming just over the terrain (~0.5 surface)
	const float backOffset = 0.45f; // eye sits behind the patch centre, looking in
	const float maxPitch   = 0.35f;
	const float minPitch   = -1.40f;

	// Rotation: J/L yaw, I/K pitch.
	m_runnerYaw   += ((yawLeft ? 1.0f : 0.0f) - (yawRight ? 1.0f : 0.0f)) * rotSpeed * dt;
	m_runnerPitch += ((pitchUp ? 1.0f : 0.0f) - (pitchDown ? 1.0f : 0.0f)) * rotSpeed * dt;
	m_runnerPitch = glm::clamp(m_runnerPitch, minPitch, maxPitch);

	float sinYaw = sinf(m_runnerYaw), cosYaw = cosf(m_runnerYaw);

	// Horizontal heading basis (on the XZ plane).
	glm::vec2 forwardXZ(sinYaw, cosYaw);
	glm::vec2 rightXZ(cosYaw, -sinYaw);

	// WASD accumulates into the world-space noise offset rather than moving the
	// eye, so the terrain scrolls forever with no floating point drift.
	glm::vec2 move = forwardXZ * ((forward ? 1.0f : 0.0f) - (back ? 1.0f : 0.0f))
		+ rightXZ * ((right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f));
	if (glm::dot(move, move) > 0.0f)
		m_runnerOffset += glm::normalize(move) * moveSpeed * dt;

	// Eye stays near the rear of the rendered patch so the terrain fills the view
	// ahead. The patch lives in local space [-0.5, 0.5]^2; the scroll is in the
	// heightmap only.
	float cosPitch = cosf(m_runnerPitch), sinPitch = sinf(m_runnerPitch);
	m_position = glm::vec3(-forwardXZ.x * backOffset, eyeHeight, -forwardXZ.y * backOffset);
	glm::vec3 dir(sinYaw * cosPitch, sinPitch, cosYaw * cosPitch);
	glm::vec3 target = m_position + dir;

	float aspect = screenH > 0 ? (float)screenW / (float)screenH : 1.0f;
	m_view = glm::lookAt(m_position, target, glm::vec3(0.0f, 1.0f, 0.0f));
	m_proj = glm::perspective(glm::radians(fov), aspect, 0.005f, 10.0f);
}

void OrbitCamera::zoom(float scrollTicks) {
	const float zoomPerTick = 0.9f;
	const float minDistance = 1.07f;
	const float maxDistance = 8.0f;
	m_distance *= powf(zoomPerTick, scrollTicks);
	m_distance = glm::clamp(m_distance, minDistance, maxDistance);
}
void OrbitCamera::update(float time, float mouseX, float mouseY, bool mouseDown,
                          int screenW, int screenH) {
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

	float fov = 60.0f; // wide enough to frame the whole radius-1 planet at distance 3 -> expose to options later!
	float aspect = screenH > 0 ? (float)screenW / (float)screenH : 1.0f;
	m_view = glm::lookAt(m_position, target , up);
	m_proj = glm::perspective(glm::radians(fov), aspect, 0.01f, 10.0f);
}
