#pragma once
#include <glad/glad.h>

// introduce ComputePass to work only if inputs change
class ComputePass {
	public:
		ComputePass(int w, int h, const char* shaderPath);
		~ComputePass();

		// GPU work is done when dirty flag is set true
		// dispatch() checks the flag and does GPU work
		void markDirty();
		void dispatch();
		GLuint texture() const { 
			return tex_; 
		}
	private:
		GLuint program_;
		GLuint tex_;
		int w_, h_;
		bool dirty_ = true;
};
