#version 460 core
// renders one face of the terrain cubemap into a small panel
// this reads it the same way the sphere shader will eventually (by direction)
// so its ground truth for whether the compute step wrote the faces correctly
in vec2 vUV;
out vec4 fragColor;

uniform samplerCube uCube;
uniform int uFace;
uniform int uChannel; // -1 = rgb, 0..3 = show one channel as grey

// face -> direction mapping the compute shader writes with so
// the panel shows exactly the textels stored on this face 
// duplicated from the compute shader) -> shared include later
vec3 cubeFaceDir(int face, vec2 uv) {
	vec2 c = uv * 2.0 - 1.0; // face-local coords in [-1, 1]
	vec3 dir;
	if (face == 0) dir = vec3(1.0, -c.y, -c.x); 	 // +X
	else if (face == 1) dir = vec3(-1.0, -c.y, c.x); // -X
	else if (face == 2) dir = vec3(c.x, 1.0, c.y);   // +Y
	else if (face == 3) dir = vec3(c.x, -1.0, -c.y); // -Y
	else if (face == 4) dir = vec3(c.x, -c.y, 1.0);  // +Z
	else dir = vec3(-c.x, -c.y, -1.0); 			   	 // -Z
	return normalize(dir);
}

void main() {
	vec3 dir = cubeFaceDir(uFace, vUV);
	vec4 texel = texture(uCube, dir);
	vec3 color = (uChannel < 0) ? texel.rgb : vec3(texel[uChannel]);
	fragColor = vec4(color, 1.0);
}
