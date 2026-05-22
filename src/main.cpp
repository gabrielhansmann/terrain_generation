#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <vector>
#include "utils/shaders.h"
#include "perlin_noise.h"
#include "compute_pass.h"

static int SCREEN_W = 1920, SCREEN_H = 1080;
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
	GLuint progImg = LoadShaders("shaders/fullscreen.vert", "shaders/terrain_flat.frag");


	// erosion pass
	ComputePass erosionPass(1080, 1080, "shaders/erosion.comp");

	// detail pass
	ComputePass detailPass(1080, 1080, "shaders/detail.comp");

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

		detailPass.dispatch();
		erosionPass.dispatch();

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
		glBindTexture(GL_TEXTURE_2D, erosionPass.texture());
		glUniform1i(glGetUniformLocation(progImg, "iChannel0"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, detailPass.texture());
		glUniform1i(glGetUniformLocation(progImg, "iChannel1"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, texDither);
		glUniform1i(glGetUniformLocation(progImg, "iChannel2"), 2);

		// Upload all 4 iChannelResolution slots — drivers can strip unreferenced
		// indices, so querying [2] alone sometimes returns -1.
		float ichRes[12] = {
			1080.0f, 1080.0f, 1.0f, // iChannel0: erosion heightmap (fixed size)
			1080.0f, 1080.0f, 1.0f, //iChannel1: detail noise (window-sized)
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
