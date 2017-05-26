#include "Common.glsl"

struct OutputVSParam
{
	float4 color;
};

#if VERTEX_SHADER

uniform mat4 VertexTransform;

layout(location = ATTRIBUTE_POSITION) in float4 InPos;
layout(location = ATTRIBUTE_COLOR) in float4 InColor;

out OutputVSParam OutputVS;
void MainVS()
{
	gl_Position = VertexTransform * InPos;
	OutputVS.color = InColor;
}

#endif //VERTEX_SHADER

#if PIXEL_SHADER

layout(location = 0) out vec4 OutColor;
in OutputVSParam OutputVS;
void MainPS()
{
	OutColor = float4(OutputVS.color.rgb, 1);
}

#endif //PIXEL_SHADER
