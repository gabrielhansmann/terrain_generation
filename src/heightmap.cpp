#include "perlin_noise.h"
#include <fstream>
#include "heightmap.h"


Heightmap::Heightmap(const int width, const int height):
	mWidth(width), mHeight(height)
{	
	mData = new float[width * height];
}

void Heightmap::create(float scale)
{
	PerlinNoise pn;

	for(int y = 0; y < mHeight; y++)
	{
		int rowOffset = y * mWidth;
	
		for(int x = 0; x < mWidth; x++)
		{
			mData[rowOffset + x] = pn.fractalNoise(y * scale, x * scale, 4, 2.0f, 0.5f); 
		}
	}
}
void Heightmap::writePGM(const char *fileName) const
{
	std::ofstream pgmFile(fileName);	
	pgmFile << "P2" << std::endl;
	pgmFile << mWidth << " " << mHeight << std::endl;
	pgmFile << "255" << std::endl;
	for(int y = 0; y < mHeight; y++)
	{
		for(int x = 0; x < mWidth; x++)
		{
			pgmFile << (int) (mData[y*mWidth+x] * 255) << " ";
		}
		pgmFile << std::endl;
	}
	pgmFile.close();
}

Heightmap::~Heightmap()
{
	delete[] mData;
	mData = nullptr;
}
