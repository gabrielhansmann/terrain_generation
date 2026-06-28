#pragma once

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path);
GLuint LoadShadersWithDefines(const char* vertex_file_path, const char* fragment_file_path, const std::string& defines);
GLuint LoadComputeShader(const char* computePath);
GLuint LoadComputeShaderWithDefines(const char* computePath, const std::string& defines);
void ReloadPrograms(GLuint& gbufferProgram, GLuint& lightingProgram, GLuint& waterProgram, const std::string& defines);
void ReloadFlatPrograms(GLuint& gbufferProgram, GLuint& lightingProgram, GLuint& wireframeProgram, const std::string& defines);
// Endless-runner flat variants: same shaders compiled with RUNNER defined so the
// G-buffer and lighting use the real CPU camera instead of the GetRay camera.
void ReloadRunnerPrograms(GLuint& gbufferProgram, GLuint& lightingProgram, const std::string& defines);
