#ifndef SHADERS_H
#define SHADERS_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path);

#endif