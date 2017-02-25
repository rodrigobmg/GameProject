#define MATERIAL_TEXCOORD_NUM 1
#define MATERIAL_BLENDING_MASKED 0
#include "MaterialCommon.glsl"
#include "ViewParam.glsl"


sampler2D TextureD;
sampler2D TextureS;


#ifdef VERTEX_SHADER

void  CalcMaterialInputVS(inout MaterialInputVS input, inout MaterialParametersVS parameters)
{
	input.worldOffset = float3(0, 0, 0);
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

void CalcMaterialInputPS(inout MaterialInputPS input, inout MaterialParametersPS parameters)
{
	float c = cos( 0.001 * parameters.worldPos.x );
	float3 n = normalize( float3(-c, 0, 1) );
	float3 t = normalize( float3(1, 0, c) );

	float3 baseColor = tex2D(TextureD, float2(parameters.texCoords[0].x, 1 - parameters.texCoords[0].y));

	input.baseColor = texture( TextureD , float2( parameters.texCoords[0].x , 1 - parameters.texCoords[0].y ) );
	input.emissiveColor = 0.2 * baseColor;
	//input.baseColor = float3(1,1,1) * float3( parameters.worldPos.xy , 0 );
	input.metallic = 0.9;
	input.roughness = 0.7;
	input.specular = 0.2;
	//float2 value = 2 * frac(10 * (parameters.worldPos.x + parameters.worldPos.y + parameters.worldPos.z )) - 1;
	float2 value = 2 * frac(10 * (parameters.worldPos.xy)) - 1;
	//input.mask = value.x * value.y > 0 ? 1 : 0;
	//input.normal = float3(0,0,1);
}

#endif //PIXEL_SHADER