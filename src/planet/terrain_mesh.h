#pragma once
#include <glad/glad.h>

// Flat XZ grid in [-0.5, 0.5]^2 with vec2 vertex positions (location 0).
class TerrainMesh {
	public:
		TerrainMesh(int resolution); // quads per side
		~TerrainMesh();
		void draw() const;
	private:
		GLuint m_vao, m_vbo, m_ebo;
		int m_indexCount;
};
