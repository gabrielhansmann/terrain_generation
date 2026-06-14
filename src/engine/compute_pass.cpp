#include "compute_pass.h"
#include "engine/shaders.h"
#include <iostream>

ComputePass::ComputePass(
	int w,
	int h,
	const char* shaderPath,
	ComputePassTextureType textureType,
	const void* const* cubeFaces,
	const std::string& defines
): shaderPath_(shaderPath), w_(w), h_(h), textureType_(textureType) {
	program_ = LoadComputeShaderWithDefines(shaderPath, defines);

	glGenTextures(1, &tex_);
	if (textureType_ == ComputePassTextureType::CubeMapGenerated) {
		glBindTexture(GL_TEXTURE_CUBE_MAP, tex_);
		// Immutable storage: we only ever imageStore into these faces, never
		// resize them. Full float precision because height drives vertex
		// displacement, where 8-bit banding would show as terraced terrain.
		imageFormat_ = GL_RGBA32F;
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, imageFormat_, w_, h_);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	} else if (textureType_ == ComputePassTextureType::CubeMap) {
		glBindTexture(GL_TEXTURE_CUBE_MAP, tex_);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);

		imageFormat_ = GL_RGBA8;
		for (int face = 0; face < 6; ++face) {
			const void* facePixels = cubeFaces ? cubeFaces[face] : nullptr;
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
				0,
				imageFormat_,
				w_,
				h_,
				0,
				GL_BGRA,
				GL_UNSIGNED_BYTE,
				facePixels
			);
		}
	} else {
		glBindTexture(GL_TEXTURE_2D, tex_);
		imageFormat_ = GL_RGBA32F;
		glTexImage2D(GL_TEXTURE_2D, 0, imageFormat_, w_, h_, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
}

ComputePass::~ComputePass() {
	glDeleteProgram(program_);
	glDeleteTextures(1, &tex_);
}

void ComputePass::markDirty() { dirty_ = true; }

void ComputePass::reloadProgram(const std::string& defines) {
	GLuint newProgram = LoadComputeShaderWithDefines(shaderPath_.c_str(), defines);
	if (program_)
		glDeleteProgram(program_);
	program_ = newProgram;
	dirty_ = true;
}

void ComputePass::dispatch() {
	if (!dirty_) return;

	glUseProgram(program_);

	// a grid of 8x8 texel work groups covering the entire texture. Ceil division
	// covers every texel when the size isn't a multiple of 8; the shader's bounds
	// check discards invocations that land past the edge.
	if (textureType_ == ComputePassTextureType::CubeMapGenerated) {
		// One dispatch per face: each binds a single face as a 2D image and tells
		// the shader which face direction it is generating, via the uFace uniform.
		GLint faceLoc = glGetUniformLocation(program_, "uFace");
		for (int face = 0; face < 6; ++face) {
			glUniform1i(faceLoc, face);
			// layered = GL_FALSE + layer = face selects exactly one cube face
			glBindImageTexture(0, tex_, 0, GL_FALSE, face, GL_WRITE_ONLY, imageFormat_);
			glDispatchCompute((w_ + 7) / 8, (h_ + 7) / 8, 1);
		}
	} else if (textureType_ == ComputePassTextureType::CubeMap) {
		// All 6 faces at once as a layered image; the shader picks its face from
		// gl_GlobalInvocationID.z.
		glBindImageTexture(0, tex_, 0, GL_TRUE, 0, GL_WRITE_ONLY, imageFormat_);
		glDispatchCompute((w_ + 7) / 8, (h_ + 7) / 8, 6);
	} else {
		glBindImageTexture(0, tex_, 0, GL_FALSE, 0, GL_WRITE_ONLY, imageFormat_);
		glDispatchCompute((w_ + 7) / 8, (h_ + 7) / 8, 1);
	}
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	dirty_ = false;
}
