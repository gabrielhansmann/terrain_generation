#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <vector>
#include "utils/shaders.h"
#include "perlin_noise.h"

static int SCREEN_W = 1920, SCREEN_H = 1080;
static float mouseX = 0, mouseY = 0;
static bool mouseDown = false;

static void mouseButtonCB(GLFWwindow*, int btn, int action, int) {
	if (btn == GLFW_MOUSE_BUTTON_LEFT)
		mouseDown = (action == GLFW_PRESS);
}

static GLuint makeFBO(int w, int h, GLuint& tex) {
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return fbo;
}

static void drawFullscreen() {
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

	// Dummy VAO - fullscreen.vert uses gl_VertexID, needs no VBO
	GLuint vao;
	glGenVertexArrays(1, &vao);
	GLuint progB = LoadShaders("shaders/fullscreen.vert", "shaders/buffer_b.frag");
	GLuint progA = LoadShaders("shaders/fullscreen.vert", "shaders/buffer_a.frag");
	GLuint progImg = LoadShaders("shaders/fullscreen.vert", "shaders/terrain_flat.frag");

	// Buffer A/B FBOs match the window size — the Shadertoy convention. The
	// shader's `BUFFER_SIZE = min(iRes.x, iRes.y, 1080)` then limits real work
	// to a square sub-region via DISCARD_MAP, and the image samples exactly
	// that region with `uv *= BUFFER_SIZE / iResolution.xy`.
	int FBO_W = SCREEN_W, FBO_H = SCREEN_H;
	GLuint texA, texB, fboA, fboB;
	fboA = makeFBO(FBO_W, FBO_H, texA);
	fboB = makeFBO(FBO_W, FBO_H, texB);

	// Stand-in for the Shadertoy keyboard texture on Buffer A's iChannel1.
	// Prevents that sampler from aliasing texA via texture unit 0.
	GLuint texZero;
	glGenTextures(1, &texZero);
	glBindTexture(GL_TEXTURE_2D, texZero);
	float zeroPixel[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, zeroPixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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

		// Resize the heightmap / detail FBOs to follow the window.
		if (SCREEN_W != FBO_W || SCREEN_H != FBO_H) {
			glDeleteTextures(1, &texA);
			glDeleteTextures(1, &texB);
			glDeleteFramebuffers(1, &fboA);
			glDeleteFramebuffers(1, &fboB);
			FBO_W = SCREEN_W;
			FBO_H = SCREEN_H;
			fboA = makeFBO(FBO_W, FBO_H, texA);
			fboB = makeFBO(FBO_W, FBO_H, texB);
		}

		// Pass 1a - Buffer B (detail noise). Re-rendered each frame so its
		// TIME_SCROLL_OFFSET_INT advances in lockstep with Buffer A.
		glBindFramebuffer(GL_FRAMEBUFFER, fboB);
		glViewport(0, 0, FBO_W, FBO_H);
		glUseProgram(progB);
		glUniform3f(glGetUniformLocation(progB, "iResolution"), (float)FBO_W, (float)FBO_H, 1.0f);
		glUniform1f(glGetUniformLocation(progB, "iTime"), t);
		glBindVertexArray(vao);
		drawFullscreen();

		// Pass 1b - Buffer A (erosion heightmap)
		glBindFramebuffer(GL_FRAMEBUFFER, fboA);
		glViewport(0, 0, FBO_W, FBO_H);
		glUseProgram(progA);
		glUniform3f(glGetUniformLocation(progA, "iResolution"), (float)FBO_W, (float)FBO_H, 1.0f);
		glUniform1f(glGetUniformLocation(progA, "iTime"), t);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texZero);
		glUniform1i(glGetUniformLocation(progA, "iChannel1"), 0);
		glBindVertexArray(vao);
		drawFullscreen();

		// Pass 2 - Image renders straight to the default framebuffer at the
		// full window size; iResolution matches so the camera centres correctly.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCREEN_W, SCREEN_H);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(progImg);
		glUniform3f(glGetUniformLocation(progImg, "iResolution"), (float)SCREEN_W, (float)SCREEN_H, 1.0f);
		glUniform1f(glGetUniformLocation(progImg, "iTime"), t);
		glUniform4f(glGetUniformLocation(progImg, "iMouse"), mouseX, (float)SCREEN_H - mouseY, mouseDown ? 1.0f : 0.0f, 0.0f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texA);
		glUniform1i(glGetUniformLocation(progImg, "iChannel0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texB);
		glUniform1i(glGetUniformLocation(progImg, "iChannel1"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texDither);
		glUniform1i(glGetUniformLocation(progImg, "iChannel2"), 2);

		// Upload all 4 iChannelResolution slots — drivers can strip unreferenced
		// indices, so querying [2] alone sometimes returns -1.
		float ichRes[12] = {
			(float)FBO_W, (float)FBO_H, 1.0f,
			(float)FBO_W, (float)FBO_H, 1.0f,
			(float)DITHER_SIZE, (float)DITHER_SIZE, 1.0f,
			1.0f, 1.0f, 1.0f,
		};
		glUniform3fv(glGetUniformLocation(progImg, "iChannelResolution[0]"), 4, ichRes);

		glBindVertexArray(vao);
		drawFullscreen();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Options Menu");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    glfwTerminate();
    return 0;
}
