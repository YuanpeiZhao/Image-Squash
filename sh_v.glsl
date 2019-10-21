#version 330

layout(location = 0) in vec3 pos;		// Model-space position
layout(location = 1) in vec2 aTexCoord;	// Texture coordinate

uniform mat4 xform;			// Model-to-clip space transform

out vec2 TexCoord;

void main() {
	// Transform vertex position
	gl_Position = xform * vec4(pos, 1.0);
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}