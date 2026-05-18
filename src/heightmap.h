#pragma once

class Heightmap
{
private:
	int mWidth, mHeight;
	float *mData{nullptr};

public:
	Heightmap(int width, int height);
	void create(float scale);
	void writePGM(const char *fileName) const;
	~Heightmap();

	const float *getData() const { return mData; }
	int width()  const { return mWidth; }
	int height() const { return mHeight; }
};

