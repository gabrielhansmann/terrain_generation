#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "planet/cube_sphere_mesh.h"
#include "planet/terrain_mesh.h"
#include "glm/matrix.hpp"
#include "engine/shaders.h"
#include "app/camera.h"
#include "engine/perlin_noise.h"
#include "engine/compute_pass.h"
#include "engine/framebuffer.h"
#include "app/ui.h"

static int SCREEN_W = 960, SCREEN_H = 540;
static float mouseX = 0, mouseY = 0;
static bool mouseDown = false;

static void mouseButtonCB(GLFWwindow*, int btn, int action, int) {
	if (btn == GLFW_MOUSE_BUTTON_LEFT)
		mouseDown = (action == GLFW_PRESS);
}

static float scrollAccum = 0.0f;
static void scrollCB(GLFWwindow*, double, double yoffset) {
	scrollAccum += (float)yoffset;	
}

static void drawFullscreen() {
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(SCREEN_W, SCREEN_H, "Terrain Generation", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    // Needed so the shader's `iMouse.z > 0.5` orbit-camera branch ever fires.
    glfwSetMouseButtonCallback(window, mouseButtonCB);
	glfwSetScrollCallback(window, scrollCB);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return -1;
    }

	Ui ui;
	ui.init(window);

	// Dummy VAO - fullscreen.vert uses gl_VertexID, needs no VBO
	GLuint vao;
	glGenVertexArrays(1, &vao);
	ShaderSettings shaderSettings;
	GLuint progGBufferSphere = 0;
	GLuint progLightingSphere = 0;
	GLuint progWaterSphere = 0;
	GLuint progGBufferFlat = 0;
	GLuint progLightingFlat = 0;
	GLuint progWireframeFlat = 0;
	GLuint progGBufferRunner = 0;
	GLuint progLightingRunner = 0;
	std::string shaderDefines = ui.buildShaderDefines(shaderSettings);
	ReloadPrograms(progGBufferSphere, progLightingSphere, progWaterSphere, shaderDefines);
	ReloadFlatPrograms(progGBufferFlat, progLightingFlat, progWireframeFlat, shaderDefines);
	ReloadRunnerPrograms(progGBufferRunner, progLightingRunner, shaderDefines);

	GLuint progDebugFace = LoadShaders("shaders/fullscreen.vert", "shaders/planet/debug_cubemap_face.frag");
	GLuint progDebugHeightmap = LoadShaders("shaders/fullscreen.vert", "shaders/planet/debug_heightmap_2d.frag");
	GLuint progWireframeSphere = LoadShadersWithDefines("shaders/planet/terrain.vert", "shaders/planet/wireframe.frag", shaderDefines);

	// Sphere path: direction-addressed cubemap heightmap
	ComputePass erosionPassSphere(1024, 1024, "shaders/planet/erosion.comp", ComputePassTextureType::CubeMapGenerated, nullptr, shaderDefines);
	// Flat path: 2D heightmap for the plane mesh
	ComputePass erosionPassFlat(1024, 1024, "shaders/planet/erosion_flat.comp", ComputePassTextureType::Texture2D, nullptr, shaderDefines);

	// detail pass: static 2D noise; used by the flat-plane shading path
	ComputePass detailPass(1024, 1024, "shaders/planet/detail.comp", ComputePassTextureType::Texture2D, nullptr, shaderDefines);

	//	G-buffer
	Framebuffer gbuffer(SCREEN_W, SCREEN_H, 4);

	CubeSphereMesh sphereMesh(128); // quads per face
	TerrainMesh flatMesh(256);      // quads per side
	OrbitCamera camera;

	// Dither noise for image iChannel2 (color += texture(..).xxx / 255.0).
	const int DITHER_SIZE = 256;
	PerlinNoise perlin(1337);
	std::vector<float> ditherPixels(DITHER_SIZE * DITHER_SIZE);
	for (int y = 0; y < DITHER_SIZE; ++y)
		for (int x = 0; x < DITHER_SIZE; ++x)
			ditherPixels[y * DITHER_SIZE + x] = perlin.noise(x * 0.5f, y * 0.5f);
	GLuint texDither;
	glGenTextures(1, &texDither);
	glBindTexture(GL_TEXTURE_2D, texDither);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, DITHER_SIZE, DITHER_SIZE, 0, GL_RED, GL_FLOAT, ditherPixels.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// small target for the cubemap face viewer (debug panel).
    const int FACE_VIEW_SIZE = 256;
    GLuint faceViewTex, faceViewFbo;
    glGenTextures(1, &faceViewTex);
    glBindTexture(GL_TEXTURE_2D, faceViewTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FACE_VIEW_SIZE, FACE_VIEW_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenFramebuffers(1, &faceViewFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, faceViewFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, faceViewTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	int lastProjectionMode = shaderSettings.projectionMode;

    // Render Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		double cx, cy;
		glfwGetCursorPos(window, &cx, &cy);
		if (mouseDown) {
			mouseX = (float)cx;
			mouseY = (float)cy;
		}

		float t = (float)glfwGetTime() + 3600.0f;
		static double prevTime = glfwGetTime();
		double nowTime = glfwGetTime();
		float dt = (float)(nowTime - prevTime);
		prevTime = nowTime;
		if (dt > 0.1f) dt = 0.1f; // clamp so a stall doesn't teleport the runner
		glfwGetFramebufferSize(window, &SCREEN_W, &SCREEN_H);
		gbuffer.resize(SCREEN_W, SCREEN_H);

		const bool flatMode = shaderSettings.projectionMode == 1;
		const bool runnerActive = flatMode && shaderSettings.endlessRunner;
		if (shaderSettings.projectionMode != lastProjectionMode) {
			if (flatMode)
				erosionPassFlat.markDirty();
			else
				erosionPassSphere.markDirty();
			lastProjectionMode = shaderSettings.projectionMode;
		}

		// Camera + terrain scroll offset. Must run before the compute dispatch so
		// the runner's uWorldOffset is current for this frame's heightmap.
		if (runnerActive) {
			auto keyDown = [&](int k) { return glfwGetKey(window, k) == GLFW_PRESS; };
			camera.updateRunner(dt,
				keyDown(GLFW_KEY_W), keyDown(GLFW_KEY_S),
				keyDown(GLFW_KEY_D), keyDown(GLFW_KEY_A),
				keyDown(GLFW_KEY_I), keyDown(GLFW_KEY_K),
				keyDown(GLFW_KEY_J), keyDown(GLFW_KEY_L),
				SCREEN_W, SCREEN_H);
			glm::vec2 off = camera.runnerOffset();
			erosionPassFlat.setWorldOffset(off.x, off.y);
			detailPass.setWorldOffset(off.x, off.y);
		} else if (flatMode) {
			// Static flat terrain matches the historical implementation exactly.
			erosionPassFlat.setWorldOffset(0.0f, 0.0f);
			detailPass.setWorldOffset(0.0f, 0.0f);
			camera.updateFlat(t, mouseX, mouseY, mouseDown, SCREEN_W, SCREEN_H, shaderSettings);
		} else {
			camera.zoom(scrollAccum);
			camera.update(t, mouseX, mouseY, mouseDown, SCREEN_W, SCREEN_H);
		}
		scrollAccum = 0.0f;

		detailPass.dispatch();
		if (flatMode)
			erosionPassFlat.dispatch();
		else
			erosionPassSphere.dispatch();

		GLuint progGBuffer = runnerActive ? progGBufferRunner : (flatMode ? progGBufferFlat : progGBufferSphere);
		GLuint progWireframe = flatMode ? progWireframeFlat : progWireframeSphere;
		GLuint progLighting = runnerActive ? progLightingRunner : (flatMode ? progLightingFlat : progLightingSphere);

		// Pass 1 - G-buffer: rasterize mesh + material 
		gbuffer.bind();
		glViewport(0, 0, SCREEN_W, SCREEN_H);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		float skyDepth[4] = { -1.0f, 0.0f, 0.0f, 0.0f };
		// glClearBufferfv instead of glClear so that sky pixels never get touched
		// by the mesh and lighting.frag sky branch needs t = -1. glClear sets
		// all attachments to 0
		glClearBufferfv(GL_COLOR, 3, skyDepth); // attachment 3 = gDepth
		glEnable(GL_DEPTH_TEST);
		if (!flatMode) {
			// planet is a closed solid, triangles facing away from camera are never
			// visible -> cull them to skip G-buffer frag work
			glEnable(GL_CULL_FACE);
		}
		glUseProgram(progGBuffer);
		glUniform3f(glGetUniformLocation(progGBuffer, "iResolution"), (float)SCREEN_W, (float)SCREEN_H, 1.0f);
		glUniform1f(glGetUniformLocation(progGBuffer, "iTime"), t);
		glUniform4f(glGetUniformLocation(progGBuffer, "iMouse"), mouseX, (float)SCREEN_H - mouseY, mouseDown ? 1.0f : 0.0f, 0.0f);

		glm::mat4 vp = camera.viewProjMatrix();
		glUniformMatrix4fv(glGetUniformLocation(progGBuffer, "uViewProj"), 1, GL_FALSE, &vp[0][0]);
		glm::vec3 camPos = camera.position();
		if (!flatMode || runnerActive)
			glUniform3fv(glGetUniformLocation(progGBuffer, "uCamPos"), 1, &camPos[0]);

		glActiveTexture(GL_TEXTURE0);
		if (flatMode) {
			glBindTexture(GL_TEXTURE_2D, erosionPassFlat.texture());
		} else {
			glBindTexture(GL_TEXTURE_CUBE_MAP, erosionPassSphere.texture());
		}
		glUniform1i(glGetUniformLocation(progGBuffer, "iChannel0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, detailPass.texture());
		glUniform1i(glGetUniformLocation(progGBuffer, "iChannel1"), 1);

		float ichRes[12] = {
			1024.0f, 1024.0f, 1.0f, // iChannel0: erosion heightmap (fixed size)
			1024.0f, 1024.0f, 1.0f, //iChannel1: detail noise (window-sized)
			(float)DITHER_SIZE, (float)DITHER_SIZE, 1.0f,
			1.0f, 1.0f, 1.0f,
		};
		glUniform3fv(glGetUniformLocation(progGBuffer, "iChannelResolution[0]"), 4, ichRes);
		if (flatMode)
			flatMesh.draw();
		else {
			sphereMesh.draw();

			// water shell (sphere path only)
			glUseProgram(progWaterSphere);
			glUniformMatrix4fv(glGetUniformLocation(progWaterSphere, "uViewProj"), 1, GL_FALSE, &vp[0][0]);
			glUniform3fv(glGetUniformLocation(progWaterSphere, "uCamPos"), 1, &camPos[0]);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, erosionPassSphere.texture());
			glUniform1i(glGetUniformLocation(progWaterSphere, "iChannel0"), 0);
			sphereMesh.draw();
		}
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		gbuffer.unbind();

		// Pass 2 - screen: plain wireframe, or the full deferred lighting
		if (shaderSettings.wireframe) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, SCREEN_W, SCREEN_H);
			glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			// No depth test and no backface cull, so every triangle edge draws
			// (front and back faces both) and the whole grid shows through the sphere.
			glUseProgram(progWireframe);
			glm::mat4 vpWire = camera.viewProjMatrix();
			glUniformMatrix4fv(glGetUniformLocation(progWireframe, "uViewProj"), 1, GL_FALSE, &vpWire[0][0]);
			glActiveTexture(GL_TEXTURE0);
			if (flatMode) {
				glBindTexture(GL_TEXTURE_2D, erosionPassFlat.texture());
				glUniform1i(glGetUniformLocation(progWireframe, "iChannel0"), 0);
				glUniform3fv(glGetUniformLocation(progWireframe, "iChannelResolution[0]"), 4, ichRes);
			} else {
				glBindTexture(GL_TEXTURE_CUBE_MAP, erosionPassSphere.texture());
				glUniform1i(glGetUniformLocation(progWireframe, "iChannel0"), 0);
			}
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			if (flatMode)
				flatMesh.draw();
			else
				sphereMesh.draw();
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		} else {
		// Pass 2 - Lighting: reads G-buffer, runs BRDF + shadow + athmosphere -> screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCREEN_W, SCREEN_H);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(progLighting);

		if (!flatMode || runnerActive) {
			glm::vec3 lightCamPos = camera.position();
			glUniform3fv(glGetUniformLocation(progLighting, "uCamPos"), 1, &lightCamPos[0]);
			glm::mat4 invVP = glm::inverse(vp);
			glUniformMatrix4fv(glGetUniformLocation(progLighting, "uInvViewProj"), 1, GL_FALSE, &invVP[0][0]);
		}

		glUniform3f(glGetUniformLocation(progLighting, "iResolution"), (float)SCREEN_W, (float)SCREEN_H, 1.0f);
		glUniform1f(glGetUniformLocation(progLighting, "iTime"), t);
		glUniform4f(glGetUniformLocation(progLighting, "iMouse"), mouseX, (float)SCREEN_H - mouseY, mouseDown ? 1.0f : 0.0f, 0.0f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gbuffer.texture(0));
		glUniform1i(glGetUniformLocation(progLighting, "gAlbedoOcclusion"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gbuffer.texture(1));
		glUniform1i(glGetUniformLocation(progLighting, "gNormalMaterial"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gbuffer.texture(2));
		glUniform1i(glGetUniformLocation(progLighting, "gF0Smoothness"), 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gbuffer.texture(3));
		glUniform1i(glGetUniformLocation(progLighting, "gDepth"), 3);

		if (flatMode) {
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, erosionPassFlat.texture());
			glUniform1i(glGetUniformLocation(progLighting, "iChannel0"), 4);
		}

		// dither
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, texDither);
		glUniform1i(glGetUniformLocation(progLighting, "iChannel2"), 5);

		glUniform3fv(glGetUniformLocation(progLighting, "iChannelResolution[0]"), 4, ichRes);

		glBindVertexArray(vao);
		drawFullscreen();
		}

		// Debug panel: cubemap face (sphere) or 2D heightmap (flat)
		GLuint debugPreviewTex = faceViewTex;
        if (flatMode) {
			glBindFramebuffer(GL_FRAMEBUFFER, faceViewFbo);
			glViewport(0, 0, FACE_VIEW_SIZE, FACE_VIEW_SIZE);
			glUseProgram(progDebugHeightmap);
			glUniform1i(glGetUniformLocation(progDebugHeightmap, "uChannel"), shaderSettings.debugCubeChannel);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, erosionPassFlat.texture());
			glUniform1i(glGetUniformLocation(progDebugHeightmap, "uHeightmap"), 0);
			drawFullscreen();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		} else {
			glBindFramebuffer(GL_FRAMEBUFFER, faceViewFbo);
			glViewport(0, 0, FACE_VIEW_SIZE, FACE_VIEW_SIZE);
			glUseProgram(progDebugFace);
			glUniform1i(glGetUniformLocation(progDebugFace, "uFace"), shaderSettings.debugCubeFace);
			glUniform1i(glGetUniformLocation(progDebugFace, "uChannel"), shaderSettings.debugCubeChannel);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, erosionPassSphere.texture());
			glUniform1i(glGetUniformLocation(progDebugFace, "uCube"), 0);
			drawFullscreen();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		ui.beginFrame();
		bool shaderDirty = ui.renderOptions(shaderSettings, debugPreviewTex);
		ui.endFrame();

		if (shaderDirty)
			{
				shaderDefines = ui.buildShaderDefines(shaderSettings);
				ReloadPrograms(progGBufferSphere, progLightingSphere, progWaterSphere, shaderDefines);
				ReloadFlatPrograms(progGBufferFlat, progLightingFlat, progWireframeFlat, shaderDefines);
				ReloadRunnerPrograms(progGBufferRunner, progLightingRunner, shaderDefines);
				progWireframeSphere = LoadShadersWithDefines("shaders/planet/terrain.vert", "shaders/planet/wireframe.frag", shaderDefines);
				erosionPassSphere.reloadProgram(shaderDefines);
				erosionPassFlat.reloadProgram(shaderDefines);
				detailPass.reloadProgram(shaderDefines);
			}

        glfwSwapBuffers(window);
    }

	// Cleanup
	ui.shutdown();
    glfwTerminate();
    return 0;
}
