#pragma once
#include <glad/glad.h>
#include <string>

// introduce ComputePass to work only if inputs change
enum class ComputePassTextureType {
	Texture2D,
	// 6 RGBA8 faces uploaded from images -> map a picture onto a sphere
	CubeMap,
	// 6 RGBA32F faces the compute shader fills in by direction, one dispatch
	// per face. This is the terrain heightmap: there is no single flat "up" to
	// index height by, so height is looked up by a direction from the center.
	CubeMapGenerated
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
		// Set the world-space noise offset (uWorldOffset). Marks dirty only when
		// the value actually changes, so a steady offset never forces a redispatch.
		void setWorldOffset(float x, float y);
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
		float worldOffsetX_ = 0.0f, worldOffsetY_ = 0.0f;
};
