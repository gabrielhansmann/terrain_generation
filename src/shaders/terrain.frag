#version 460 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 lightDir;
uniform vec3 viewPos;

void main()
{
    FragColor = vec4(1.0, .0, .0, 1.0);
}