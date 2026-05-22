#include "framebuffer.h"
#include <iostream>
#include <vector>

Framebuffer::Framebuffer(int w, int h, int numAttachments) {
	m_textures.resize(numAttachments);
	glGenTextures(numAttachments, m_textures.data());

	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	std::vector<GLenum> drawBuffers(numAttachments);
	for (int i = 0; i < numAttachments; i++) {
		glBindTexture(GL_TEXTURE_2D, m_textures[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_textures[i], 0);
        drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
	}

	glDrawBuffers(numAttachments, drawBuffers.data());
	resize(w, h);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    	std::cerr << "G-buffer framebuffer is incomplete\n";
    }
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Framebuffer::resize(int w, int h) {
	if (w == m_width && h == m_height) return;

	m_width = w;
	m_height = h;

	for (GLuint tex : m_textures) {
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
	}
}

Framebuffer::~Framebuffer() {
	glDeleteTextures(m_textures.size(), m_textures.data());
	glDeleteFramebuffers(1, &m_fbo);
}

void Framebuffer::bind() const { 
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); 
}
void Framebuffer::unbind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
GLuint Framebuffer::texture(int i) const {
	return m_textures[i];
}


