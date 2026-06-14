#include <algorithm>
#include <random>
#include "perlin_noise.h" 

static float quinticCurve(float t) { return t*t*t*(t*(t*6.f - 15.f) + 10.f); }

PerlinNoise::PerlinNoise(unsigned seed)
{
	for (size_t i = 0; i < 256; i++)
		p[i] = i;
	
	std::mt19937 generator(seed);
	std::shuffle(&p[0], &p[256], generator);
	for (int i = 0; i < 256; i++)
		p[256 + i] = p[i];

}
float PerlinNoise::noise (float x, float y) 
{

	int xFloored{ ((int) std::floor(x)) };
	int yFloored{ ((int) std::floor(y)) };

	float xDist = x - (float) xFloored;	
	float yDist = y - (float) yFloored;	

	int xIndex = xFloored & 255;
	int yIndex = yFloored & 255;

	float u{ quinticCurve(xDist) };
	float v{ quinticCurve(yDist) };

	int numGradients = sizeof(gradients) / sizeof(gradients[0]);

	int gi00 = p[p[xIndex] + yIndex] & (numGradients - 1);
	int gi10 = p[p[xIndex + 1] + yIndex] & (numGradients - 1);
	int gi01 = p[p[xIndex] + yIndex + 1] & (numGradients - 1);
	int gi11 = p[p[xIndex + 1] + yIndex + 1] & (numGradients - 1);

	// float dp00 = gradients[gi00].x * xDist + gradients[gi00].y * yDist;
	// float dp10 = gradients[gi10].x * (xDist - 1) + gradients[gi10].y * yDist;
	// float dp01 = gradients[gi01].x * xDist + gradients[gi01].y * (yDist - 1);
	// float dp11 = gradients[gi11].x * (xDist - 1) + gradients[gi11].y * (yDist - 1);
	
	// use glm::dot instead of manually calculating dot product
	float dp00 = glm::dot(gradients[gi00], glm::vec2(xDist, yDist));
	float dp10 = glm::dot(gradients[gi10], glm::vec2(xDist - 1.f, yDist));
	float dp01 = glm::dot(gradients[gi01], glm::vec2(xDist, yDist - 1.f));
	float dp11 = glm::dot(gradients[gi11], glm::vec2(xDist - 1.f, yDist - 1.f));

	// before: custom lerp function -> glm::mix
	float top = glm::mix(dp00, dp10, u);
	float bottom = glm::mix(dp01, dp11, u);

	// map -1.0 ... 1.0 to 0.0 ... 1.0
	return (glm::mix(top, bottom, v) + 1) / 2.0f;
}

float PerlinNoise::fractalNoise(float x, float y, int octaves, float lacunarity, float persistence)
{
	float total{ 0.0f };
	float frequency{ 1.0f };
	float amplitude{ 1.0f };
	float maxAmplitude{ 0.0f };

	for (int i = 0; i < octaves; i++)
	{
		total += noise(x * frequency, y * frequency) * amplitude;

		maxAmplitude += amplitude;

		frequency *= lacunarity;
		amplitude *= persistence;
	}

	return total/maxAmplitude;
	
}
