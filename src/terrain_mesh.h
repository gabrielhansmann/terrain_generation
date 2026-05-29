#pragma once
#include <glad/glad.h>

class TerrainMesh {
	public:
		TerrainMesh(int resolution); // = quads per side
		~TerrainMesh();
		void draw() const;
	private:
		GLuint m_vao, m_vbo, m_ebo;
		int m_indexCount;
};
