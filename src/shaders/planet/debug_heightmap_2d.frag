#version 460 core
in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uHeightmap;
uniform int uChannel; // -1 = rgb, 0..3 = show one channel as grey

void main() {
	vec4 texel = texture(uHeightmap, vUV);
	vec3 color = (uChannel < 0) ? texel.rgb : vec3(texel[uChannel]);
	fragColor = vec4(color, 1.0);
}
