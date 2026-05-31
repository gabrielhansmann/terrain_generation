// face -> direction helper
// maps a face index and thats face UV (0..1) to a unit direction on the sphere
// and uses OpenGL cubemap face convention so a later texture(cube, dir) lands
// back on exactly this face and texel. Write here and read in fragment shader
// adresses the same data
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
