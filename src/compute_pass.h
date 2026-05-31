#pragma once
#include <glad/glad.h>
#include <string>

// introduce ComputePass to work only if inputs change
enum class ComputePassTextureType {
	Texture2D,
	CubeMap
};

class ComputePass {
	public:
		ComputePass(
			int w,
			int h,
			const char* shaderPath,
			ComputePassTextureType textureType = ComputePassTextureType::Texture2D,
			const void* const* cubeFaces = nullptr,
			const std::string& defines = std::string()
		);
		~ComputePass();

		// GPU work is done when dirty flag is set true
		// dispatch() checks the flag and does GPU work
		void markDirty();
		void dispatch();
		void reloadProgram(const std::string& defines);
		GLuint texture() const { 
			return tex_; 
		}
	private:
		std::string shaderPath_;
		GLuint program_;
		GLuint tex_;
		int w_, h_;
		ComputePassTextureType textureType_ = ComputePassTextureType::Texture2D;
		GLenum imageFormat_ = GL_RGBA32F;
		bool dirty_ = true;
};
