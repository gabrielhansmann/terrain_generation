#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class TerrainPlane {
public:
	TerrainPlane(const char* vertexShaderPath, const char* fragmentShaderPath);
	~TerrainPlane();

	TerrainPlane(const TerrainPlane&) = delete;
	TerrainPlane& operator=(const TerrainPlane&) = delete;

	void render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos);

private:
	GLuint m_vao = 0;
	GLuint m_vbo = 0;
	GLuint m_ebo = 0;
	GLuint m_shaderProgram = 0;
};