#version 330

in vec2 TexCoord;

// texture sampler
uniform sampler2D texture1;

out vec4 outCol;	// Final pixel color

void main() {

	outCol = texture(texture1, TexCoord);
}