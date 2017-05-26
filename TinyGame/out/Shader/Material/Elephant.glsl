#define MATERIAL_TEXCOORD_NUM 2
#define MATERIAL_BLENDING_MASKED 0

#include "MaterialCommon.glsl"
#include "ViewParam.glsl"

uniform sampler2D BaseTexture;

#if VERTEX_SHADER

void  CalcMaterialInputVS(inout MaterialInputVS input, inout MaterialParametersVS parameters)
{
	input.worldOffset = float3(0.0);//3 * sin( 10 * View.gameTime ) * parameters.noraml;
}

#endif //VERTEX_SHADER

#if PIXEL_SHADER

void CalcMaterialInputPS(inout MaterialInputPS input, inout MaterialParametersPS parameters)
{
	float s = 1;// 0.5 * (sin(10 * View.gameTime) + 1);
	input.shadingModel = SHADINGMODEL_DEFAULT_LIT;

	float3 baseColor = texture2D(BaseTexture, float2(parameters.texCoords[0].x, 1 - parameters.texCoords[0].y)).rgb;
	//input.baseColor = ( 0.5 * sin(View.gameTime) + 0.5 )* parameters.vectexColor;
	input.baseColor = s * baseColor * parameters.vectexColor.rgb /** frac( dot(parameters.worldPos, parameters.worldPos) / 100 )*/;
	//input.baseColor = float3(parameters.texCoords[0].xy, 0);
	input.metallic = 0.3;
	input.roughness = 0.83;
	input.emissiveColor = float3(0);
	//input.roughness = texture2D(BaseTexture, parameters.texCoords[0].xy).r * 0.9;
	input.specular = 1 * baseColor.r;
	float2 value = 2 * frac( 10 * ( parameters.texCoords[0].xy ) ) - 1;
	//input.mask = value.x * value.y > 0 ? 1 : 0;
	//input.emissiveColor = abs( float3(0.3, 0.3, 0.3) * parameters.worldNormal );
}

#endif //PIXEL_SHADER