#pragma once
#include <glad/glad.h>
#include <vector>

class Framebuffer {
	public:
		Framebuffer(int w, int h, int numAttachments);
		~Framebuffer();
		void bind() const;
		void unbind() const;
		void resize(int w, int h);

		GLuint texture(int i) const;
		int width() const { return m_width; }
		int height() const { return m_height; }
	private:
		GLuint m_fbo;
		std::vector<GLuint> m_textures;
		int m_width = 0;
		int m_height = 0;
};
