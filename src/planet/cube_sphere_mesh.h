#pragma once
#include <glad/glad.h>

class CubeSphereMesh {
	public:
		CubeSphereMesh(int resolution); // = quads per face side
		~CubeSphereMesh();
		void draw() const;
	private:
		GLuint m_vao, m_vbo, m_ebo;
		int m_indexCount;
};
