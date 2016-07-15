#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define TYPE_COLOR 1
#define TYPE_GRADIENT 2
#define TYPE_TEXTURE 3

#define TEXTYPE_RGBA 1
#define TEXTYPE_A 2

#define strokeThr -1.0f

layout(constant_id = 0) const bool edgeAntiAlias = true;

layout(location = 0) in vec2 ipos;
layout(location = 1) in vec2 itexcoord;

layout(location = 0) out vec4 ocolor;

layout(set = 0, binding = 0) uniform UBO
{
	//not used here; from vertex shader
	vec2 viewSize; //0

	//type to draw (TYPE_* macros)
	uint type; //8

	//type of the texture (if type is TYPE_TEXTURE)
	uint texType; //12

	//two colors values
	vec4 innerColor; //16
	vec4 outerColor; //32

	//mat3 is used as matrix.
	//mat[3][0;1] is used as scissor extent
	//mat[3][2;3] is used as scissor scale
	//mat[0][3] is used as radius
	//mat[1][3] is used as feather
	//mat[2][3] is used as strokeWidth
	mat4 scissorMat; //48

	//mat3 is used as matrix
	//mat[3][0;1] is used as extent
	//mat[3][2;3] is FREE
	//mat[0][3] is used as strokeMult
	//mat[1][3] is FREE
	//mat[2][3] is FREE
	mat4 paintMat; //112

} ubo;

layout(set = 0, binding = 1) uniform sampler2D tex; //for texture drawing

//CODE
float sdroundrect(vec2 pt, vec2 ext, float rad)
{
	vec2 ext2 = ext - vec2(rad, rad);
	vec2 d = abs(pt) - ext2;
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - rad;
}

float scissorMask(vec2 pos)
{
	vec2 sc = (abs((mat3(ubo.scissorMat) * vec3(pos, 1.0)).xy) - vec2(ubo.scissorMat[3]));
	sc = vec2(0.5, 0.5) - sc * vec2(ubo.scissorMat[3][2], ubo.scissorMat[3][3]);
	return clamp(sc.x, 0.0, 1.0) * clamp(sc.y, 0.0, 1.0);
}

float strokeMask()
{
	float strokeMult = ubo.paintMat[0][3];
	return min(1.0, (1.0 - abs(itexcoord.x * 2.0 - 1.0)) * strokeMult) * min(1.0, itexcoord.y);
}

void main()
{
	float scissorAlpha = scissorMask(ipos);
	if(edgeAntiAlias && scissorAlpha < 0.5f) discard;

	float strokeAlpha = strokeMask();
	if(strokeAlpha < strokeThr) discard;

	if(ubo.type == TYPE_COLOR)
	{
		ocolor = ubo.innerColor;
		if(edgeAntiAlias) ocolor *= strokeAlpha;
	}
	else if(ubo.type == TYPE_GRADIENT)
	{
		vec2 pt = (mat3(ubo.paintMat) * vec3(ipos, 1.0)).xy;
		float ft = ubo.scissorMat[1][3];
		float fac = sdroundrect(pt, vec2(ubo.paintMat[3]),
			ubo.scissorMat[0][3]) / ubo.scissorMat[1][3];
		ocolor = mix(ubo.innerColor, ubo.outerColor, clamp(0.5 + fac, 0.0, 1.0));
		if(edgeAntiAlias) ocolor *= strokeAlpha;
	}
	else if(ubo.type == TYPE_TEXTURE)
	{
		ocolor = texture(tex, itexcoord);
		if(ubo.texType == TEXTYPE_RGBA) ocolor = vec4(ocolor.xyz * ocolor.w, ocolor.w);
		else if(ubo.texType == TEXTYPE_A) ocolor = vec4(ocolor.x);
		ocolor = ocolor * ubo.innerColor;
	}

	ocolor *= scissorAlpha;
}
