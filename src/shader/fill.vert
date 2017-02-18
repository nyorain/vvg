#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 ivertex;
layout(location = 1) in vec2 itexcoord;

layout(location = 0) out vec2 opos;
layout(location = 1) out vec2 otexcoord;

layout(set = 0, binding = 0) uniform UBO
{
	vec2 viewSize;
} ubo;

void main()
{
	//just perform interpolation for texture coords and screen position
	otexcoord = itexcoord;
	opos = ivertex;

	//normalize the vertex coords from ([0, width], [0, height]) to ([-1, 1], [-1, 1]).
	//unlike in opengl there is no y inversion needed.
	gl_Position = vec4(2.0 * ivertex / ubo.viewSize - 1.0, 0.0, 1.0);
}
