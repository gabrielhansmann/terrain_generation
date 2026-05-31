#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "utils/shaders.h"
#include "utils/orbit_camera.h"
#include "perlin_noise.h"
#include "compute_pass.h"
#include "framebuffer.h"
#include "terrain_mesh.h"
#include "ui.h"

static int SCREEN_W = 960, SCREEN_H = 540;
static float mouseX = 0, mouseY = 0;
static bool mouseDown = false;

static void mouseButtonCB(GLFWwindow*, int btn, int action, int) {
	if (btn == GLFW_MOUSE_BUTTON_LEFT)
		mouseDown = (action == GLFW_PRESS);
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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return -1;
    }

	Ui ui;
	ui.init(window);

	// Dummy VAO - fullscreen.vert uses gl_VertexID, needs no VBO
	GLuint vao;
	glGenVertexArrays(1, &vao);
	ShaderSettings shaderSettings;
	GLuint progGBuffer = 0;
	GLuint progLighting = 0;
	ReloadPrograms(progGBuffer, progLighting, ui.buildShaderDefines(shaderSettings));

	// erosion pass
	ComputePass erosionPass(1080, 1080, "shaders/erosion.comp");

	// detail pass
	ComputePass detailPass(1080, 1080, "shaders/detail.comp");

	//	G-buffer
	Framebuffer gbuffer(SCREEN_W, SCREEN_H, 4);

	TerrainMesh terrainMesh(256);
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
		glfwGetFramebufferSize(window, &SCREEN_W, &SCREEN_H);
		gbuffer.resize(SCREEN_W, SCREEN_H);

		detailPass.dispatch();
		erosionPass.dispatch();
		camera.update(t, mouseX, mouseY, mouseDown, SCREEN_W, SCREEN_H, shaderSettings);

		// Pass 1 - G-buffer: march + material -> 4 render targets 
		gbuffer.bind();
		glViewport(0, 0, SCREEN_W, SCREEN_H);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		float skyDepth[4] = { -1.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 3, skyDepth); // attachment 3 = gDepth
		glEnable(GL_DEPTH_TEST);
		glUseProgram(progGBuffer);
		glUniform3f(glGetUniformLocation(progGBuffer, "iResolution"), (float)SCREEN_W, (float)SCREEN_H, 1.0f);
		glUniform1f(glGetUniformLocation(progGBuffer, "iTime"), t);
		glUniform4f(glGetUniformLocation(progGBuffer, "iMouse"), mouseX, (float)SCREEN_H - mouseY, mouseDown ? 1.0f : 0.0f, 0.0f);

		glm::mat4 vp = camera.viewProjMatrix();
		glUniformMatrix4fv(glGetUniformLocation(progGBuffer, "uViewProj"), 1, GL_FALSE, &vp[0][0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, erosionPass.texture());
		glUniform1i(glGetUniformLocation(progGBuffer, "iChannel0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, detailPass.texture());
		glUniform1i(glGetUniformLocation(progGBuffer, "iChannel1"), 1);

		float ichRes[12] = {
			1080.0f, 1080.0f, 1.0f, // iChannel0: erosion heightmap (fixed size)
			1080.0f, 1080.0f, 1.0f, //iChannel1: detail noise (window-sized)
			(float)DITHER_SIZE, (float)DITHER_SIZE, 1.0f,
			1.0f, 1.0f, 1.0f,
		};
		glUniform3fv(glGetUniformLocation(progGBuffer, "iChannelResolution[0]"), 4, ichRes);

		terrainMesh.draw();
		glDisable(GL_DEPTH_TEST);
		gbuffer.unbind();

		// Pass 2 - Lighting: reads G-buffer, runs BRDF + shadow + athmosphere -> screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCREEN_W, SCREEN_H);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(progLighting);

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

		// heightmap for shadow and reflection marches
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, erosionPass.texture());
		glUniform1i(glGetUniformLocation(progLighting, "iChannel0"), 4);

		// dither
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, texDither);
		glUniform1i(glGetUniformLocation(progLighting, "iChannel2"), 5);

		glUniform3fv(glGetUniformLocation(progLighting, "iChannelResolution[0]"), 4, ichRes);

		glBindVertexArray(vao);
		drawFullscreen();

		ui.beginFrame();
		bool shaderDirty = ui.renderOptions(shaderSettings);
		ui.endFrame();

		if (shaderDirty)
			ReloadPrograms(progGBuffer, progLighting, ui.buildShaderDefines(shaderSettings));

        glfwSwapBuffers(window);
    }

	// Cleanup
	ui.shutdown();
    glfwTerminate();
    return 0;
}
