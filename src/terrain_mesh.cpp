#include "terrain_mesh.h"
#include <vector>

TerrainMesh::TerrainMesh(int resolution) {
	int verts = resolution + 1;

	std::vector<float> positions;
	positions.reserve(verts * verts * 2);

	for (int z = 0; z < verts; z++) {
		for (int x = 0; x < verts; x++) {
			positions.push_back((float)x / resolution - 0.5f);
			positions.push_back((float)z / resolution - 0.5f);
		}
	}

	std::vector<unsigned int> indices;
	indices.reserve(resolution * resolution * 6);
	for (int z = 0; z < resolution; z++) {
		for (int x = 0; x < resolution; x++) {
			unsigned int tl = z * verts + x;
			unsigned int tr = tl + 1;
			unsigned int bl = tl + verts;
			unsigned int br = bl + 1;
			indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
			indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
		}
	}
	m_indexCount = (int)indices.size();

	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ebo);

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
}

TerrainMesh::~TerrainMesh() {
	glDeleteBuffers(1, &m_ebo);
	glDeleteBuffers(1, &m_vbo);
	glDeleteVertexArrays(1, &m_vao);
}

void TerrainMesh::draw() const {
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}
