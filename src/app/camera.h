#pragma once
#include "glm/fwd.hpp"
#include <glm/glm.hpp>

struct ShaderSettings;

class OrbitCamera {
	public:
		void update(float time, float mouseX, float mouseY, bool mouseDown, 
		int screenW, int screenH);
		// Shadertoy-style animated camera used by the flat-plane path (pre cube-sphere).
		void updateFlat(float time, float mouseX, float mouseY, bool mouseDown,
			int screenW, int screenH, const ShaderSettings& settings);
		// Free first-person camera for the flat endless-runner. WASD translates and
		// IJKL rotates. Translation never moves the eye in world space; it
		// accumulates into a world-space noise offset (runnerOffset) so the terrain
		// scrolls underneath while the rendered patch and eye stay put (treadmill).
		void updateRunner(float dt, bool forward, bool back, bool left, bool right,
			bool pitchUp, bool pitchDown, bool yawLeft, bool yawRight,
			int screenW, int screenH);
		// World-space offset to feed the flat erosion/detail compute passes.
		glm::vec2 runnerOffset() const { return m_runnerOffset; }

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
		
		// Endless-runner state
		glm::vec2 m_runnerOffset = glm::vec2(0.0f); // accumulated world-space scroll
		float m_runnerYaw = 0.0f;     // heading, radians (0 looks toward +Z)
		float m_runnerPitch = -0.35f; // tilt, radians (negative looks down)

		glm::mat4 m_view;
		glm::mat4 m_proj;
		glm::vec3 m_position;
};
