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
