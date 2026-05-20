#include "compute_pass.h"
#include "utils/shaders.h"
#include <iostream>

ComputePass::ComputePass(int w, int h, const char* shaderPath): w_(w), h_(h) {
	program_ = LoadComputeShader(shaderPath);

	glGenTextures(1, &tex_);
	glBindTexture(GL_TEXTURE_2D, tex_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

}

ComputePass::~ComputePass() {
	glDeleteProgram(program_);
	glDeleteTextures(1, &tex_);
}

void ComputePass::markDirty() { dirty_ = true; }

void ComputePass::dispatch() {
	if (!dirty_) return;

	glUseProgram(program_);
	glBindImageTexture(0, tex_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	// a grid of 8x8 texel work groups covering the entire texture. Calling division ensures
	// we cover every texel when size isnt a multiple of 8
	// the shaders bounds check discards invocations that land past the edge
	glDispatchCompute((w_ + 7) / 8, (h_ + 7) / 8, 1);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	dirty_ = false;
}
