#pragma once

#include <string>

struct GLFWwindow;

struct ShaderSettings {
	bool animateParameters = true;
	bool shadows = true;
	bool fixedSun = true;
	bool water = true;
	bool drainage = true;
	bool trees = true;
	bool detailTexture = true;
	bool cameraMouseControl = true;

	bool useAutoLowAngle = true;
	bool lowAngle = false;
	bool useAnimatedWaterHeight = true;
	bool useAutoScrollOffset = true;

	float waterHeight = 0.36f;
	float fogHeight = 0.465f;
	float grassHeight = 0.465f;
	float drainageWidth = 0.3f;
	float raymarchQuality = 2.0f;

	float cameraDistLow = 1.5f;
	float cameraDistHigh = 3.25f;
	float cameraFovLow = 20.0f;
	float cameraFovHigh = 11.0f;
	float cameraAngleLow = 0.25f;
	float cameraAngleHigh = -0.45f;
	float cameraElevationLow = -0.35f;
	float cameraElevationHigh = -0.43f;
	float cameraLookAt[3] = {0.0f, 0.40f, 0.0f};

	float timeScrollOffset[2] = {0.0f, 0.0f};
	float timeCamSpinLow = 0.0f;
	float timeCamSpinHigh = 1.0f / 60.0f;
	float timeCamWobble = 0.0f;

	float erosionScale = 0.15f;
	float erosionStrength = 0.22f;
	float erosionGullyWeight = 0.5f;
	float erosionDetail = 1.5f;
	float erosionRounding[4] = {0.1f, 0.0f, 0.1f, 2.0f};
	float erosionOnset[4] = {1.25f, 1.25f, 2.8f, 1.5f};
	float erosionAssumedSlope[2] = {0.7f, 1.0f};
	float erosionCellScale = 0.7f;
	float erosionNormalization = 0.5f;
	int erosionOctaves = 5;
	float erosionLacunarity = 2.0f;
	float erosionGain = 0.5f;
	float terrainHeightOffset[2] = {-0.65f, 0.0f};
	float heightFrequency = 3.0f;
	int heightOctaves = 3;
	float heightLacunarity = 2.0f;
	float heightGain = 0.1f;
	float heightAmp = 0.125f;

	bool greyscale = false;
	bool showRidgemap = false;
	bool showDiffuse = false;
	bool showNormals = false;
	bool comparisonSlider = false;
	bool showBuffer = false;
	int showBufferNr = 0;

	float cliffColor[3] = {0.22f, 0.2f, 0.2f};
	float dirtColor[3] = {0.6f, 0.5f, 0.4f};
	float treeColor[3] = {0.12f, 0.26f, 0.1f};
	float grassColor1[3] = {0.15f, 0.3f, 0.1f};
	float grassColor2[3] = {0.4f, 0.5f, 0.2f};
	float sandColor[3] = {0.8f, 0.7f, 0.6f};
	float waterColor[3] = {0.0f, 0.05f, 0.1f};
	float waterShoreColor[3] = {0.0f, 0.25f, 0.25f};
	float sunColor[3] = {2.0f, 1.96f, 1.9f};
	float ambientColor[3] = {0.03f, 0.05f, 0.07f};
};

class Ui {
	public:
		void init(GLFWwindow* window);
		void shutdown();
		void beginFrame();
		bool renderOptions(ShaderSettings& settings);
		void endFrame();
		std::string buildShaderDefines(const ShaderSettings& settings) const;
	private:
		bool m_initialized = false;
};
