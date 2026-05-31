#include "cube_sphere_mesh.h"
#include <vector>
#include <glm/glm.hpp>

// copy of cubeFaceDir from shaders/cube_face.glsl. Keep these two
// in sync (mesh builds vertices with this, heightmap is written and sampled
// in GLSL twin so vertex lands exactly on its heightmap texel)
static glm::vec3 cubeFaceDir(int face, float u, float v) {
	float cx = u * 2.0f - 1.0f; // face-local x in [-1, 1]
	float cy = v * 2.0f - 1.0f; // face-local y in [-1, 1]
	glm::vec3 dir;
	if (face == 0) dir = glm::vec3(1.0, -cy, -cx); 	 // +X
	else if (face == 1) dir = glm::vec3(-1.0, -cy, cx); // -X
	else if (face == 2) dir = glm::vec3(cx, 1.0, cy);   // +Y
	else if (face == 3) dir = glm::vec3(cx, -1.0, -cy); // -Y
	else if (face == 4) dir = glm::vec3(cx, -cy, 1.0);  // +Z
	else dir = glm::vec3(-cx, -cy, -1.0); 			   	 // -Z
	return normalize(dir);
}

// similar to terrain_mesh.cpp -> 1 to 6 faces + 3 dimensions (displace)
CubeSphereMesh::CubeSphereMesh(int resolution) {
	int verts = resolution + 1;

	std::vector<float> positions;
	positions.reserve(6 * verts * verts * 3);

	std::vector<unsigned int> indices;
	indices.reserve(6 * resolution * resolution * 3);

	for (int face = 0; face < 6; face++) {
		// first vertex index of this faces block in the shared buffer
		unsigned int base = (unsigned int)(positions.size()/3);

		for (int y = 0; y < verts; y++) {
			for (int x = 0; x < verts; x++) {
				glm::vec3 dir = cubeFaceDir(face, (float)x / resolution, (float)y / resolution);
				positions.push_back(dir.x);
				positions.push_back(dir.y);
				positions.push_back(dir.z);
			}
		}

		for (int y = 0; y < resolution; y++) {
			for (int x = 0; x < resolution; x++) {
				unsigned int tl = base + y * verts + x;
				unsigned int tr = tl + 1;
				unsigned int bl = tl + verts;
				unsigned int br = bl + 1;
				indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
				indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
			}
		}
	}
	m_indexCount = (int)indices.size();

	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ebo);

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
}
CubeSphereMesh::~CubeSphereMesh() {
	glDeleteBuffers(1, &m_ebo);
	glDeleteBuffers(1, &m_vbo);
	glDeleteVertexArrays(1, &m_vao);
}
void CubeSphereMesh::draw() const {
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

