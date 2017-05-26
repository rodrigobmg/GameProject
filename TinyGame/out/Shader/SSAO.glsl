#pragma once
#include "ScreenVertexShader.glsl"
#include "DeferredShadingCommon.glsl"
#include "ViewParam.glsl"

#if PIXEL_SHADER

in VSOutput vsOutput;

uniform sampler2D TextureSSAO;


uniform int KernelNum;
uniform float3 KernelVectors[128];

uniform float OcclusionRadius;
uniform float2 DepthBiasFactor = float2( 0.01 , 0.01 );

float3 GetAnyVericalVector(float3 v)
{
	float3 reslut = cross(v, float3(0, 0, 1));
	float len = length(reslut);

	if( len > 1e-5 )
		return  reslut / len;

	return normalize(cross(v, float3(1, 0, 0)) );
}

void GeneratePS()
{

	GBufferData GBuffer = GetGBufferData(vsOutput.UVs);

	float3 tangentZ = mat3( View.worldToView ) * GBuffer.normal;
	float3 tangentX = GetAnyVericalVector(tangentZ);
	float3 tangentY = cross(tangentZ, tangentX);

	mat3 tangentToView= mat3(tangentX, tangentY, tangentZ);

	float3 viewPos = View.worldToView * float4( GBuffer.worldPos , 1 );
	OcclusionRadius = 1;
	float occlusion = 0;
	for( int i = 0; i < KernelNum; ++i )
	{
		float3 offset = tangentToView * KernelVectors[i];
		float3 sampleViewPos = viewPos + offset * OcclusionRadius;
		float4 sampleClipPos = View.viewToClip * float4( sampleViewPos , 1 );
		float2 screenUV = 0.5 + 0.5 * sampleClipPos.xy / sampleClipPos.w;

		GBufferData sampleGBuffer = GetGBufferData(screenUV);
		float checkSceneDepth = BufferDepthToSceneDepth(sampleGBuffer.depth);

		float radiusCheck = smoothstep(0.0, 1.0, OcclusionRadius / abs( checkSceneDepth + viewPos.z ) );
		occlusion += radiusCheck * ((checkSceneDepth * ( 1 + DepthBiasFactor.x) + DepthBiasFactor.y >= -sampleViewPos.z ) ? 0 : 1);

	}
	float ambientFactor = 1.0 - occlusion / KernelNum;

	gl_FragData[0] = float4(ambientFactor, ambientFactor, ambientFactor, 1.0 );

}

static const float Weight[] =
{
	0.1 , 0.1 ,0.6 , 0.1 , 0.1
};

void BlurPS()
{
	float2 texSizeInv = 1.0 / float2(textureSize(TextureSSAO, 0));

	float3 color = 0;
#if 0
	for( int i = -2; i <= 2; ++i )
	{
		float3 cy = 0;
		for( int j = -2; j <= 2; ++j )
		{
			float3 c = texture2D(TextureSSAO, vsOutput.UVs + float2(float(i), float(j)) * texSizeInv ).rgb;
			cy += c * Weight[j+2];
		}
		color += cy * Weight[i + 2];
	}
#else
	for( int i = -2; i <= 2; ++i )
	{
		float3 cy = 0;
		cy += Weight[0] * texture2D(TextureSSAO, vsOutput.UVs + float2(float(i), float(-2)) * texSizeInv).rgb;
		cy += Weight[1] * texture2D(TextureSSAO, vsOutput.UVs + float2(float(i), float(-1)) * texSizeInv).rgb;
		cy += Weight[2] * texture2D(TextureSSAO, vsOutput.UVs + float2(float(i), float(-0)) * texSizeInv).rgb;
		cy += Weight[3] * texture2D(TextureSSAO, vsOutput.UVs + float2(float(i), float( 1)) * texSizeInv).rgb;
		cy += Weight[4] * texture2D(TextureSSAO, vsOutput.UVs + float2(float(i), float( 2)) * texSizeInv).rgb;
		color += cy * Weight[i + 2];
	}
#endif
	gl_FragData[0] = float4(color, 1);
}

float3 AmbientColor =  1 * float3(1,1,1);

void AmbientPS()
{
	float2 ScreenUVs = vsOutput.UVs;
	GBufferData GBuffer = GetGBufferData(ScreenUVs);
	float4 outColor;
	outColor.rgb = ( GBuffer.diffuseColor * AmbientColor ) * texture2D(TextureSSAO, ScreenUVs).r;
	//outColor.rgb = GBuffer.diffuseColor * AmbientColor;
	outColor.a = 1;
	gl_FragColor = outColor;
	//gl_FragColor = 0;
}

#endif //PIXEL_SHADER