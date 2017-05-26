#pragma once
#include "MaterialCommon.glsl"
#include "ViewParam.glsl"

//Material Template
#if 0
#if VERTEX_SHADER
void  CalcMaterialInputVS(inout MaterialInputVS input, in MaterialParametersVS parameters)
{
}

#endif //VERTEX_SHADER

#if PIXEL_SHADER
void CalcMaterialInputPS(inout MaterialInputPS input, in MaterialParametersPS parameters)
{
}
#endif //PIXEL_SHADER
#endif

#if PIXEL_SHADER

float3 GetMaterialWorldNormal(in MaterialInputPS input, inout MaterialParametersPS parameters)
{
#if MATERIAL_USE_WORLD_NORMAL
	return input.normal;
#else

	return normalize(parameters.tangentToWorld * input.normal);
#endif
}

float3 GetMaterialWorldPositionAndCheckDepthOffset(in MaterialInputPS input, inout MaterialParametersPS parameters)
{
	float3 worldPos = parameters.worldPos;
#if MATERIAL_USE_DEPTH_OFFSET
	float3 clipPos = parameters.clipPos;
	if( input.depthOffset != 0 )
	{
		clipPos.z += input.depthOffset;
		float4 worldPosH = View.clipToWorld * float4( clipPos , 1 );
		worldPos = worldPosH.xyz / worldPosH.w;
	}
	WritePxielDepth(clipPos.z);
#endif
	return worldPos;
	
}

void CalcMaterialParameters(inout MaterialInputPS input, inout MaterialParametersPS parameters)
{




	CalcMaterialInputPS(input, parameters);

#if MATERIAL_BLENDING_MASKED
	if( input.mask < 0.01 )
		discard;
#endif
}

#endif //PIXEL_SHADER