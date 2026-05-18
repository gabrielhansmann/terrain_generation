#pragma once
#include <glm/glm.hpp>

struct NoiseGrad {
	float value;
	float dx;
	float dy;
};

class PerlinNoise
{
private: 
	int p[256*2];
	glm::vec2 gradients[16] = {
		{1.f, 0.f}, {0.f, 1.f}, {-1.f, 0.f}, {0.f, -1.f},
		{1.f, 1.f}, {-1.f, 1.f}, {-1.f, -1.f}, {1.f, -1.f},
		{0.9239f, 0.3827f}, {0.3827f, 0.9239f}, {-0.3827f, 0.9239f}, {-0.9239f, 0.3827f},
		{-0.9239f, -0.3827f}, {-0.3827f, -0.9239f}, {0.3827f, -0.9239f}, {0.9239f, -0.3827f}
	};

public:
	PerlinNoise(unsigned seed = 0);
	float noise(float x, float y);
	NoiseGrad noiseGrad(float x, float y);  // value + analytical derivatives — for normal map generation
	float fractalNoise(float x, float y, int octaves, float lacunarity, float persistence);
};

