#include "ui.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <iomanip>
#include <sstream>

void Ui::init(GLFWwindow* window) {
	if (m_initialized)
		return;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
	m_initialized = true;
}

void Ui::shutdown() {
	if (!m_initialized)
		return;
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	m_initialized = false;
}

void Ui::beginFrame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

bool Ui::renderOptions(ShaderSettings& settings, GLuint cubeFaceTex) {
	bool shaderDirty = false;

	ImGui::Begin("Options Menu");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
		1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
		shaderDirty |= ImGui::Checkbox("Greyscale", &settings.greyscale);
		shaderDirty |= ImGui::Checkbox("Show ridgemap", &settings.showRidgemap);
		shaderDirty |= ImGui::Checkbox("Show diffuse", &settings.showDiffuse);
		shaderDirty |= ImGui::Checkbox("Show normals", &settings.showNormals);
		shaderDirty |= ImGui::Checkbox("Comparison slider", &settings.comparisonSlider);
		shaderDirty |= ImGui::Checkbox("Show buffer", &settings.showBuffer);
		shaderDirty |= ImGui::SliderInt("Show buffer index", &settings.showBufferNr, 0, 5);
		ImGui::SliderInt("Cube face", &settings.debugCubeFace, 0, 5);
        ImGui::SliderInt("Cube channel (-1=rgb)", &settings.debugCubeChannel, -1, 3);
        // GL textures start at the bottom-left, ImGui at the top-left, so the
        // V coordinate is flipped here to show the face the right way up
        ImGui::Image((ImTextureID)(uintptr_t)cubeFaceTex, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	}

	if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
		shaderDirty |= ImGui::Checkbox("Animate parameters", &settings.animateParameters);
		shaderDirty |= ImGui::Checkbox("Shadows", &settings.shadows);
		shaderDirty |= ImGui::Checkbox("Fixed sun", &settings.fixedSun);
		shaderDirty |= ImGui::Checkbox("Water", &settings.water);
		shaderDirty |= ImGui::Checkbox("Drainage", &settings.drainage);
		shaderDirty |= ImGui::Checkbox("Trees", &settings.trees);
		shaderDirty |= ImGui::Checkbox("Detail texture", &settings.detailTexture);

		shaderDirty |= ImGui::Checkbox("Animated water height", &settings.useAnimatedWaterHeight);
		if (!settings.useAnimatedWaterHeight)
			shaderDirty |= ImGui::SliderFloat("Water height", &settings.waterHeight, 0.0f, 1.0f);

		shaderDirty |= ImGui::SliderFloat("Fog height", &settings.fogHeight, 0.0f, 1.0f);
		shaderDirty |= ImGui::SliderFloat("Grass height", &settings.grassHeight, 0.0f, 1.0f);
		shaderDirty |= ImGui::SliderFloat("Drainage width", &settings.drainageWidth, 0.0f, 2.0f);
		shaderDirty |= ImGui::SliderFloat("Raymarch quality", &settings.raymarchQuality, 0.25f, 6.0f);
	}

	if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
		shaderDirty |= ImGui::Checkbox("Mouse control", &settings.cameraMouseControl);
		shaderDirty |= ImGui::Checkbox("Auto low angle", &settings.useAutoLowAngle);
		if (!settings.useAutoLowAngle)
			shaderDirty |= ImGui::Checkbox("Low angle", &settings.lowAngle);

		shaderDirty |= ImGui::SliderFloat("Camera dist (low)", &settings.cameraDistLow, 0.1f, 10.0f);
		shaderDirty |= ImGui::SliderFloat("Camera dist (high)", &settings.cameraDistHigh, 0.1f, 10.0f);
		shaderDirty |= ImGui::SliderFloat("Camera FOV (low)", &settings.cameraFovLow, 1.0f, 120.0f);
		shaderDirty |= ImGui::SliderFloat("Camera FOV (high)", &settings.cameraFovHigh, 1.0f, 120.0f);
		shaderDirty |= ImGui::SliderFloat("Camera angle (low)", &settings.cameraAngleLow, -3.14f, 3.14f);
		shaderDirty |= ImGui::SliderFloat("Camera angle (high)", &settings.cameraAngleHigh, -3.14f, 3.14f);
		shaderDirty |= ImGui::SliderFloat("Camera elevation (low)", &settings.cameraElevationLow, -3.14f, 3.14f);
		shaderDirty |= ImGui::SliderFloat("Camera elevation (high)", &settings.cameraElevationHigh, -3.14f, 3.14f);
		shaderDirty |= ImGui::DragFloat3("Camera look-at", settings.cameraLookAt, 0.01f, -10.0f, 10.0f);

		shaderDirty |= ImGui::Checkbox("Auto scroll offset", &settings.useAutoScrollOffset);
		if (!settings.useAutoScrollOffset)
			shaderDirty |= ImGui::DragFloat2("Scroll offset", settings.timeScrollOffset, 0.01f, -10.0f, 10.0f);

		shaderDirty |= ImGui::SliderFloat("Cam spin (low)", &settings.timeCamSpinLow, -1.0f, 1.0f);
		shaderDirty |= ImGui::SliderFloat("Cam spin (high)", &settings.timeCamSpinHigh, -1.0f, 1.0f);
		shaderDirty |= ImGui::SliderFloat("Cam wobble", &settings.timeCamWobble, -1.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader("Erosion", ImGuiTreeNodeFlags_DefaultOpen)) {
		shaderDirty |= ImGui::SliderFloat("Erosion scale", &settings.erosionScale, 0.0f, 1.0f);
		shaderDirty |= ImGui::SliderFloat("Erosion strength", &settings.erosionStrength, 0.0f, 1.0f);
		shaderDirty |= ImGui::SliderFloat("Gully weight", &settings.erosionGullyWeight, 0.0f, 1.0f);
		shaderDirty |= ImGui::SliderFloat("Detail", &settings.erosionDetail, 0.0f, 4.0f);
		shaderDirty |= ImGui::DragFloat4("Rounding", settings.erosionRounding, 0.01f, 0.0f, 4.0f);
		shaderDirty |= ImGui::DragFloat4("Onset", settings.erosionOnset, 0.01f, 0.0f, 4.0f);
		shaderDirty |= ImGui::DragFloat2("Assumed slope", settings.erosionAssumedSlope, 0.01f, 0.0f, 2.0f);
		shaderDirty |= ImGui::SliderFloat("Cell scale", &settings.erosionCellScale, 0.0f, 2.0f);
		shaderDirty |= ImGui::SliderFloat("Normalization", &settings.erosionNormalization, 0.0f, 1.0f);
		shaderDirty |= ImGui::SliderInt("Erosion octaves", &settings.erosionOctaves, 1, 8);
		shaderDirty |= ImGui::SliderFloat("Erosion lacunarity", &settings.erosionLacunarity, 1.0f, 4.0f);
		shaderDirty |= ImGui::SliderFloat("Erosion gain", &settings.erosionGain, 0.0f, 1.0f);
		shaderDirty |= ImGui::DragFloat2("Height offset", settings.terrainHeightOffset, 0.01f, -2.0f, 2.0f);
		shaderDirty |= ImGui::SliderFloat("Height frequency", &settings.heightFrequency, 0.0f, 10.0f);
		shaderDirty |= ImGui::SliderInt("Height octaves", &settings.heightOctaves, 1, 8);
		shaderDirty |= ImGui::SliderFloat("Height lacunarity", &settings.heightLacunarity, 1.0f, 4.0f);
		shaderDirty |= ImGui::SliderFloat("Height gain", &settings.heightGain, 0.0f, 1.0f);
		shaderDirty |= ImGui::SliderFloat("Height amp", &settings.heightAmp, 0.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader("Colors", ImGuiTreeNodeFlags_DefaultOpen)) {
		shaderDirty |= ImGui::ColorEdit3("Cliff", settings.cliffColor);
		shaderDirty |= ImGui::ColorEdit3("Dirt", settings.dirtColor);
		shaderDirty |= ImGui::ColorEdit3("Tree", settings.treeColor);
		shaderDirty |= ImGui::ColorEdit3("Grass 1", settings.grassColor1);
		shaderDirty |= ImGui::ColorEdit3("Grass 2", settings.grassColor2);
		shaderDirty |= ImGui::ColorEdit3("Sand", settings.sandColor);
		shaderDirty |= ImGui::ColorEdit3("Water", settings.waterColor);
		shaderDirty |= ImGui::ColorEdit3("Water shore", settings.waterShoreColor);
		shaderDirty |= ImGui::ColorEdit3("Sun", settings.sunColor);
		shaderDirty |= ImGui::ColorEdit3("Ambient", settings.ambientColor);
	}

	if (ImGui::Button("Recompile shaders"))
		shaderDirty = true;
	ImGui::End();

	return shaderDirty;
}

void Ui::endFrame() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

std::string Ui::buildShaderDefines(const ShaderSettings& s) const {
	std::ostringstream out;
	out << std::fixed << std::setprecision(6);

	out << "#define ANIMATE_PARAMETERS " << (s.animateParameters ? 1 : 0) << "\n";

	out << "#define SHADOWS " << (s.shadows ? 1 : 0) << "\n";
	out << "#define FIXED_SUN " << (s.fixedSun ? 1 : 0) << "\n";
	out << "#define WATER " << (s.water ? 1 : 0) << "\n";
	out << "#define DRAINAGE " << (s.drainage ? 1 : 0) << "\n";
	out << "#define TREES " << (s.trees ? 1 : 0) << "\n";
	out << "#define DETAIL_TEXTURE " << (s.detailTexture ? 1 : 0) << "\n";
	out << "#define CAMERA_MOUSE_CONTROL " << (s.cameraMouseControl ? 1 : 0) << "\n";

	out << "#define GREYSCALE " << (s.greyscale ? 1 : 0) << "\n";
	out << "#define SHOW_RIDGEMAP " << (s.showRidgemap ? 1 : 0) << "\n";
	out << "#define SHOW_DIFFUSE " << (s.showDiffuse ? 1 : 0) << "\n";
	out << "#define SHOW_NORMALS " << (s.showNormals ? 1 : 0) << "\n";
	out << "#define COMPARISON_SLIDER " << (s.comparisonSlider ? 1 : 0) << "\n";
	out << "#define SHOW_BUFFER " << (s.showBuffer ? 1 : 0) << "\n";
	out << "#define SHOW_BUFFER_NR " << s.showBufferNr << "\n";

	if (s.useAnimatedWaterHeight) {
		out << "#define WATER_HEIGHT (0.36 + 0.1 * (smoothstep(54.0, 60.0, mod(iTime, 120.0)) - smoothstep(114.0, 120.0, mod(iTime, 120.0))))\n";
	} else {
		out << "#define WATER_HEIGHT " << s.waterHeight << "\n";
	}
	out << "#define FOG_HEIGHT " << s.fogHeight << "\n";
	out << "#define GRASS_HEIGHT " << s.grassHeight << "\n";
	out << "#define DRAINAGE_WIDTH " << s.drainageWidth << "\n";
	out << "#define RAYMARCH_QUALITY " << s.raymarchQuality << "\n";

	if (s.useAutoLowAngle) {
		out << "#define LOW_ANGLE (mod(iTime, 240.0) >= 120.0)\n";
	} else {
		out << "#define LOW_ANGLE " << (s.lowAngle ? "true" : "false") << "\n";
	}
	out << "#define CAMERA_DIST (LOW_ANGLE ? " << s.cameraDistLow << " : " << s.cameraDistHigh << ")\n";
	out << "#define CAMERA_FOV (LOW_ANGLE ? " << s.cameraFovLow << " : " << s.cameraFovHigh << ")\n";
	out << "#define CAMERA_ANGLE (LOW_ANGLE ? " << s.cameraAngleLow << " : " << s.cameraAngleHigh << ")\n";
	out << "#define CAMERA_ELEVATION (LOW_ANGLE ? " << s.cameraElevationLow << " : " << s.cameraElevationHigh << ")\n";
	out << "#define CAMERA_LOOKAT vec3(" << s.cameraLookAt[0] << ", " << s.cameraLookAt[1] << ", " << s.cameraLookAt[2] << ")\n";

	if (s.useAutoScrollOffset) {
		out << "#define TIME_SCROLL_OFFSET vec2(cos(iTime / 60.0 * 2.0 * PI) * 2.0, -sin(iTime / 60.0 * 2.0 * PI) * 0.1)\n";
	} else {
		out << "#define TIME_SCROLL_OFFSET vec2(" << s.timeScrollOffset[0] << ", " << s.timeScrollOffset[1] << ")\n";
	}
	out << "#define TIME_CAM_SPIN (LOW_ANGLE ? " << s.timeCamSpinLow << " : " << s.timeCamSpinHigh << ")\n";
	out << "#define TIME_CAM_WOBBLE " << s.timeCamWobble << "\n";

	out << "#define EROSION_SCALE " << s.erosionScale << "\n";
	out << "#define EROSION_STRENGTH " << s.erosionStrength << "\n";
	out << "#define EROSION_GULLY_WEIGHT " << s.erosionGullyWeight << "\n";
	out << "#define EROSION_DETAIL " << s.erosionDetail << "\n";
	out << "#define EROSION_ROUNDING vec4(" << s.erosionRounding[0] << ", " << s.erosionRounding[1] << ", " << s.erosionRounding[2] << ", " << s.erosionRounding[3] << ")\n";
	out << "#define EROSION_ONSET vec4(" << s.erosionOnset[0] << ", " << s.erosionOnset[1] << ", " << s.erosionOnset[2] << ", " << s.erosionOnset[3] << ")\n";
	out << "#define EROSION_ASSUMED_SLOPE vec2(" << s.erosionAssumedSlope[0] << ", " << s.erosionAssumedSlope[1] << ")\n";
	out << "#define EROSION_CELL_SCALE " << s.erosionCellScale << "\n";
	out << "#define EROSION_NORMALIZATION " << s.erosionNormalization << "\n";
	out << "#define EROSION_OCTAVES " << s.erosionOctaves << "\n";
	out << "#define EROSION_LACUNARITY " << s.erosionLacunarity << "\n";
	out << "#define EROSION_GAIN " << s.erosionGain << "\n";
	out << "#define TERRAIN_HEIGHT_OFFSET vec2(" << s.terrainHeightOffset[0] << ", " << s.terrainHeightOffset[1] << ")\n";
	out << "#define HEIGHT_FREQUENCY " << s.heightFrequency << "\n";
	out << "#define HEIGHT_OCTAVES " << s.heightOctaves << "\n";
	out << "#define HEIGHT_LACUNARITY " << s.heightLacunarity << "\n";
	out << "#define HEIGHT_GAIN " << s.heightGain << "\n";
	out << "#define HEIGHT_AMP " << s.heightAmp << "\n";

	out << "#define CLIFF_COLOR vec3(" << s.cliffColor[0] << ", " << s.cliffColor[1] << ", " << s.cliffColor[2] << ")\n";
	out << "#define DIRT_COLOR vec3(" << s.dirtColor[0] << ", " << s.dirtColor[1] << ", " << s.dirtColor[2] << ")\n";
	out << "#define TREE_COLOR vec3(" << s.treeColor[0] << ", " << s.treeColor[1] << ", " << s.treeColor[2] << ")\n";
	out << "#define GRASS_COLOR1 vec3(" << s.grassColor1[0] << ", " << s.grassColor1[1] << ", " << s.grassColor1[2] << ")\n";
	out << "#define GRASS_COLOR2 vec3(" << s.grassColor2[0] << ", " << s.grassColor2[1] << ", " << s.grassColor2[2] << ")\n";
	out << "#define SAND_COLOR vec3(" << s.sandColor[0] << ", " << s.sandColor[1] << ", " << s.sandColor[2] << ")\n";
	out << "#define WATER_COLOR vec3(" << s.waterColor[0] << ", " << s.waterColor[1] << ", " << s.waterColor[2] << ")\n";
	out << "#define WATER_SHORE_COLOR vec3(" << s.waterShoreColor[0] << ", " << s.waterShoreColor[1] << ", " << s.waterShoreColor[2] << ")\n";
	out << "#define SUN_COLOR vec3(" << s.sunColor[0] << ", " << s.sunColor[1] << ", " << s.sunColor[2] << ")\n";
	out << "#define AMBIENT_COLOR vec3(" << s.ambientColor[0] << ", " << s.ambientColor[1] << ", " << s.ambientColor[2] << ")\n";

	return out.str();
}
