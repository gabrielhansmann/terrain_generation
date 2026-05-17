#include "terrain_plane.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "utils/shaders.h"

static GLuint g_VAO = 0;
static GLuint g_VBO = 0;
static GLuint g_EBO = 0;
static GLuint g_ShaderProgram = 0;

void Plane_Init(const char* vertexShaderPath, const char* fragmentShaderPath) {
    float vertices[] = {
        // positions         // normals
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &g_VAO);
    glGenBuffers(1, &g_VBO);
    glGenBuffers(1, &g_EBO);

    glBindVertexArray(g_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    g_ShaderProgram = LoadShaders(vertexShaderPath, fragmentShaderPath);
    if (!g_ShaderProgram) {
        std::cerr << "Failed to load plane shaders" << std::endl;
    }
}
void Plane_Render(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos) {
    if (!g_VAO || !g_ShaderProgram) return;

    glUseProgram(g_ShaderProgram);

    // model matrix with rotation over time
    float angle = (float)glfwGetTime() * glm::radians(20.0f);
    glm::mat4 tilt = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotateY = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 model = rotateY * tilt;

    GLint modelLoc = glGetUniformLocation(g_ShaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(g_ShaderProgram, "view");
    GLint projLoc = glGetUniformLocation(g_ShaderProgram, "projection");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // light and view positions
    GLint lightDirLoc = glGetUniformLocation(g_ShaderProgram, "lightDir");
    glUniform3f(lightDirLoc, -0.2f, -1.0f, -0.3f);
    GLint viewPosLoc = glGetUniformLocation(g_ShaderProgram, "viewPos");
    glUniform3f(viewPosLoc, viewPos.x, viewPos.y, viewPos.z);

    glBindVertexArray(g_VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Plane_Cleanup() {
    if (g_VBO) { glDeleteBuffers(1, &g_VBO); g_VBO = 0; }
    if (g_EBO) { glDeleteBuffers(1, &g_EBO); g_EBO = 0; }
    if (g_VAO) { glDeleteVertexArrays(1, &g_VAO); g_VAO = 0; }
    if (g_ShaderProgram) { glDeleteProgram(g_ShaderProgram); g_ShaderProgram = 0; }
}
