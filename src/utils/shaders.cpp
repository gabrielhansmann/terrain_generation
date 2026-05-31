#include "shaders.h"
#include <iostream>
#include <sstream>
#include <string>

static std::string readFile(const std::string& path) {
	std::ifstream f(path);
	if (!f.is_open()) {
		std::cerr << "ERROR::SHADER::FILE_NOT_FOUND: " << path << "\n";
		return "";
	}
	std::ostringstream ss;
	ss << f.rdbuf();
	return ss.str();
}

static std::string resolveIncludes(const std::string& src, const std::string& dir) {
	std::istringstream stream(src);
	std::ostringstream out;
	std::string line;
	while (std::getline(stream, line)) {
		if (line.rfind("#include", 0) == 0) {
			auto a = line.find('"');
			auto b = line.rfind('"');
			if (a != b) {
				std::string name = line.substr(a + 1, b - a - 1);
				out << readFile(dir + "/" + name) << "\n";
				continue;
				}
		}
		out << line << "\n";
	}
	return out.str();
}

static std::string injectDefines(const std::string& src, const std::string& defines) {
	if (defines.empty())
		return src;

	std::istringstream stream(src);
	std::ostringstream out;
	std::string line;
	bool inserted = false;

	if (std::getline(stream, line)) {
		out << line << "\n";
		if (line.rfind("#version", 0) == 0) {
			out << defines;
			if (!defines.empty() && defines.back() != '\n')
				out << "\n";
			inserted = true;
		}
	}

	while (std::getline(stream, line))
		out << line << "\n";

	if (inserted)
		return out.str();

	return defines + "\n" + src;
}

static GLuint compileShader(GLenum type, const std::string& src, const std::string& label) {
	const char* code = src.c_str();
	GLuint id = glCreateShader(type);
	glShaderSource(id, 1, &code, nullptr);
	glCompileShader(id);
	GLint ok;
	glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[1024];
		glGetShaderInfoLog(id, 1024, nullptr, log);
		std::cerr << "ERROR::SHADER::COMPILE [" << label << "]\n" << log << "\n";
	}
	return id;
}

GLuint LoadShaders(const char* vertPath, const char* fragPath) {
	std::string vertSrc = resolveIncludes(readFile(vertPath), "shaders");
	std::string fragSrc = resolveIncludes(readFile(fragPath), "shaders");

	GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc, vertPath);
	GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc, fragPath);

	GLuint program =glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);

	GLint ok;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[1024];
		glGetProgramInfoLog(program, 1024, nullptr, log);
		std::cerr << "ERROR:SHADER::LINK\n" << log << "\n";
	}

	glDeleteShader(vert);
	glDeleteShader(frag);
	return program;

}

GLuint LoadShadersWithDefines(const char* vertPath, const char* fragPath, const std::string& defines) {
	std::string vertSrc = resolveIncludes(readFile(vertPath), "shaders");
	std::string fragSrc = resolveIncludes(readFile(fragPath), "shaders");

	vertSrc = injectDefines(vertSrc, defines);
	fragSrc = injectDefines(fragSrc, defines);

	GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc, vertPath);
	GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc, fragPath);

	GLuint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);

	GLint ok;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[1024];
		glGetProgramInfoLog(program, 1024, nullptr, log);
		std::cerr << "ERROR:SHADER::LINK\n" << log << "\n";
	}

	glDeleteShader(vert);
	glDeleteShader(frag);
	return program;
}

GLuint LoadComputeShader(const char* computePath) {
	std::string src = resolveIncludes(readFile(computePath), "shaders");
	GLuint comp = compileShader(GL_COMPUTE_SHADER, src, computePath);

	GLuint program = glCreateProgram();
	glAttachShader(program, comp);
	glLinkProgram(program);

	GLint ok;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[1024];
		glGetProgramInfoLog(program, 1024, nullptr, log);
		std::cerr << "ERROR:SHADER::LINK [" << computePath<< "]\n" << log << "\n";
	}

	glDeleteShader(comp);
	return program;
}

GLuint LoadComputeShaderWithDefines(const char* computePath, const std::string& defines) {
	std::string src = resolveIncludes(readFile(computePath), "shaders");
	src = injectDefines(src, defines);
	GLuint comp = compileShader(GL_COMPUTE_SHADER, src, computePath);

	GLuint program = glCreateProgram();
	glAttachShader(program, comp);
	glLinkProgram(program);

	GLint ok;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[1024];
		glGetProgramInfoLog(program, 1024, nullptr, log);
		std::cerr << "ERROR:SHADER::LINK [" << computePath<< "]\n" << log << "\n";
	}

	glDeleteShader(comp);
	return program;
}

void ReloadPrograms(GLuint& gbufferProgram, GLuint& lightingProgram, const std::string& defines) {
	GLuint newGBuffer = LoadShadersWithDefines("shaders/terrain.vert", "shaders/terrain_gbuffer.frag", defines);
	GLuint newLighting = LoadShadersWithDefines("shaders/fullscreen.vert", "shaders/lighting.frag", defines);

	if (gbufferProgram)
		glDeleteProgram(gbufferProgram);
	if (lightingProgram)
		glDeleteProgram(lightingProgram);

	gbufferProgram = newGBuffer;
	lightingProgram = newLighting;
}
