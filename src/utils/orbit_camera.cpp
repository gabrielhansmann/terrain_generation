#include "orbit_camera.h"
#include "../ui.h"
#include "glm/common.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

void OrbitCamera::update(float time, float mouseX, float mouseY, bool mouseDown,
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
	
	// Mirrors CameraRotation(vec2(elevation, azimuth)) in common.glsl
	float cosElevation = cosf(elevation), sinElevation = sinf(elevation);
	float cosAzimuth = cosf(azimuth), sinAzimuth = sinf(azimuth);
	// matches the GLSL collumns exactly
	glm::mat3 cameraRotation(
		glm::vec3(cosAzimuth,				0.0f ,         -sinAzimuth),
		glm::vec3(sinAzimuth * sinElevation, cosElevation,  cosAzimuth * sinElevation),
		glm::vec3(sinAzimuth * cosElevation, -sinElevation, cosAzimuth * cosElevation)
	);

	glm::vec3 lookAtTarget(s.cameraLookAt[0], s.cameraLookAt[1], s.cameraLookAt[2]);
	m_position = lookAtTarget + cameraRotation * glm::vec3(0.0f, 0.0f, cameraDistance);
	glm::vec3 cameraUp = cameraRotation * glm::vec3(0.0f, 1.0f, 0.0f);

	float aspect = screenH > 0 ? (float) screenW / (float) screenH : 1.0f;
	m_view = glm::lookAt(m_position, lookAtTarget, cameraUp);
	m_proj = glm::perspective(glm::radians(fov), aspect, 0.01f, 10.0f);
}
